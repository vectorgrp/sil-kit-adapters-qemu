// SPDX-FileCopyrightText: Copyright 2025 Vector Informatik GmbH
// SPDX-License-Identifier: MIT

#include "EthSocketToEthControllerAdapter.hpp"

#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <set>

#include "common/Exceptions.hpp"
#include "WriteUintBe.hpp"
#include "common/StringUtils.hpp"
#include "common/Parsing.hpp"
#include "common/SocketToBytesPubSubAdapter.hpp"

#include <asio/ts/buffer.hpp>
#include <asio/ts/net.hpp>
#include <asio/local/stream_protocol.hpp>

#include "silkit/services/ethernet/string_utils.hpp"

namespace adapters {
extern const std::string ethArg;
extern const std::string unixEthArg;
} // namespace adapters

using namespace SilKit::Services::Ethernet;
using namespace adapters::ethernet;
using namespace std::chrono_literals;
using namespace adapters;
using namespace util;
using namespace demo;

EthSocketToEthControllerAdapter::EthSocketToEthControllerAdapter(SilKit::IParticipant* participant,
                                                                 asio::io_context& io_context, const std::string& host,
                                                                 const std::string& service,
                                                                 const std::string& ethernetControllerName,
                                                                 const std::string& ethernetNetworkName,
                                                                 bool enableDomainSockets)
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
        debug_message << "Adapter >> SIL Kit: Ethernet frame (" << frameSize << " bytes, txId=" << transmitId << ")";
        _logger->Debug(debug_message.str());
    }))
{
    try
    {
        if (enableDomainSockets)
        {
            asio::local::stream_protocol::endpoint endpoint(host);
            _socket.connect(endpoint);
        }
        else
        {
            asio::ip::tcp::resolver resolver(io_context);
            auto endpoints = resolver.resolve(host, service);

            asio::ip::tcp::socket temp_socket(io_context);
            asio::connect(temp_socket, endpoints);
            _socket = asio::generic::stream_protocol::socket(std::move(temp_socket));
        }
    }
    catch (std::exception& e)
    {
        std::ostringstream error_message;
        error_message << e.what() << std::endl;
        if (enableDomainSockets)
        {
            error_message << "Error encountered while trying to connect to QEMU with \"" << unixEthArg << "\" at \""
                          << host << '"';
        }
        else
        {
            error_message << "Error encountered while trying to connect to QEMU with \"" << ethArg << "\" at \"" << host
                          << ':' << service << '"';
        }
        throw std::runtime_error(error_message.str());
    }
    _logger->Info("Connect success");
    DoReceiveFrameFromQemu();

    _ethController->AddFrameHandler([&](IEthernetController* /*controller*/, const EthernetFrameEvent& msg) {
        auto rawFrame = msg.frame.raw;
        SendEthernetFrameToQemu(rawFrame);
        std::ostringstream debug_message;
        debug_message << "SIL Kit >> Adapter: Ethernet frame (" << rawFrame.size() << " bytes)";
        _logger->Debug(debug_message.str());
    });
    _ethController->AddFrameTransmitHandler(
        [&](IEthernetController* /*controller*/, const EthernetFrameTransmitEvent& ack) -> void {
        std::ostringstream debug_message;
        if (ack.status == EthernetTransmitStatus::Transmitted)
        {
            debug_message << "SIL Kit >> Adapter: ACK for ETH Message with transmitId="
                          << reinterpret_cast<intptr_t>(ack.userContext);
        }
        else
        {
            debug_message << "SIL Kit >> Adapter: NACK for ETH Message with transmitId="
                          << reinterpret_cast<intptr_t>(ack.userContext) << ": " << ack.status;
        }
        _logger->Debug(debug_message.str());
    });

    _ethController->Activate();
}

void EthSocketToEthControllerAdapter::DoReceiveFrameFromQemu()
{
    asio::async_read(_socket, asio::buffer(_frame_size_buffer.data(), _frame_size_buffer.size()),
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
                              asio::buffer(_frame_data_buffer.data(), _frame_data_buffer.size()), frame_size);

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
    auto splitNetwork = util::split(networkname, "=");
    if (splitNetwork.size() == 2 && splitNetwork[0] == "network")
    {
        networkname = splitNetwork[1];
    }
    else
    {
        throw InvalidCli();
    }

    auto splitController = util::split(networkname, ":");
    if (splitController.size() == 2)
    {
        networkname = splitController[0];
        controllername = splitController[1];
    }
    else if (splitController.size() != 1)
    {
        throw InvalidCli();
    }
};

std::string generateControllerName()
{
    static const std::string base = "SilKit_ETH_CTRL_";
    static int count = 1;
    std::ostringstream subscriberName;
    return static_cast<std::ostringstream&>(subscriberName << base << count++).str();
}

std::unique_ptr<EthSocketToEthControllerAdapter> ethernet::parseEthernetSocketArgument(
    char* arg, std::set<std::string>& alreadyProvidedSockets, const std::string& participantName,
    asio::io_context& ioContext, SilKit::IParticipant* participant, SilKit::Services::Logging::ILogger* logger,
    const bool isUnixSocket)
{
    auto args = util::split(arg, ",");
    auto arg_iter = args.begin();

    assertAdditionalIterator(arg_iter, args);
    throwInvalidCliIf(alreadyProvidedSockets.insert(*arg_iter).second == false);

    std::string address, port, debug_message;
    if (isUnixSocket)
    {
        extractUnixSocket(address, port, arg_iter);
        debug_message = "Created Ethernet transmitter " + address;
    }
    else // TCP Socket
    {
        extractTcpSocket(address, port, arg_iter);
        debug_message = "Created Ethernet transmitter " + address + ':' + port;
    }

    //handle inbound topic and labels
    assertAdditionalIterator(arg_iter, args);
    std::string controllerName = "";
    extractAndEraseNamespaceAndControllernameFrom(*arg_iter, controllerName);
    if (controllerName == "")
        controllerName = generateControllerName();
    const std::string& networkName = *arg_iter;

    auto newAdapter = std::make_unique<EthSocketToEthControllerAdapter>(participant, ioContext, address, port,
                                                                        controllerName, networkName, isUnixSocket);

    debug_message += " (" + networkName + ')';
    logger->Debug(debug_message);

    return std::move(newAdapter);
}