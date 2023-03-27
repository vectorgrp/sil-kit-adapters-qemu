// Copyright (c) Vector Informatik GmbH. All rights reserved.

#include "SilKitAdapterQemu.hpp"

#include <iostream>
#include <string>
#include <thread>
#include <vector>
#include <set>
#include <algorithm>

#include "Exceptions.hpp"
#include "Parsing.hpp"
#include "../chardev/adapter/ChardevSocketToPubSubAdapter.hpp"
#include "../eth/adapter/EthSocketToEthControllerAdapter.hpp"

#include "silkit/SilKit.hpp"
#include "silkit/config/all.hpp"
#include "silkit/services/pubsub/all.hpp"
#include "silkit/services/logging/all.hpp"
#include "silkit/util/serdes/Serialization.hpp"

#include "../chardev/Utility/StringUtils.hpp"

using namespace adapters;
using namespace adapters::chardev;
using namespace adapters::ethernet;
using namespace std::chrono_literals;

void promptForExit()
{
    std::cout << "Press enter to stop the process..." << std::endl;
    std::cin.ignore();
}

int main(int argc, char** argv)
{

    if (findArg(argc, argv, helpArg, argv) != NULL)
    {
        print_help(true);
        return NO_ERROR;
    }

    const std::string loglevel = getArgDefault(argc, argv, logLevelArg, "Info");
    const std::string participantConfigurationString =
        R"({ "Logging": { "Sinks": [ { "Type": "Stdout", "Level": ")" + loglevel + R"("} ] } })";

    asio::io_context ioContext;

    const std::string participantName = getArgDefault(argc, argv, participantNameArg, "SilKitAdapterQemu");

    const std::string registryURI = getArgDefault(argc, argv, regUriArg, "silkit://localhost:8501");

    unsigned numberOfRequestedAdaptations = 0;
    try
    {
        throwInvalidCliIf(thereAreUnknownArguments(argc, argv));  

        auto participantConfiguration =
            SilKit::Config::ParticipantConfigurationFromString(participantConfigurationString);

        std::cout << "Creating participant '" << participantName << "' at " << registryURI << std::endl;
        std::unique_ptr<SilKit::IParticipant> p_container = SilKit::CreateParticipant(participantConfiguration, participantName, registryURI);
        auto participant = p_container.get();

        auto logger = participant->GetLogger();

        std::vector<ChardevSocketToPubSubAdapter*> chardevSocketTransmitters;

        //set to ensure the provided sockets are unique (text-based)
        std::set<std::string> alreadyProvidedSockets;

        foreachArgDo(argc, argv, chardevArg, [&](char* arg) -> void {
            ++numberOfRequestedAdaptations;
            chardevSocketTransmitters.push_back(parseChardevSocketArgument(
                arg, alreadyProvidedSockets, participantName, ioContext, participant, logger));
        });

        std::vector<EthSocketToEthControllerAdapter*> ethernetSocketTransmitters;

        foreachArgDo(argc, argv, ethArg, [&](char* arg) -> void {
            ++numberOfRequestedAdaptations;
            ethernetSocketTransmitters.push_back(parseEthernetSocketArgument(
                arg, alreadyProvidedSockets, participantName, ioContext, participant, logger));
        });

        throwInvalidCliIf(numberOfRequestedAdaptations == 0);

        ioContext.run();

        promptForExit();
    }
    catch( const SilKit::ConfigurationError& error )
    {
        std::cerr << "Invalid configuration: " << error.what() << std::endl;
        promptForExit();
        return CONFIGURATION_ERROR;
    }
    catch( const InvalidCli& )
    {
        adapters::print_help();
        std::cerr << std::endl
                  << "Invalid command line arguments." << std::endl;
        promptForExit();
        return CLI_ERROR;
    }
    catch( const std::exception& error )
    {
        std::cerr << "Something went wrong: " << error.what() << std::endl;
        promptForExit();
        return OTHER_ERROR;
    }

    return NO_ERROR;
}
