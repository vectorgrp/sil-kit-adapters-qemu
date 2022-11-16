# Time Sync Plugin

This TCG Plugin offers a basic time synchronization interface to QEMU which can be used to synchronize simulation
participants. This is done by running QEMU in icount mode where 1 instruction takes 1 nanosecond to execute. This is
enforced by passing the QEMU commandline argument -icount shift=0. The TCG Plugin interface offers the possibility to
register a callback whenever a translation block is executed. Inside this callback we then halt the guest whenever n
instructions have been executed and wait for the external simulation participant to signal that the guest can continue.

## Prerequisites

Version of QEMU on Ubuntu does not come with plugin support. You need to download the sources and compile them with:
```
cd third_party
git clone https://github.com/qemu/qemu.git -b v7.0.0
cd qemu 
mkdir build ; cd build
../configure --target-list=x86_64-softmmu --enable-plugins --enable-kvm
ninja
```

## Usage of the Time Sync Plugin

Adding the following QEMU command line arguments is necessary:

```
-L <pathToQemuSources>/pc-bios/
-icount 0
-plugin <pathTo>/libTimeSyncPlugin.so,HostToGuestPipe=/tmp/HostToGuestPipe,GuestToHostPipe=/tmp/GuestToHostPipe,TimeinMicroseconds=1000
```

You are able to run the `run-silkit-qemu-demos-guest` script with the `--with-time-sync` argument to do this.

## Basic Sync

In the simulation participant that controls the execution, open the corresponding pipes specified in the QEMU command line argument (/tmp/HostToGuestPipe, /tmp/GuestToHostPipe). 
Next read exactly two bytes that the plugin writes to the GuestToHostPipe indicating that the translation and execution are finished.
Now is the time to synchronize your simulation by signaling the guest to continue execution by writing exactly one character to the HostToGuestPipe.

## Helpful links

* https://qemu.readthedocs.io/en/latest/devel/tcg-plugins.html (About TCG Plugins)
* https://github.com/qemu/qemu/blob/master/contrib/plugins (Already written TCG Plugins)
* https://patchwork.kernel.org/project/qemu-devel/patch/1513724822-25232-1-git-send-email-nutarojj@ornl.gov/ (Latest version of a QEMU patch that offers a similar functionality)
* https://qemu.readthedocs.io/en/latest/devel/tcg-icount.html (About TCG instruction counting)
* https://www.qemu.org/docs/master/system/qemu-manpage.html (QEMU invocation options including -icount)



