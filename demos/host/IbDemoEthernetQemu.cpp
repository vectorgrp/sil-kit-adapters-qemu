// Copyright (c) Vector Informatik GmbH. All rights reserved.

#include <algorithm>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "ib/IntegrationBus.hpp"
#include "ib/sim/all.hpp"
#include "ib/mw/sync/all.hpp"
#include "ib/mw/sync/string_utils.hpp"

#include "Exceptions.hpp"
#include "WriteUintBe.hpp"

#include <asio/ts/buffer.hpp>
#include <asio/ts/io_context.hpp>
#include <asio/ts/net.hpp>

using namespace ib::mw;
using namespace ib::sim;

using namespace std::chrono_literals;

class QemuSocketClient
{
public:
    QemuSocketClient(asio::io_context& io_context, std::string_view host, std::string_view service,
                     std::function<void(std::vector<uint8_t>)> onNewFrameHandler)
        : _socket{io_context}
        , _resolver{io_context}
        , mOnNewFrameHandler(onNewFrameHandler)
    {
        asio::connect(_socket, _resolver.resolve(host, service));
        std::cout << "connect success" << std::endl;
        DoReceiveFrameFromQemu();
    }

    void SendEthernetFrameToQemu(const std::vector<uint8_t>& data)
    {
        std::array<std::uint8_t, 4> frameSizeBytes = {};
        demo::WriteUintBe<std::uint32_t>(asio::buffer(frameSizeBytes), data.size());

        asio::write(_socket, asio::buffer(frameSizeBytes));
        asio::write(_socket, asio::buffer(data));
    }

private:
    void DoReceiveFrameFromQemu()
    {
        asio::async_read(_socket, asio::buffer(_frame_size_buffer.data(), _frame_size_buffer.size()),
                         [this](const std::error_code ec, const std::size_t bytes_received) {
                             if (ec || bytes_received != _frame_size_buffer.size())
                             {
                                 throw demo::IncompleteReadError{};
                             }

                             std::uint32_t frame_size = 0;
                             for (unsigned byte_index = 0; byte_index < 4; ++byte_index)
                             {
                                 frame_size |= static_cast<std::uint32_t>(_frame_size_buffer[4 - 1 - byte_index])
                                               << (byte_index * 8u);
                             }

                             if (frame_size > _frame_data_buffer.size())
                             {
                                 throw demo::InvalidFrameSizeError{};
                             }

                             asio::async_read(
                                 _socket, asio::buffer(_frame_data_buffer.data(), frame_size),
                                 [this, frame_size](const std::error_code ec, const std::size_t bytes_received) {
                                     if (ec || bytes_received != frame_size)
                                     {
                                         throw demo::IncompleteReadError{};
                                     }

                                     auto frame_data = make_frame_data(frame_size);
                                     asio::buffer_copy(
                                         asio::buffer(frame_data),
                                         asio::buffer(_frame_data_buffer.data(), _frame_data_buffer.size()),
                                         frame_size);

                                     print_frame_data(frame_data);

                                     mOnNewFrameHandler(frame_data);
                                     DoReceiveFrameFromQemu();
                                 });
                         });
    }

    void print_frame_data(std::vector<uint8_t>& frame_data)
    {
        std::cout << "frame data ";
        constexpr auto nibble_to_hex_char = [](const unsigned nibble) {
            if (nibble < 10)
                return '0' + nibble;
            else
                return 'a' + (nibble - 10u);
        };

        auto it = std::ostream_iterator<char>{std::cout};

        std::size_t index = 0;
        for (auto byte : frame_data)
        {
            if (index++ > 0)
                *it++ = ':';
            *it++ = nibble_to_hex_char((static_cast<unsigned>(byte) & 0xF0) >> 4u);
            *it++ = nibble_to_hex_char((static_cast<unsigned>(byte) & 0x0F) >> 0u);
        }
    }

    std::vector<uint8_t> make_frame_data(std::size_t size)
    {
        std::vector<uint8_t> frame_data;
        frame_data.resize(size);
        return frame_data;
    }

private:
    asio::ip::tcp::socket _socket;
    asio::ip::tcp::resolver _resolver;

