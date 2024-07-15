// Copyright (c) Vector Informatik GmbH. All rights reserved.

#include "Device.hpp"

#include <iostream>
#include <string>
#include <vector>

#include "silkit/SilKit.hpp"
#include "silkit/config/all.hpp"
#include "silkit/services/ethernet/all.hpp"
#include "silkit/services/ethernet/string_utils.hpp"
#include "../../adapter/Parsing.hpp"
#include "../adapter/SignalHandler.hpp"

using namespace adapters;

using namespace SilKit::Services::Ethernet;

using namespace std::chrono_literals;

void promptForExit()
{    
    std::promise<int> signalPromise;
    auto signalValue = signalPromise.get_future();
    RegisterSignalHandler([&signalPromise](auto sigNum) {
        signalPromise.set_value(sigNum);
    });
        
    std::cout << "Press CTRL + C to stop the process..." << std::endl;

    signalValue.wait();

    std::cout << "\nSignal " << signalValue.get() << " received!" << std::endl;
    std::cout << "Exiting..." << std::endl;
}

void EthAckCallback(IEthernetController* /*controller*/, const EthernetFrameTransmitEvent& ack)
{
    if (ack.status == EthernetTransmitStatus::Transmitted)
    {
        std::cout << "SIL Kit >> Demo: ACK for ETH Message with transmitId="
                  << reinterpret_cast<intptr_t>(ack.userContext) << std::endl;
    }
    else
    {
        std::cout << "SIL Kit >> Demo: NACK for ETH Message with transmitId="
                  << reinterpret_cast<intptr_t>(ack.userContext)
                  << ": " << ack.status
                  << std::endl;
    }
}

/**************************************************************************************************
 * Main Function
 **************************************************************************************************/

int main(int argc, char** argv)
{
    if (findArg(argc, argv, "--help", argv) != nullptr)
    {
        std::cout << "Usage (defaults in curly braces if you omit the switch):" << std::endl
                  << "SilKitDemoEthernetIcmpEchoDevice [" << participantNameArg << " <participant's name{EthernetDevice}>]\n"
                     "  [" << regUriArg << " silkit://<host{localhost}>:<port{8501}>]\n"
                     "  [" << logLevelArg << " <Trace|Debug|Warn|{Info}|Error|Critical|Off>]\n";
        return 0;
    }
    
    const std::string loglevel = getArgDefault(argc, argv, logLevelArg, "Info");
    const std::string participantName = getArgDefault(argc, argv, participantNameArg, "EthernetDevice");
    const std::string registryURI = getArgDefault(argc, argv, regUriArg, "silkit://localhost:8501");

    const std::string participantConfigurationString =
        R"({ "Logging": { "Sinks": [ { "Type": "Stdout", "Level": ")" + loglevel + R"("} ] } })";

    const std::string ethernetControllerName = participantName + "_Eth1";
    const std::string ethernetNetworkName ="qemu_demo";

    try
    {
        auto participantConfiguration = SilKit::Config::ParticipantConfigurationFromString(participantConfigurationString);

        std::cout << "Creating participant '" << participantName << "' at " << registryURI << std::endl;
        auto participant = SilKit::CreateParticipant(participantConfiguration, participantName, registryURI);

        std::cout << "Creating ethernet controller '" << ethernetControllerName << "'" << std::endl;
        auto* ethController = participant->CreateEthernetController(ethernetControllerName, ethernetNetworkName);

        static constexpr auto ethernetAddress = demo::EthernetAddress{0x01, 0x23, 0x45, 0x67, 0x89, 0xab};
        static constexpr auto ip4Address = demo::Ip4Address{192, 168, 7, 35};
        demo::Device demoDevice{ethernetAddress, ip4Address, [ethController](std::vector<std::uint8_t> data) {
                                    const auto frameSize = data.size();
                                    static intptr_t transmitId = 0;
                                    ethController->SendFrame(EthernetFrame{std::move(data)},
                                                             reinterpret_cast<void*>(++transmitId));
                                    std::cout << "Demo >> SIL Kit: Ethernet frame (" << frameSize
                                              << " bytes, txId=" << transmitId << ")" << std::endl;
                               }};

        ethController->AddFrameHandler(
            [&demoDevice](IEthernetController* /*controller*/, const EthernetFrameEvent& msg) {
                auto rawFrame = msg.frame.raw;
                std::cout << "SIL Kit >> Demo: Ethernet frame (" << rawFrame.size() << " bytes)" << std::endl;
                demoDevice.Process(asio::buffer(rawFrame.data(),rawFrame.size()));
            });

        ethController->AddFrameTransmitHandler(&EthAckCallback);

        ethController->Activate();

        promptForExit();
    }
    catch (const SilKit::ConfigurationError& error)
    {
        std::cerr << "Invalid configuration: " << error.what() << std::endl;
        return -2;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Something went wrong: " << error.what() << std::endl;
        return -3;
    }

    return 0;
}
