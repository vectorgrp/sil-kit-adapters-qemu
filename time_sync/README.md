# Time Sync Plugin

This TCG Plugin offers a basic time synchronization interface to QEMU which can be used to synchronize simulation participants. This is done by running QEMU in icount mode where 1 instruction takes 1 nanosecond to execute.
This is enforced by passing the QEMU commandline argument -icount shift=0. The TCG Plugin interface offers the possibility to register a callback whenever a translation block is
executed. Inside this callback we then halt the guest whenever n instructions have been executed and wait for the external simulation participant to signal that the guest can continue.

## Usage of the Time Sync Plugin

Add the following QEMU command line argument:

```
-plugin <pathTo>/libTimeSyncPlugin.so,HostToGuestPipe=/tmp/HostToGuestPipe,GuestToHostPipe=/tmp/GuestToHostPipe,TimeinMicroseconds=1000
```

In the simulation participant that controls the execution you need to open the corresponding pipes you specified in the QEMU command line argument (/tmp/HostToGuestPipe, /tmp/GuestToHostPipe). 
Next read exactly one character that the plugin writes to the GuestToHostPipe indicating that the execution is finished.
Now is the time to synchronize your simulation. Afterwards you signal the guest to continue execution by writing exactly one character to the HostToGuestPipe.

      char pipeHostToGuest[] = "/tmp/HostToGuestPipe";
      char pipeGuestToHost[] = "/tmp/GuestToHostPipe";
      char contString[] = "c";

      mkfifo(pipeHostToGuest, 0666);
      mkfifo(pipeGuestToHost, 0666);

      int fdHostToGuest = open(pipeHostToGuest, O_WRONLY, O_CREAT);
      int fdGuestToHost = open(pipeGuestToHost, O_RDONLY, O_CREAT);

      ssize_t bytes;

      // participant behavior
      participantController->SetPeriod(1ms);
      participantController->SetSimulationTask(
        [&](std::chrono::nanoseconds now, std::chrono::nanoseconds /*duration*/) {
        // The application reacts to CANoe messages only.

        char arr[2];
        bytes = read(fdGuestToHost, arr, sizeof(arr));

        std::this_thread::sleep_for(1ms);

        bytes = write(fdHostToGuest, &contString, strlen(contString));
      });
    
## Helpful links

* https://qemu.readthedocs.io/en/latest/devel/tcg-plugins.html (About TCG Plugins)
* https://github.com/qemu/qemu/blob/master/contrib/plugins (Already written TCG Plugins)
* https://patchwork.kernel.org/project/qemu-devel/patch/1513724822-25232-1-git-send-email-nutarojj@ornl.gov/ (Latest version of a QEMU patch that offers a similar functionality)
* https://qemu.readthedocs.io/en/latest/devel/tcg-icount.html (About TCG instruction counting)
* https://www.qemu.org/docs/master/system/qemu-manpage.html (QEMU invocation options including -icount)



