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
#include "QemuSocketClient.hpp"

using namespace ib::mw;
using namespace ib::sim;

using namespace std::chrono_literals;

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
        case eth::EthTransmitStatus::Transmitted:
            break;
        case eth::EthTransmitStatus::InvalidFrameFormat:
            std::cout << ": InvalidFrameFormat";
            break;
        case eth::EthTransmitStatus::ControllerInactive:
            std::cout << ": ControllerInactive";
            break;
        case eth::EthTransmitStatus::LinkDown:
            std::cout << ": LinkDown";
            break;
        case eth::EthTransmitStatus::Dropped:
            std::cout << ": Dropped";
            break;
        case eth::EthTransmitStatus::DuplicatedTransmitId:
            std::cout << ": DuplicatedTransmitId";
            break;
        }

        std::cout << std::endl;
    }
}

void ReceiveEthMessage(eth::IEthController* /*controller*/, const eth::EthMessage& msg)
{
    auto rawFrame = msg.ethFrame.RawFrame();
    auto payload = GetPayloadStringFromRawFrame(rawFrame);


    std::cout << ">> ETH Message: \""
              << payload
              << "\"" << std::endl;
}

void SendMessage(eth::IEthController* controller, std::vector<uint8_t> data)
{
    eth::EthFrame frame;
    frame.SetRawFrame(data);

    auto transmitId = controller->SendFrame(std::move(frame));
    std::cout << "<< ETH Frame sent with transmitId=" << transmitId << std::endl;
}

/**************************************************************************************************
 * Main Function
 **************************************************************************************************/

int main(int argc, char**)
{
    try
    {
        std::string participantConfigurationString("\"Logging\": { \"Sinks\": [ { \"Type\": \"Stdout\", \"Level\": \"Info\" } ] }");
        std::string participantName("EthernetWriter");

        uint32_t domainId = 42;

        if(argc == 2)
        {
            participantName = "EthernetReader";
        }
        
        auto participantConfiguration = ib::cfg::ParticipantConfigurationFromString("");

        std::cout << "Creating participant '" << participantName << "' in domain " << domainId << std::endl;
        auto participant = ib::CreateParticipant(participantConfiguration, participantName, domainId, false);
        auto* ethController = participant->CreateEthController("Eth1");

        try
        {
            asio::io_context ioContext;
            QemuSocketClient client{ioContext, "localhost", "12345", [ethController](std::vector<uint8_t> data){
              SendMessage(ethController, data);
            }};

            ethController->RegisterReceiveMessageHandler([&client](eth::IEthController* /*controller*/, const eth::EthMessage& msg)
            {
                auto rawFrame = msg.ethFrame.RawFrame();
                auto payload = GetPayloadStringFromRawFrame(rawFrame);

                client.do_send(rawFrame);

                std::cout << ">> ETH Message: \""
                          << payload
                          << "\"" << std::endl;
            });

            ethController->RegisterMessageAckHandler(&EthAckCallback);

            ioContext.run();
        }
        catch (const std::exception& exception)
        {
            std::cerr << "error: " << exception.what() << std::endl;
            return EXIT_FAILURE;
        }

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
