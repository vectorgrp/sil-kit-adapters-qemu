// Copyright (c) Vector Informatik GmbH. All rights reserved.

#include "SilKitAdapter.hpp"

#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <set>
#include <algorithm>

#include "Exceptions.hpp"
#include "Parsing.hpp"
#include "../chardev/adapter/ChardevSocketToPubSubAdapter.hpp"

#include "silkit/SilKit.hpp"
#include "silkit/config/all.hpp"
#include "silkit/services/pubsub/all.hpp"
#include "silkit/services/logging/all.hpp"
#include "silkit/util/serdes/Serialization.hpp"

#include "../chardev/Utility/StringUtils.hpp"

using namespace SilKit::Services::PubSub;
using namespace adapters;
using namespace demo;
using namespace demo::chardev;
using namespace std::chrono_literals;

int main(int argc, char** argv)
{
    if (findArg(argc, argv, "--help", argv) != NULL)
    {
        adapters::print_help(true);
        return NO_ERROR;
    }

    const std::string loglevel = getArgDefault(argc, argv, "--log", "Info");
    const std::string participantConfigurationString =
        R"({ "Logging": { "Sinks": [ { "Type": "Stdout", "Level": ")" + loglevel + R"("} ] } })";

    asio::io_context ioContext;

    const std::string participantName = getArgDefault(argc, argv, "--name", "ChardevAdapter");

    const std::string registryURI = getArgDefault(argc, argv, "--registry-uri", "silkit://localhost:8501");

    unsigned numberOfRequestedAdaptations = 0;
    try
    {
        auto participantConfiguration =
            SilKit::Config::ParticipantConfigurationFromString(participantConfigurationString);

        std::cout << "Creating participant '" << participantName << "' at " << registryURI << std::endl;
        std::unique_ptr<SilKit::IParticipant> p_container = SilKit::CreateParticipant(participantConfiguration, participantName, registryURI);
        auto participant = p_container.get();

        auto logger = participant->GetLogger();

        std::vector<ChardevSocketToPubSubAdapter*> transmitters;
        char** chardevSocketTransmitterArg = NULL;

        //set to ensure the provided sockets are unique (text-based)
        std::set<std::string> alreadyProvidedSockets;

        for (chardevSocketTransmitterArg = findArgOf(argc, argv, "--socket-to-chardev", argv);
             chardevSocketTransmitterArg != NULL;
             chardevSocketTransmitterArg =
                 findArgOf(argc, argv, "--socket-to-chardev", chardevSocketTransmitterArg + 1))
        {
            ++numberOfRequestedAdaptations;
            transmitters.push_back(parseChardevSocketArgument(
                chardevSocketTransmitterArg, alreadyProvidedSockets,
                participantName, ioContext, participant, logger));
        }

        throwInvalidCliIf(numberOfRequestedAdaptations == 0);

        ioContext.run();

        std::cout << "Press enter to stop the process..." << std::endl;
        std::cin.ignore();
    }
    catch( const SilKit::ConfigurationError& error )
    {
        std::cerr << "Invalid configuration: " << error.what() << std::endl;
        std::cout << "Press enter to stop the process..." << std::endl;
        std::cin.ignore();
        return CONFIGURATION_ERROR;
    }
    catch( const InvalidCli& )
    {
        std::cerr << "Invalid command line arguments." << std::endl;
        adapters::print_help();
        std::cout << "Press enter to stop the process..." << std::endl;
        std::cin.ignore();
        return CLI_ERROR;
    }
    catch( const std::exception& error )
    {
        std::cerr << "Something went wrong: " << error.what() << std::endl;
        std::cout << "Press enter to stop the process..." << std::endl;
        std::cin.ignore();
        return OTHER_ERROR;
    }

    return NO_ERROR;
}
