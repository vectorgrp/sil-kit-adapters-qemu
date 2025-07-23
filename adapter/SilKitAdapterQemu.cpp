// Copyright (c) Vector Informatik GmbH. All rights reserved.

#include "SilKitAdapterQemu.hpp"

#include <iostream>
#include <thread>

#include "common/Parsing.hpp"
#include "common/Cli.hpp"
#include "common/ParticipantCreation.hpp"
#include "common/SocketToBytesPubSubAdapter.hpp"
#include "../eth/adapter/EthSocketToEthControllerAdapter.hpp"

using namespace util;
using namespace adapters;
using namespace adapters::bytes_socket;
using namespace adapters::ethernet;

const std::string adapters::ethArg = "--socket-to-ethernet";

const std::string adapters::chardevArg = "--socket-to-chardev";

const std::string adapters::unixEthArg = "--unix-socket-to-ethernet";

const std::string adapters::defaultParticipantName = "SilKitAdapterQemu";

void print_help(bool userRequested=false)
{
    std::cout << "Usage (defaults in curly braces if you omit the switch):" << std::endl
              << "sil-kit-adapter-qemu [" << participantNameArg
              << " <participant's name{SilKitAdapterQemu}>]\n"
                 "  ["
              << configurationArg
              << " <path to .silkit.yaml or .json configuration file>]\n"
                 "  ["
              << regUriArg
              << " silkit://<host{localhost}>:<port{8501}>]\n"
                 "  ["
              << logLevelArg
              << " <Trace|Debug|Warn|{Info}|Error|Critical|off>]\n"
                 " [["
              << ethArg
              << " <host>:<port>,network=<network's name>[:<controller's name>]]]\n"
                 " [["
              << unixEthArg
              << " <path>,network=<network's name>[:<controller's name>]]]\n"
                 " [["
              << chardevArg
              << "\n"
              << SocketToBytesPubSubAdapter::printArgumentHelp("    ")
              << " ]]\n"
                 "\n"
                 "There needs to be at least one "
              << chardevArg << " or " << ethArg << " or " << unixEthArg
              << " argument. Each socket must be unique.\n"
                 "SIL Kit-specific CLI arguments will be overwritten by the config file passed by "
              << configurationArg << ".\n";
    std::cout << "\n"
                 "Example:\n"
                 "sil-kit-adapter-qemu "
              << participantNameArg << " ChardevAdapter " << chardevArg
              << " localhost:12345,"
                 "Namespace::toChardev,VirtualNetwork=Default,"
                 "fromChardev,Namespace:Namespace,VirtualNetwork:Default\n";
    if (!userRequested)
        std::cout << "\n"
                     "Pass " << helpArg << " to get this message.\n";
};

int main(int argc, char** argv)
{
    if (findArg(argc, argv, helpArg, argv) != NULL)
    {
        ::print_help(true);
        return CodeSuccess;
    }
    
    asio::io_context ioContext;

    try
    {
        throwInvalidCliIf(thereAreUnknownArguments(argc, argv, 
            {&ethArg, &chardevArg, &regUriArg, &logLevelArg, &participantNameArg, &configurationArg, &unixEthArg},
            {&helpArg}));

        SilKit::Services::Logging::ILogger* logger;
        SilKit::Services::Orchestration::ILifecycleService* lifecycleService;
        std::promise<void> runningStatePromise;

        std::string participantName = defaultParticipantName;
        const auto participant =
            CreateParticipant(argc, argv, logger, &participantName, &lifecycleService, &runningStatePromise);

        unsigned numberOfRequestedAdaptations = 0;

        //set to ensure the provided sockets are unique (text-based)
        std::set<std::string> alreadyProvidedSockets;

        std::vector<std::unique_ptr<SocketToBytesPubSubAdapter>> chardevSocketTransmitters;

        foreachArgDo(argc, argv, chardevArg, [&](char* arg) -> void {
            ++numberOfRequestedAdaptations;
            chardevSocketTransmitters.emplace_back(SocketToBytesPubSubAdapter::parseArgument(
                arg, alreadyProvidedSockets, participantName, ioContext, participant.get(), logger));
        });

        std::vector<std::unique_ptr<EthSocketToEthControllerAdapter>> ethernetSocketTransmitters;

        foreachArgDo(argc, argv, ethArg, [&](char* arg) -> void {
            ++numberOfRequestedAdaptations;
            ethernetSocketTransmitters.emplace_back(parseEthernetSocketArgument(
                arg, alreadyProvidedSockets, participantName, ioContext, participant.get(), logger));
        });

        std::vector<EthSocketToEthControllerAdapter*> ethernetUnixSocketTransmitters;

        foreachArgDo(argc, argv, unixEthArg, [&](char* arg) -> void {
            ++numberOfRequestedAdaptations;
            ethernetUnixSocketTransmitters.emplace_back(parseEthernetUnixSocketArgument(
                arg, alreadyProvidedSockets, participantName, ioContext, participant.get(), logger));
        });

        if (numberOfRequestedAdaptations == 0)
        {
            logger->Error("No " + ethArg + ", " + unixEthArg + " or " + chardevArg + " argument, exiting.");
            throw InvalidCli();
        }
        auto finalStateFuture = lifecycleService->StartLifecycle();

        std::thread ioContextThread([&]() -> void {
            ioContext.run();
        });

        promptForExit();

        Stop(ioContext, ioContextThread, *logger, &runningStatePromise, lifecycleService, &finalStateFuture);
    }
    catch (const SilKit::ConfigurationError& error)
    {
        std::cerr << "Invalid configuration: " << error.what() << std::endl;
        return CodeErrorConfiguration;
    }
    catch (const InvalidCli&)
    {
        ::print_help();
        std::cerr << std::endl << "Invalid command line arguments." << std::endl;
        return CodeErrorCli;
    }
    catch (const SilKit::SilKitError& error)
    {
        std::cerr << "SIL Kit runtime error: " << error.what() << std::endl;
        return CodeErrorOther;
    }
    catch (const std::exception& error)
    {
        std::cerr << "Something went wrong: " << error.what() << std::endl;
        return CodeErrorOther;
    }

    return CodeSuccess;
}
