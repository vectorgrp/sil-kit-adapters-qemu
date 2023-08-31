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
using namespace SilKit::Services::Orchestration;

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

    const std::string configurationFile = getArgDefault(argc, argv, configurationArg, "");

    asio::io_context ioContext;

    const std::string participantName = getArgDefault(argc, argv, participantNameArg, "SilKitAdapterQemu");

    const std::string registryURI = getArgDefault(argc, argv, regUriArg, "silkit://localhost:8501");

    unsigned numberOfRequestedAdaptations = 0;
    try
    {
        throwInvalidCliIf(thereAreUnknownArguments(argc, argv));

        std::shared_ptr<SilKit::Config::IParticipantConfiguration> participantConfiguration;
        if (!configurationFile.empty())
        {
            participantConfiguration = SilKit::Config::ParticipantConfigurationFromFile(configurationFile);
            static const auto conflictualArguments = {
                &logLevelArg,
                /* others are correctly handled by SilKit if one is overwritten.*/};
            for (const auto* conflictualArgument : conflictualArguments)
            {
                if (findArg(argc, argv, *conflictualArgument, argv) != NULL)
                {
                    auto configFileName = configurationFile;
                    if (configurationFile.find_last_of("/\\") != std::string::npos)
                    {
                        configFileName = configurationFile.substr(configurationFile.find_last_of("/\\") + 1);
                    }
                    std::cout << "[info] Be aware that argument given with " << *conflictualArgument
                              << " can be overwritten by a different value defined in the given configuration file "
                              << configFileName << std::endl;
                }
            }
        }
        else
        {
            const std::string loglevel = getArgDefault(argc, argv, logLevelArg, "Info");
            const std::string participantConfigurationString =
                R"({ "Logging": { "Sinks": [ { "Type": "Stdout", "Level": ")" + loglevel + R"("} ] } })";
            participantConfiguration =
                SilKit::Config::ParticipantConfigurationFromString(participantConfigurationString);
        }

        std::cout << "Creating participant '" << participantName << "' at " << registryURI << std::endl;
        std::unique_ptr<SilKit::IParticipant> p_container =
            SilKit::CreateParticipant(participantConfiguration, participantName, registryURI);
        auto participant = p_container.get();

        auto logger = participant->GetLogger();

        auto* lifecycleService = participant->CreateLifecycleService({OperationMode::Autonomous});
        auto* systemMonitor = participant->CreateSystemMonitor();

        std::promise<void> runningStatePromise;

        systemMonitor->AddParticipantStatusHandler(
            [&runningStatePromise, participantName](const SilKit::Services::Orchestration::ParticipantStatus& status) {
                if (participantName == status.participantName)
                {
                    if (status.state == SilKit::Services::Orchestration::ParticipantState::Running)
                    {
                        // Only call lifecycleservice->stop() after hitting this
                        runningStatePromise.set_value();
                    }
                }
            });

        std::vector<ChardevSocketToPubSubAdapter*> chardevSocketTransmitters;

        //set to ensure the provided sockets are unique (text-based)
        std::set<std::string> alreadyProvidedSockets;

        foreachArgDo(argc, argv, chardevArg, [&](char* arg) -> void {
            ++numberOfRequestedAdaptations;
            chardevSocketTransmitters.push_back(parseChardevSocketArgument(arg, alreadyProvidedSockets, participantName,
                                                                           ioContext, participant, logger));
        });

        std::vector<EthSocketToEthControllerAdapter*> ethernetSocketTransmitters;

        foreachArgDo(argc, argv, ethArg, [&](char* arg) -> void {
            ++numberOfRequestedAdaptations;
            ethernetSocketTransmitters.push_back(parseEthernetSocketArgument(
                arg, alreadyProvidedSockets, participantName, ioContext, participant, logger));
        });

        throwInvalidCliIf(numberOfRequestedAdaptations == 0);
        auto finalStateFuture = lifecycleService->StartLifecycle();

        std::thread t([&]() -> void {
            ioContext.run();
        });

        promptForExit();

        ioContext.stop();
        t.join();

        auto runningStateFuture = runningStatePromise.get_future();
        auto futureStatus = runningStateFuture.wait_for(15s);
        if (futureStatus != std::future_status::ready)
        {
            std::cout << "Lifecycle Service Stopping: timed out while checking if the participant is currently running.";
            promptForExit();
        }
        
        lifecycleService->Stop("Adapter stopped by the user.");

        auto finalState = finalStateFuture.wait_for(15s);
        if (finalState != std::future_status::ready)
        {
            std::cout << "Lifecycle service stopping: timed out" << std::endl;
            promptForExit();
        }
    }
    catch (const SilKit::ConfigurationError& error)
    {
        std::cerr << "Invalid configuration: " << error.what() << std::endl;
        promptForExit();
        return CONFIGURATION_ERROR;
    }
    catch (const InvalidCli&)
    {
        adapters::print_help();
        std::cerr << std::endl << "Invalid command line arguments." << std::endl;
        promptForExit();
        return CLI_ERROR;
    }
    catch (const SilKit::SilKitError& error)
    {
        std::cerr << "SIL Kit runtime error: " << error.what() << std::endl;
        promptForExit();
        return OTHER_ERROR;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Something went wrong: " << error.what() << std::endl;
        promptForExit();
        return OTHER_ERROR;
    }

    return NO_ERROR;
}
