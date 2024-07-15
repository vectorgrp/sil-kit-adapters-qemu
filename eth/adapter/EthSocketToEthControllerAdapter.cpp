// Copyright (c) Vector Informatik GmbH. All rights reserved.

#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <set>

#include "adapter/Exceptions.hpp"
#include "WriteUintBe.hpp"
#include "../../chardev/Utility/StringUtils.hpp"
#include "adapter/Parsing.hpp"

#include <asio/ts/buffer.hpp>
#include <asio/ts/io_context.hpp>
#include <asio/ts/net.hpp>

#include "EthSocketToEthControllerAdapter.hpp"

using namespace SilKit::Services::Ethernet;
using namespace adapters::ethernet;
using namespace std::chrono_literals;
using namespace adapters;
using namespace demo;

adapters::ethernet::EthSocketToEthControllerAdapter::EthSocketToEthControllerAdapter(SilKit::IParticipant* participant,
                                                                   asio::io_context& io_context,
                                  const std::string& host, const std::string& service,
                                  const std::string ethernetControllerName, const std::string ethernetNetworkName)
    : _socket{io_context}
    , _logger{participant->GetLogger()}
    , _ethController(participant->CreateEthernetController(ethernetControllerName, ethernetNetworkName))
    , _onNewFrameHandler(std::move([&](std::vector<std::uint8_t> data) {
        if (data.size() < 60)
        {
            data.resize(60, 0);
        }
        const auto frameSize = data.size();
        static intptr_t transmitId = 0;
        _ethController->SendFrame(EthernetFrame{std::move(data)}, reinterpret_cast<void*>(++transmitId));
        std::ostringstream debug_message;
        debug_message << "QEMU >> SIL Kit: Ethernet frame (" << frameSize << " bytes, txId=" << transmitId << ")";
        _logger->Debug(debug_message.str());
    }))
{
    asio::ip::tcp::resolver resolver{io_context};
    try
    {
      const auto onReceiveEthernetFrameFromQemu = asio::connect(_socket, resolver.resolve(host, service));
    }
    catch (std::exception& e)
    {
      std::ostringstream error_message;
      error_message << e.what() << std::endl;
      error_message << "Error encountered while trying to connect to QEMU with \"" << ethArg << "\" at \"" << host
                    << ':' << service << '"';
      throw std::runtime_error(error_message.str());
    }
    _logger->Info("Connect success");
    DoReceiveFrameFromQemu();

    _ethController->AddFrameHandler([&](IEthernetController* /*controller*/, const EthernetFrameEvent& msg) {
        auto rawFrame = msg.frame.raw;
        SendEthernetFrameToQemu(rawFrame);
        std::ostringstream debug_message;
        debug_message << "SIL Kit >> QEMU: Ethernet frame (" << rawFrame.size() << " bytes)";
        _logger->Debug(debug_message.str());
    });
    _ethController->AddFrameTransmitHandler(
        [&](IEthernetController* /*controller*/, const EthernetFrameTransmitEvent& ack) -> void {
            std::ostringstream debug_message;
            if (ack.status == EthernetTransmitStatus::Transmitted)
            {
                debug_message << "SIL Kit >> Demo: ACK for ETH Message with transmitId="
                              << reinterpret_cast<intptr_t>(ack.userContext);
            }
            else
            {
                debug_message << "SIL Kit >> Demo: NACK for ETH Message with transmitId="
                              << reinterpret_cast<intptr_t>(ack.userContext) << ": " << ack.status;
            }
            _logger->Debug(debug_message.str());
        });

    _ethController->Activate();
}

void adapters::ethernet::EthSocketToEthControllerAdapter::DoReceiveFrameFromQemu()
{
    asio::async_read(
        _socket, asio::buffer(_frame_size_buffer.data(), _frame_size_buffer.size()),
        [this](const std::error_code ec, const std::size_t bytes_received) {
            if (ec || bytes_received != _frame_size_buffer.size())
            {
                throw IncompleteReadError{};
            }

            std::uint32_t frame_size = 0;
            for (unsigned byte_index = 0; byte_index < 4; ++byte_index)
            {
                frame_size |= static_cast<std::uint32_t>(_frame_size_buffer[4 - 1 - byte_index]) << (byte_index * 8u);
            }

            if (frame_size > _frame_data_buffer.size())
            {
                throw InvalidFrameSizeError{};
            }

            asio::async_read(_socket, asio::buffer(_frame_data_buffer.data(), frame_size),
                             [this, frame_size](const std::error_code ec, const std::size_t bytes_received) {
                                 if (ec || bytes_received != frame_size)
                                 {
                                     throw IncompleteReadError{};
                                 }

                                 auto frame_data = std::vector<std::uint8_t>(frame_size);
                                 asio::buffer_copy(asio::buffer(frame_data),
                                                   asio::buffer(_frame_data_buffer.data(), _frame_data_buffer.size()),
                                                   frame_size);

                                 _onNewFrameHandler(std::move(frame_data));

                                 DoReceiveFrameFromQemu();
                             });
        });
}

/// <summary>
/// parses networkname for format "network=networkname[:<controllername>]"
/// leaves networkname without extra said format after execution.
/// </summary>
/// <param name="networkname">input/output</param>
/// <param name="defaultname">output</param>
void extractAndEraseNamespaceAndControllernameFrom(std::string& networkname, std::string& controllername)
{
    auto splitNetwork = Utils::split(networkname, "=");
    if (splitNetwork.size() == 2 && splitNetwork[0] == "network")
    {
        networkname = splitNetwork[1];
    }
    else
    {
        throw InvalidCli();
    }

    auto splitController = Utils::split(networkname, ":");
    if (splitController.size() == 2)
    {
        networkname = splitController[0];
        controllername = splitController[1];
    }
    else if(splitController.size()!=1)
    {
        throw InvalidCli();
    }
};

std::string generateControllerNameFrom(const std::string& participantName)
{
    static const std::string base = participantName + "_eth_";
    static int count = 1;
    std::ostringstream subscriberName;
    return static_cast<std::ostringstream&>(subscriberName << base << count++).str();
}

EthSocketToEthControllerAdapter* adapters::ethernet::parseEthernetSocketArgument(char* arg,
                                                                        std::set<std::string>& alreadyProvidedSockets,
                                                                        const std::string& participantName,
                                                                        asio::io_context& ioContext,
                                                                        SilKit::IParticipant* participant,
                                                                        SilKit::Services::Logging::ILogger* logger)
{
    EthSocketToEthControllerAdapter* newAdapter=NULL;
    auto args = Utils::split(arg, ",");
    auto arg_iter = args.begin();

    //handle <address>:<port>
    assertAdditionalIterator(arg_iter, args);
    throwInvalidCliIf(alreadyProvidedSockets.insert(*arg_iter).second == false);
    auto portAddress = Utils::split(*arg_iter++, ":");
    throwInvalidCliIf(portAddress.size() != 2);
    const auto& address = portAddress[0];
    const auto& port = portAddress[1];
    //handle inbound topic and labels
    assertAdditionalIterator(arg_iter, args);
    std::string controllerName = "";
    extractAndEraseNamespaceAndControllernameFrom(*arg_iter, controllerName);
    if (controllerName == "")
        controllerName = generateControllerNameFrom(participantName);
    const std::string& networkName = *arg_iter;
    
    newAdapter = new EthSocketToEthControllerAdapter(participant, ioContext, address, port, controllerName,
                                                       networkName);

    logger->Debug("Created Ethernet transmitter " + address + ':' + port + " (" + networkName + ')');

    return newAdapter;
}