    std::array<uint8_t, 4> _frame_size_buffer;
    std::array<uint8_t, 4096> _frame_data_buffer;

    std::function<void(std::vector<uint8_t>)> mOnNewFrameHandler;
};

std::string GetPayloadStringFromRawFrame(const std::vector<uint8_t>& rawFrame)
{
    std::vector<uint8_t> payload;
    payload.insert(payload.end(), rawFrame.begin() + 14 /*destination[6]+source[6]+ethType[2]*/,
                   rawFrame.end() - 4); // CRC
    std::string payloadString(payload.begin(), payload.end());
    return payloadString;
}

void EthAckCallback(eth::IEthController* /*controller*/, const eth::EthTransmitAcknowledge& ack)
{
    if (ack.status == eth::EthTransmitStatus::Transmitted)
    {
        std::cout << ">> ACK for ETH Message with transmitId=" << ack.transmitId << std::endl;
    }
    else
    {
        std::cout << ">> NACK for ETH Message with transmitId=" << ack.transmitId;
        switch (ack.status)
        {
        case eth::EthTransmitStatus::Transmitted: break;
        case eth::EthTransmitStatus::InvalidFrameFormat: std::cout << ": InvalidFrameFormat"; break;
        case eth::EthTransmitStatus::ControllerInactive: std::cout << ": ControllerInactive"; break;
        case eth::EthTransmitStatus::LinkDown: std::cout << ": LinkDown"; break;
        case eth::EthTransmitStatus::Dropped: std::cout << ": Dropped"; break;
        case eth::EthTransmitStatus::DuplicatedTransmitId: std::cout << ": DuplicatedTransmitId"; break;
        }

        std::cout << std::endl;
    }
}

void ReceiveEthMessage(eth::IEthController* /*controller*/, const eth::EthMessage& msg)
{
    auto rawFrame = msg.ethFrame.RawFrame();
    auto payload = GetPayloadStringFromRawFrame(rawFrame);

    std::cout << ">> ETH Message: \"" << payload << "\"" << std::endl;
}

void SendMessage(eth::IEthController* controller, std::vector<uint8_t> data)
{
    eth::EthFrame frame;
    frame.SetRawFrame(data);

    auto transmitId = controller->SendFrame(std::move(frame));
    std::cout << "<< ETH Frame sent with transmitId=" << transmitId << std::endl;
}

const std::uint32_t domainId = 42;
const std::string participantName = "EthernetQemu";
const std::string participantConfigurationString = R"(
"Logging": { "Sinks": [ { "Type": "Stdout", "Level": "Info" } ] }
)";

int main(int argc, char**)
{
    try
    {
        auto participantConfiguration = ib::cfg::ParticipantConfigurationFromString(participantConfigurationString);

        std::cout << "Creating participant '" << participantName << "' in domain " << domainId << std::endl;
        auto participant = ib::CreateSimulationParticipant(participantConfiguration, participantName, domainId, false);
        auto* ethController = participant->CreateEthController("Eth1");

        asio::io_context ioContext;

        QemuSocketClient client{ioContext, "localhost", "12345", [ethController](std::vector<std::uint8_t> data) {
                                    SendMessage(ethController, data);
                                }};

        ethController->RegisterReceiveMessageHandler(
            [&client](eth::IEthController* /*controller*/, const eth::EthMessage& msg) {
                auto rawFrame = msg.ethFrame.RawFrame();
                auto payload = GetPayloadStringFromRawFrame(rawFrame);

                client.SendEthernetFrameToQemu(rawFrame);

                std::cout << ">> ETH Message: \"" << payload << "\"" << std::endl;
            });

        ethController->RegisterMessageAckHandler(&EthAckCallback);

        ioContext.run();

        std::cout << "Press enter to stop the process..." << std::endl;
        std::cin.ignore();
    }
    catch (const ib::ConfigurationError& error)
    {
        std::cerr << "Invalid configuration: " << error.what() << std::endl;
        std::cout << "Press enter to stop the process..." << std::endl;
        std::cin.ignore();
        return -2;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Something went wrong: " << error.what() << std::endl;
        std::cout << "Press enter to stop the process..." << std::endl;
        std::cin.ignore();
        return -3;
    }

    return 0;
}
