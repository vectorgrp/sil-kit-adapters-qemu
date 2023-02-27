# Chardev adapter and demo

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

## Demos Commons Steps

When the QEMU emulator boots the debian image, the serial devices are in ``cooked echo`` mode, which sends back input
received (the ``echo`` part) and transforms input instead of keeping it ``raw`` (the ``cooked`` part). While this is
especially useful for the interactive part of the ``-serial mon:stdio``, it is useless for the demo adapter. Indeed,
the adapter is not configured to ignore this echo which could produce extra control characters or infinite loops, so
it is advised to deactivate it. It is also advised to toggle into raw mode, because the serial driver will replace
newline characters by two characters: carriage return and line feed, as well as other behavioral problem with
accidental control codes.

So first, inside the QEMU image, setup the ``ttyS1`` pseudo file for raw mode and remove echo:
```
root@silkit-qemu-demos-guest:~# stty raw -echo -F /dev/ttyS1
```

Then, you can start a dump of data sent through the link from the outside:
```
root@silkit-qemu-demos-guest:~# cat /dev/ttyS1&
```

Then (or before) you have to setup the SIL Kit environment:
```
wsl$ ./path/to/SilKit-x.y.z-$platform/SilKit/bin/sil-kit-registry --listen-uri 'silkit://0.0.0.0:8501'
```

## Basic Chardev Echo
After following the common steps above, launch the following two processes in separate windows:

``` 
wsl$ ./build/bin/SilKitAdapterQemu --socket-to-chardev localhost:23456,Namespace::qemuInbound,VirtualNetwork=Default,Instance=EchoDevice,Namespace::qemuOutbound,VirtualNetwork:Default,Instance:Adapter --log Debug
Creating participant 'SilKitAdapterQemu' at silkit://localhost:8501
[2022-08-31 18:06:27.674] [SilKitAdapterQemu] [info] Creating participant 'SilKitAdapterQemu' at 'silkit://localhost:8501', SIL Kit version: 4.0.7
[2022-08-31 18:06:27.790] [SilKitAdapterQemu] [info] Connected to registry at 'tcp://127.0.0.1:8501' via 'tcp://127.0.0.1:49224' (silkit://localhost:8501)
connect success
    ...

wsl$ ./build/bin/SilKitDemoChardevEchoDevice
Creating participant 'ChardevDevice' at silkit://localhost:8501
[2022-08-31 18:07:03.818] [ChardevDevice] [info] Creating participant 'ChardevDevice' at 'silkit://localhost:8501', SIL Kit version: 4.0.7
[2022-08-31 18:07:03.935] [ChardevDevice] [info] Connected to registry at 'tcp://127.0.0.1:8501' via 'tcp://127.0.0.1:49242' (silkit://localhost:8501)
Press enter to stop the process...
    ...
```

Finally, you can test sending characters to ``/dev/ttyS1`` inside QEMU, which will be received by SilKitAdapterQemu
out of the socket port 23456, sent to SilKitDemoChardevEchoDevice which will send them back, before finally be resent
through the QEMU socket connection and you'll see them printed by the ``cat`` command launched during the setup.

If you chose not to use SSH to read what is incoming from ``ttyS1``, it is recommended to make the terminal pause to
allow the message to roundtrip through the SIL Kit:
```
root@silkit-qemu-demos-guest:~# echo message1 > /dev/ttyS1 ; sleep 0.1
message1
root@silkit-qemu-demos-guest:~# echo message10 > /dev/ttyS1 ; sleep 0.1
message10
root@silkit-qemu-demos-guest:~# echo message11 > /dev/ttyS1 ; sleep 0.1
message11
```

You should see output similar to the following from the SilKitAdapterQemu application:
```
QEMU >> SIL Kit: message1
SIL Kit >> QEMU: message1
QEMU >> SIL Kit: message10
SIL Kit >> QEMU: message10
QEMU >> SIL Kit: message11
SIL Kit >> QEMU: message11
```


And you should see output similar to the following from the SilKitDemoChardevEchoDevice application:
```
SIL Kit >> SIL Kit: message1
SIL Kit >> SIL Kit: message10
SIL Kit >> SIL Kit: message11
```

In the following diagram you can see the whole setup. It illustrates the data flow going through each component involved.

```
+--[ QEMU ]--+                                  SIL Kit  topics:
| Debian  11 |                            
|   ttyS1    |                                  > qemuOutbound >  
+------------+                                 ------------------
     |             +------[SIL Kit]------+    /                  \      +----[SIL Kit]----+
      \____________|  SilKitAdapterQemu  |----                    ------|  ChardevDevice  |
      < socket >   +---------------------+    \                  /      +-----------------+
        23456                                  ------------------
                                                < qemuInbound < 
```

Please note that you can compile and run the demos on Windows even if QEMU is running in WSL.

### Using the demo with CANoe

You can also start ``CANoe 16 SP3`` or newer and load the ``Qemu_Chardev_demo_CANoe_observer.cfg`` from the ``CANoe`` directory 
and start the measurement after setting the demo as above. Note that if necessary, you must provide the associated 
``Datasource_observer.vcdl`` file to CANoe.

CANoe's Publisher/Subscriber counterpart are Distributed Objects. By nature, they are meant to convey their state to and from
CANoe, but not simultaneously. Therefore, the CANoe demo will contain 2 such objects in order to demonstrate the
observation capability, but the stimulation capability will be demonstrated in a future demo.

Here is a small drawing to illustrate how CANoe observes the topics (the observation happens in SIL Kit's network):
```
      CANoe Observation
                   \
+----[SIL Kit]----+ \        > qemuOutbound >        +----[SIL Kit]----+
|                 |__)_______________________________|                 |
| ChardevAdapter  |                                  |  ChardevDevice  |    
|                 |__________________________________|                 |
|                 |          < qemuInbound <       / |                 |
+-----------------+                               (  +-----------------+
                                                   \
                                                CANoe Observation
```
