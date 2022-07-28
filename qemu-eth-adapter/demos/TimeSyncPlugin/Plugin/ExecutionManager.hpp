// Copyright (c) Vector Informatik GmbH. All rights reserved.

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

class ExecutionManager
{
public:
    ExecutionManager(const std::string& hostToGuestPipe, const std::string& guestToHostPipe)
    {
        mkfifo(hostToGuestPipe.c_str(), 0666);
        fdHostToGuest = open(hostToGuestPipe.c_str(), O_RDONLY, O_CREAT );

        mkfifo(guestToHostPipe.c_str(), 0666);
        fdGuestToHost = open(guestToHostPipe.c_str(), O_WRONLY, O_CREAT );
    }

    ~ExecutionManager()
    {
        close(fdHostToGuest);
        close(fdGuestToHost);
    }

    void SendFinishedSignal() const
    {
        ssize_t bytes = write(fdGuestToHost, &finishedString, strlen(finishedString));
        qemu_plugin_outs("send finished.\n");
    }

    void WaitForContinueSignal()
    {
        char arr[2];
        qemu_plugin_outs("wait for continue signal.\n");
        ssize_t bytes = read(fdHostToGuest, &arr, sizeof(arr));
    }

private:
    int fdHostToGuest;
    int fdGuestToHost;
    char finishedString[2] = "f";
};
