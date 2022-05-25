// Copyright (c) Vector Informatik GmbH. All rights reserved.

#include "Device.hpp"

#include <iostream>
#include <string>
#include <vector>

#include "ib/IntegrationBus.hpp"
#include "ib/sim/all.hpp"
#include "ib/mw/sync/all.hpp"
#include "ib/mw/sync/string_utils.hpp"

using namespace ib::mw;
using namespace ib::sim;

using namespace std::chrono_literals;

void EthAckCallback(eth::IEthController* /*controller*/, const eth::EthTransmitAcknowledge& ack)
{
    if (ack.status == eth::EthTransmitStatus::Transmitted)
    {
        std::cout << "IB >> Demo: ACK for ETH Message with transmitId=" << ack.transmitId << std::endl;
    }
    else
    {
        std::cout << "IB >> Demo: NACK for ETH Message with transmitId=" << ack.transmitId;
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

/**************************************************************************************************
 * Main Function
 **************************************************************************************************/

int main(int argc, char**)
{
    const std::string participantConfigurationString =
        R"({ "Logging": { "Sinks": [ { "Type": "Stdout", "Level": "Info" } ] } })";

    const std::string participantName = "EthernetDevice";
    const std::uint32_t domainId = 42;

    const std::string ethernetControllerName = "Eth1";

    try
    {
        auto participantConfiguration = ib::cfg::ParticipantConfigurationFromString(participantConfigurationString);

        std::cout << "Creating participant '" << participantName << "' in domain " << domainId << std::endl;
        auto participant = ib::CreateSimulationParticipant(participantConfiguration, participantName, domainId, false);

        std::cout << "Creating ethernet controller '" << ethernetControllerName << "'" << std::endl;
        auto* ethController = participant->CreateEthController(ethernetControllerName);

        static constexpr auto ethernetAddress = demo::EthernetAddress{0x01, 0x23, 0x45, 0x67, 0x89, 0xab};
        static constexpr auto ip4Address = demo::Ip4Address{192, 168, 12, 35};
        auto demoDevice = demo::Device{ethernetAddress, ip4Address, [ethController](std::vector<std::uint8_t> data) {
                                           const auto frameSize = data.size();
                                           auto transmitId = ethController->SendFrame(eth::EthFrame(std::move(data)));
                                           std::cout << "Demo >> IB: Ethernet frame (" << frameSize
                                                     << " bytes, txId=" << transmitId << ")" << std::endl;
                                       }};

        ethController->RegisterReceiveMessageHandler(
            [&demoDevice](eth::IEthController* /*controller*/, const eth::EthMessage& msg) {
                auto rawFrame = msg.ethFrame.RawFrame();
                std::cout << "IB >> Demo: Ethernet frame (" << rawFrame.size() << " bytes)" << std::endl;
                demoDevice.Process(asio::buffer(rawFrame));
            });

        ethController->RegisterMessageAckHandler(&EthAckCallback);

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
