# SPI adapter and demo

This demo application allows the user to attach a simulated SPI interface (``pipe``) of a QEMU virtual machine to the
SIL Kit in the form of a DataPublisher/DataSubscriber.

The demo uses the *chardev* backend provided by QEMU, with a *socket* frontend.
It can be configured for the QEMU virtual machine using such following command line arguments of QEMU:

```
-chardev socket,id=ttySPI,port=23456
-device isa-serial,chardev=ttySPI,id=serialSilKit,index=1\
```

Please note that use of QEMU in ``-nographics`` is however in competition with this, as it uses ``ttyS0``. You need to
keep ``-serial mon:stdio`` in the arguments before the ``-device isa-serial`` to be able to interact with the system
when QEMU boots it.

## Basic SPI Echo

When the QEMU emulator boots the debian image, the serial devices are in ``cooked echo`` mode, which sends back input
received (the ``echo`` part) and transforms input instead of keeping it ``raw`` (the ``cooked`` part). While this is
especially useful for the interactive part of the ``-serial mon:stdio``, it is useless for the demo adapter. Indeed,
the adapter is not configured to ignore this echo which could produce extra control characters or infinite loops, so
it is advised to deactivate it. It is also advised to toggle into raw mode, because the serial driver will replace
newline characters by two characters: carriage return and line feed, as well as other behaviorial problem with
accidental control codes.

So first, inside the QEMU image, setup the ``ttyS1`` pseudofile for raw mode and remove echo:
```
root@silkit-qemu-demos-guest:~# stty raw -echo -F /dev/ttyS1
```

Then, you can start a dump of data sent through the link from the outside:
```
root@silkit-qemu-demos-guest:~# cat /dev/ttyS1&
```

Alternatively you may also start the dump into a second terminal by using SSH through the port 10022:
```
wsl$ ssh -p10022 -oStrictHostKeyChecking=no -oUserKnownHostsFile=/dev/null root@localhost
Warning: Permanently added '[localhost]:10022' (ECDSA) to the list of known hosts.
root@localhost's password:
Linux silkit-qemu-demos-guest 5.10.0-9-amd64 #1 SMP Debian 5.10.70-1 (2021-09-30) x86_64

The programs included with the Debian GNU/Linux system are free software;
the exact distribution terms for each program are described in the
individual files in /usr/share/doc/*/copyright.

Debian GNU/Linux comes with ABSOLUTELY NO WARRANTY, to the extent
permitted by applicable law.
Last login: Wed Oct  5 10:11:29 2022 from 10.0.2.2
root@silkit-qemu-demos-guest:~# cat /dev/ttyS1
```

Then (or before) you have to setup the SIL Kit environment:
```
wsl$ ./path/to/vib/4.0.5/SilKit/bin/sil-kit-registry --listen-uri 'silkit://127.0.0.1:8501'
    
wsl$ ./build/bin/SilKitAdapterQemuSpi
Creating participant 'SPIAdapter' at silkit://localhost:8501
[2022-08-31 18:06:27.674] [SPIAdapter] [info] Creating participant 'SPIAdapter' at 'silkit://localhost:8501', SIL Kit version: 4.0.7
[2022-08-31 18:06:27.790] [SPIAdapter] [info] Connected to registry at 'tcp://127.0.0.1:8501' via 'tcp://127.0.0.1:49224' (silkit://localhost:8501)
connect success
    ...
    
wsl$ ./build/bin/SilKitDemoSpiEchoDevice
Creating participant 'SPIDevice' at silkit://localhost:8501
[2022-08-31 18:07:03.818] [SPIDevice] [info] Creating participant 'SPIDevice' at 'silkit://localhost:8501', SIL Kit version: 4.0.7
[2022-08-31 18:07:03.935] [SPIDevice] [info] Connected to registry at 'tcp://127.0.0.1:8501' via 'tcp://127.0.0.1:49242' (silkit://localhost:8501)
Press enter to stop the process...
    ...
```

Finally, you can test sending characters to ``/dev/ttyS1`` inside QEMU, which will be received by SilKitAdapterQemuSpi
out of the socket port 23456, sent to SilKitDemoSpiEchoDevice which will send them back, before finally be resent
through the QEMU socket connection and you'll see them printed by the ``cat`` command launched during the setup.

If you chose not to use SSH to read what is incoming from ``ttyS1``, it is recommended to make the terminal pause to
allow the message to backtrip through the SIL Kit:
```
root@silkit-qemu-demos-guest:~# echo message1 > /dev/ttyS1 ; sleep 0.1
message1
root@silkit-qemu-demos-guest:~# echo message10 > /dev/ttyS1 ; sleep 0.1
message10
root@silkit-qemu-demos-guest:~# echo message11 > /dev/ttyS1 ; sleep 0.1
message11
```

You should see output similar to the following from the SilKitAdapterQemuSpi application:
```
QEMU >> SIL Kit: message1
SIL Kit >> QEMU: message1
QEMU >> SIL Kit: message10
SIL Kit >> QEMU: message10
QEMU >> SIL Kit: message11
SIL Kit >> QEMU: message11
```

And you shoud see output similar to the following from the SilKitAdapterQemuSpi application:
```
SIL Kit >> SIL Kit: message1
SIL Kit >> SIL Kit: message10
SIL Kit >> SIL Kit: message11
```

## Starting CANoe 16

You can also start ``CANoe 16 SP3`` and load the ``Qemu_SPI_adapter_CANoe.cfg`` from the ``CANoe`` directory and start
the measurement. Note that if necessary, you must provide the associated ``Datasource.vcdl`` file to CANoe.

The demo setup is done so that CANNoe is optional in this demo, therefore the Publisher/Subscribers matching labels are
done so that communication coming from the other participant or from CANoe are not distinguished. The "instance"
matching label is set by the ``Datasource.vcdl`` file on CANoe's side and programmatically in the participants.

Here is a small drawing to illustrate how CANoe taps and stimulate the topics:
```
+--[ QEMU ]--+           CANoe::                          CANoe::
| Debian  11 |    SPIDevice.qemuOutbound   O-   -<  SPIAdapter.qemuOutbound
|   ttyS1    |                               \ /
+------------+                         _______V________
      ^            +--[SIL Kit]--+    / >qemuOutbound> \      +--[SIL Kit]--+
      \____________| SPIAdapter  |----  < qemuInbound<  ------|  SPIDevice  |
        socket     +-------------+    \_______ ________/      +-------------+
        23456                                 ^
                         CANoe::             / \          CANoe:: 
                    SPIDevice.qemuInbound  >-   -O  SPIAdapter.qemuInbound
```

Please note that you can compile and run the demos on Windows even if QEMU is running in WSL.