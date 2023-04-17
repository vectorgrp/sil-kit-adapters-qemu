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

Finally, launch the adapter, which connects to the registry (to connect to other partitipants) and to QEMU,
to transmit the serial data written to ``/dev/ttyS1`` inside QEMU through SIL Kit, and vice-versa:
```
wsl$ ./build/bin/SilKitAdapterQemu --socket-to-chardev localhost:23456,Namespace::toChardev,VirtualNetwork=Default,Instance=EchoDevice,Namespace::fromChardev,VirtualNetwork:Default,Instance:Adapter --log Debug
Creating participant 'SilKitAdapterQemu' at silkit://localhost:8501
[2022-08-31 18:06:27.674] [SilKitAdapterQemu] [info] Creating participant 'SilKitAdapterQemu' at 'silkit://localhost:8501', SIL Kit version: 4.0.19
[2022-08-31 18:06:27.790] [SilKitAdapterQemu] [info] Connected to registry at 'tcp://127.0.0.1:8501' via 'tcp://127.0.0.1:49224' (silkit://localhost:8501)
connect success
    ...
```

## Basic Chardev Echo
After following the common steps above, launch the following process in separate window:

``` 
wsl$ ./build/bin/SilKitDemoChardevEchoDevice
Creating participant 'EchoDevice' at silkit://localhost:8501
[2022-08-31 18:07:03.818] [EchoDevice] [info] Creating participant 'EchoDevice' at 'silkit://localhost:8501', SIL Kit version: 4.0.19
[2022-08-31 18:07:03.935] [EchoDevice] [info] Connected to registry at 'tcp://127.0.0.1:8501' via 'tcp://127.0.0.1:49242' (silkit://localhost:8501)
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
[2023-03-30 11:17:54.376] [SilKitAdapterQemu] [debug] QEMU >> SIL Kit: message1

[2023-03-30 11:17:54.377] [SilKitAdapterQemu] [debug] SIL Kit >> QEMU: message1

[2023-03-30 11:17:57.136] [SilKitAdapterQemu] [debug] QEMU >> SIL Kit: message10

[2023-03-30 11:17:57.136] [SilKitAdapterQemu] [debug] SIL Kit >> QEMU: message10

[2023-03-30 11:17:59.695] [SilKitAdapterQemu] [debug] QEMU >> SIL Kit: message11

[2023-03-30 11:17:59.695] [SilKitAdapterQemu] [debug] SIL Kit >> QEMU: message11

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
|   ttyS1    |                                  > fromChardev >  
+------------+                                 ------------------
     |             +------[SIL Kit]------+    /                  \      +----[SIL Kit]----+
      \____________|  SilKitAdapterQemu  |----                    ------|    EchoDevice   |
      < socket >   +---------------------+    \                  /      +-----------------+
        23456                                  ------------------
                                                <  toChardev  < 
```

Please note that you can compile and run the demos on Windows even if QEMU is running in WSL.

### Observing and/or testing the echo demo with CANoe

You need to use CANoe 16 SP3 or newer. 

Before you can connect CANoe to the SIL Kit network you should adapt the RegistryUri in /chardev/demos/SilKitConfig_CANoe.silkit.yaml to the IP address of your system where your sil-kit-registry is running (in case of a WSL2 Ubuntu image e.g. the IP address of Eth0). The configuration file is referenced by both following CANoe use cases (Desktop Edition and Server Edition).

CANoe's Publisher/Subscriber counterpart are Distributed Objects. By nature, they are meant to convey their state to and from
CANoe, but not simultaneously. Therefore, this CANoe demo will contain 2 such objects in order to demonstrate the
observation capability.

Here is a small drawing to illustrate how CANoe observes the topics (the observation happens in SIL Kit's network):
```
      CANoe Observation
                   \
+----[SIL Kit]----+ \        > fromChardev >         +----[SIL Kit]----+
|                 |__)_______________________________|                 |
| ChardevAdapter  |                                  |    EchoDevice   |    
|                 |__________________________________|                 |
|                 |          <  toChardev  <       / |                 |
+-----------------+                               (  +-----------------+
                                                   \
                                                CANoe Observation
```
### CANoe Desktop Edition
Load the ``Qemu_Chardev_demo_CANoe_observer.cfg`` from the ``demos/CANoe`` directory and start the measurement.
Note that if necessary, you must provide the associated ``Datasource_observer.vcdl`` file to CANoe.
#### Tests in CANoe Desktop Edition
Optionally you can also start the test unit execution of included test configuration. While the demo is running, these tests should be successful. The advised way to "run" the demo for the test to be successful while you focus on it is to execute the following commands in the QEMU image:
```
root@silkit-qemu-demos-guest:~# stty -echo raw -F /dev/ttyS1
root@silkit-qemu-demos-guest:~# while true; do echo test > /dev/ttyS1; sleep 1; done
```

### CANoe4SW Server Edition (Windows)
You can also run the same test set with ``CANoe4SW SE`` by executing the following PowerShell script ``demos/CANoe4SW_SE/run.ps1``. The test cases are executed automatically and you should see a short test report in PowerShell after execution.

### CANoe4SW Server Edition (Linux)
You can also run the same test set with ``CANoe4SW SE (Linux)``. At first you have to execute the powershell script ``demos/CANoe4SW_SE/createEnvForLinux.ps1`` on your windows system by using tools of ``CANoe4SW SE (Windows)`` to prepare your test environment for Linux. In ``demos/CANoe4SW_SE/run.sh`` you should set ``canoe4sw_se_install_dir`` to the path of your ``CANoe4SW SE`` installation in your WSL. Afterwards you can execute ``demos/CANoe4SW_SE/run.sh`` in your WSL. The test cases are executed automatically and you should see a short test report in your terminal after execution.

## Demo with CANoe interaction
This demo showcases sending data from CANoe Desktop to the serial port of QEMU.

You need to use CANoe 16 SP3 or newer.

Before you can connect CANoe to the SIL Kit network you should adapt the RegistryUri in /chardev/demos/SilKitConfig_CANoe.silkit.yaml to the IP address of your system where your sil-kit-registry is running (in case of a WSL2 Ubuntu image e.g. the IP address of Eth0).

When you will start the measurement, CANoe will subscribe only to the ``fromChardev`` topic and set itself up as
a publisher on the other, ``toChardev``.

After following the common steps above, launch ``CANoe 16 SP3`` or newer and load the
``Qemu_Chardev_demo_CANoe_device.cfg`` from the ``demos/CANoe`` directory. Note that if necessary, you must provide the
associated ``Datasource_device.vcdl`` file to CANoe.

Here is a small drawing to illustrate how CANoe is connected to QEMU:
```
+----[SIL Kit]----+          > fromChardev >         +----[SIL Kit]----+
|                 |__________________________________|                 |
| ChardevAdapter  |                                  |      CANoe      |    
|                 |__________________________________|                 |
|                 |          <  toChardev  <         |                 |
+-----------------+                                  +-----------------+
```

Similarly to the previous demo, any text sent to the ``/dev/ttyS1`` file inside QEMU will show up in 
CANoe as data in the small widget for ``fromChardev``. While text inputted in CANoe's ``toChardev``
field will be outputted by the ``cat`` inside QEMU once you press the "Send" button.