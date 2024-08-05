// Copyright (c) Vector Informatik GmbH. All rights reserved.

#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>

#include "../../adapter/Parsing.hpp"
#include "../adapter/SignalHandler.hpp"

#include "silkit/SilKit.hpp"
#include "silkit/config/all.hpp"
#include "silkit/services/pubsub/all.hpp"
#include "silkit/util/serdes/Serialization.hpp"

using namespace adapters;

using namespace SilKit::Services::PubSub;

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

/**************************************************************************************************
 * Main Function
 **************************************************************************************************/

int main(int argc, char** argv)
{
    if (findArg(argc, argv, "--help", argv) != nullptr)
    {
        std::cout << "Usage (defaults in curly braces if you omit the switch):" << std::endl
                  << "SilKitDemoChardevEchoDevice [" << participantNameArg << " <participant's name{EchoDevice}>]\n"
                     "  ["<<regUriArg<<" silkit://<host{localhost}>:<port{8501}>]\n"
                     "  ["<<logLevelArg<<" <Trace|Debug|Warn|{Info}|Error|Critical|Off>]\n";
        return 0;
    }

    const std::string loglevel = getArgDefault(argc, argv, logLevelArg, "Info");
    const std::string participantName = getArgDefault(argc, argv, participantNameArg, "EchoDevice");
    const std::string registryURI = getArgDefault(argc, argv, regUriArg, "silkit://localhost:8501");

    const std::string participantConfigurationString =
        R"({ "Logging": { "Sinks": [ { "Type": "Stdout", "Level": ")" + loglevel + R"("} ] } })";

    const auto create_pubsubspec = [](const std::string& topic_name,
                                SilKit::Services::MatchingLabel::Kind matching_mode) {
        PubSubSpec r(topic_name, SilKit::Util::SerDes::MediaTypeData());
        r.AddLabel("VirtualNetwork", "Default", matching_mode);
        r.AddLabel("Namespace", "Namespace", matching_mode);
        return r;
    };

    PubSubSpec subDataSpec = create_pubsubspec("fromChardev", SilKit::Services::MatchingLabel::Kind::Mandatory);
    subDataSpec.AddLabel("Instance", "Adapter", SilKit::Services::MatchingLabel::Kind::Mandatory);

    PubSubSpec pubDataSpec = create_pubsubspec("toChardev", SilKit::Services::MatchingLabel::Kind::Optional);
    pubDataSpec.AddLabel("Instance", participantName, SilKit::Services::MatchingLabel::Kind::Optional);

    try
    {
        auto participantConfiguration =
            SilKit::Config::ParticipantConfigurationFromString(participantConfigurationString);

        std::cout << "Creating participant '" << participantName << "' at " << registryURI << std::endl;
        auto participant = SilKit::CreateParticipant(participantConfiguration, participantName, registryURI);

        auto dataPublisher = participant->CreateDataPublisher(participantName + "_pub", pubDataSpec);

        std::string line_buffer;
        
        auto dataSubscriber = participant->CreateDataSubscriber(
            participantName + "_sub", subDataSpec,
            [&](SilKit::Services::PubSub::IDataSubscriber* subscriber, const DataMessageEvent& dataMessageEvent) {
                if (dataMessageEvent.data.size() <= 4)
                {
                    std::cerr << "warning: message received probably wasn't following SAB format."<<std::endl;
                    line_buffer += std::string(reinterpret_cast<const char*>(dataMessageEvent.data.data()),
                                               dataMessageEvent.data.size());
                }
                else
                {
                    line_buffer += std::string(reinterpret_cast<const char*>(dataMessageEvent.data.data() + 4),
                                               dataMessageEvent.data.size() - 4);
                }
                std::string::size_type newline_pos;
                while ( (newline_pos = line_buffer.find_first_of('\n')) != std::string::npos)
                {
                    std::cout << "SIL Kit >> SIL Kit: " << line_buffer.substr(0,newline_pos) << std::endl;
                    line_buffer.erase(0, newline_pos + 1);
                }
                dataPublisher->Publish(dataMessageEvent.data);
            });

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
