// Copyright (c) Vector Informatik GmbH. All rights reserved.

#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <cerrno>
#include <cstring>

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "silkit/SilKit.hpp"
#include "silkit/config/all.hpp"
#include "silkit/services/orchestration/all.hpp"
#include "silkit/services/orchestration/ILifecycleService.hpp"
#include "silkit/experimental/participant/ParticipantExtensions.hpp"

using namespace SilKit::Services::PubSub;
using namespace SilKit::Services::Orchestration;

using namespace std::chrono_literals;

/**************************************************************************************************
 * Main Function
 **************************************************************************************************/

int main(int argc, char**)
{
    const std::string participantConfigurationString =
        R"({ "Logging": { "Sinks": [ { "Type": "Stdout", "Level": "Info" } ] } })";

    const std::string participantName = "SPIDevice";
    const std::string registryURI = "silkit://localhost:8501";

    try
    {
        auto participantConfiguration =
            SilKit::Config::ParticipantConfigurationFromString(participantConfigurationString);

        std::cout << "Creating participant '" << participantName << "' at " << registryURI << std::endl;
        auto participant = SilKit::CreateParticipant(participantConfiguration, participantName, registryURI);

        const char pipeHostToGuest[] = "/tmp/HostToGuestPipe";
        const char pipeGuestToHost[] = "/tmp/GuestToHostPipe";
        const char contString[] = "c";

        mkfifo(pipeHostToGuest, 0666);
        mkfifo(pipeGuestToHost, 0666);

        int fdHostToGuest = open(pipeHostToGuest, O_WRONLY, O_CREAT);
        if (fdHostToGuest == -1 )
        {
          throw std::runtime_error( std::string { "Could not open Host to Guest pipe: " } + strerror( errno ) );
        }
        
        int fdGuestToHost = open(pipeGuestToHost, O_RDONLY, O_CREAT);
        if( fdGuestToHost == -1 ) {
          throw std::runtime_error( std::string { "Could not open Guest to Host pipe: " } + strerror( errno ) );
        }
        
        //timesyncService requires a SystemController to be stimulated
        SilKit::Experimental::Participant::CreateSystemController(participant.get())
            ->SetWorkflowConfiguration({{participantName}});
        //even though it's alone, the lifecycle service needs to be "Coordinated" to be stimulated by the System controller
        auto lifecycleService = participant->CreateLifecycleService({OperationMode::Coordinated});
        auto timesyncService = lifecycleService->CreateTimeSyncService();

        // participant behavior
        timesyncService
            ->SetSimulationStepHandler([&](std::chrono::nanoseconds now, std::chrono::nanoseconds /*duration*/) {
                // The application reacts to QEMU messages only.

                char arr[2];
                read(fdGuestToHost, arr, sizeof(arr)); //expect 'f' twice from the plugin

                //std::this_thread::sleep_for(1ms);

                write(fdHostToGuest, &contString, strlen(contString));
            }, 1000ns);

        lifecycleService->StartLifecycle();

        //Until it is running, logging does produce output, so let's wait:
        while (lifecycleService->State() !=SilKit::Services::Orchestration::ParticipantState::Running)
          std::this_thread::sleep_for(20ms);
        //TODO: Could be implemented using the CommReadyHandler callback and using a std::promise/std::future to block execution without burning cpu cycles

        auto help = "Enter one of 'stop', 'continue' or 'exit' (you may enter as few as one letter).";
        std::cout << help << std::endl;

        auto isSubstringOf = [](const std::string& word,const std::string& prefix) -> bool {
            return prefix.length() && word.compare(0, prefix.length(), prefix) == 0;
        };

        std::string userInput;
        while (!isSubstringOf("exit", userInput))
        {
            std::cout << "(DummySync) ";
            std::getline(std::cin, userInput);
            if (isSubstringOf("stop", userInput))
                lifecycleService->Pause(std::string{"User-requested pause."});
            else if (isSubstringOf("continue",userInput))
                lifecycleService->Continue();
            else if (!isSubstringOf("exit", userInput) && !userInput.empty() )
            {
                std::cout << "unknown command: " << userInput << std::endl
                  << help << std::endl;
            }
        }
        lifecycleService->Stop(std::string{"User-requested exit."});
        std::cout << "Press enter to stop the process..." << std::endl;
        std::cin.ignore();
    }
    catch (const SilKit::ConfigurationError& error)
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
