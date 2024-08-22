# Chardev adapter and demos

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

## Demos Common Steps

When the QEMU emulator boots the debian image, the serial devices are in ``cooked echo`` mode, which sends back input
received (the ``echo`` part) and transforms input instead of keeping it ``raw`` (the ``cooked`` part). While this is
especially useful for the interactive part of the ``-serial mon:stdio``, it is useless for the demo adapter. Indeed,
the adapter is not configured to ignore this echo which could produce extra control characters or infinite loops, so
it is advised to deactivate it. It is also advised to toggle into raw mode, because the serial driver will replace
newline characters by two characters: carriage return and line feed, as well as other behavioral problem with
accidental control codes.

So first, inside the QEMU image, setup the ``ttyS1`` pseudo file for raw mode, remove echo and maximum speed (115200 bauds):

```
root@silkit-qemu-demos-guest:~# stty raw -echo 115200 -F /dev/ttyS1
```

Then (or before) you have to setup the SIL Kit environment:

```
./path/to/SilKit-x.y.z-$platform/SilKit/bin/sil-kit-registry --listen-uri 'silkit://0.0.0.0:8501'
```


## Basic Chardev Echo
After following the common steps above, first launch the adapter, which connects to the registry (to connect to other participants) and to QEMU,
to transmit the serial data written to ``/dev/ttyS1`` inside QEMU through SIL Kit, and vice-versa:
```
./bin/sil-kit-adapter-qemu --socket-to-chardev localhost:23456,Namespace::toChardev,VirtualNetwork=Default,Instance=EchoDevice,Namespace::fromChardev,VirtualNetwork:Default,Instance:Adapter --configuration ./chardev/demos/SilKitConfig_Adapter.silkit.yaml
Creating participant 'SilKitAdapterQemu' at silkit://localhost:8501
[2022-08-31 18:06:27.674] [SilKitAdapterQemu] [info] Creating participant 'SilKitAdapterQemu' at 'silkit://localhost:8501', SIL Kit version: 4.0.19
[2022-08-31 18:06:27.790] [SilKitAdapterQemu] [info] Connected to registry at 'tcp://127.0.0.1:8501' via 'tcp://127.0.0.1:49224' (silkit://localhost:8501)
connect success
    ...
```

Then, you can start a dump of data sent through the link from the outside:
```
root@silkit-qemu-demos-guest:~# cat /dev/ttyS1&
```

Then, launch the following helping process in separate window:
``` 
./bin/sil-kit-demo-chardev-echo-device
Creating participant 'EchoDevice' at silkit://localhost:8501
[2022-08-31 18:07:03.818] [EchoDevice] [info] Creating participant 'EchoDevice' at 'silkit://localhost:8501', SIL Kit version: 4.0.19
[2022-08-31 18:07:03.935] [EchoDevice] [info] Connected to registry at 'tcp://127.0.0.1:8501' via 'tcp://127.0.0.1:49242' (silkit://localhost:8501)
Press CTRL + C to stop the process...
    ...
```

Finally, you can test sending characters to ``/dev/ttyS1`` inside QEMU, which will be received by sil-kit-adapter-qemu
out of the socket port 23456, sent to sil-kit-demo-chardev-echo-device which will send them back, before finally be resent
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

You should see output similar to the following from the sil-kit-adapter-qemu application:
```
[2023-03-30 11:17:54.376] [SilKitAdapterQemu] [debug] QEMU >> SIL Kit: message1

[2023-03-30 11:17:54.377] [SilKitAdapterQemu] [debug] SIL Kit >> QEMU: message1

[2023-03-30 11:17:57.136] [SilKitAdapterQemu] [debug] QEMU >> SIL Kit: message10

[2023-03-30 11:17:57.136] [SilKitAdapterQemu] [debug] SIL Kit >> QEMU: message10

[2023-03-30 11:17:59.695] [SilKitAdapterQemu] [debug] QEMU >> SIL Kit: message11

[2023-03-30 11:17:59.695] [SilKitAdapterQemu] [debug] SIL Kit >> QEMU: message11

```
Take note that timing and other considerations may split the message, as thus:
```
[2023-03-30 11:17:54.376] [SilKitAdapterQemu] [debug] QEMU >> SIL Kit: m
[2023-03-30 11:17:54.376] [SilKitAdapterQemu] [debug] QEMU >> SIL Kit: essage1

```

And you should see output similar to the following from the sil-kit-demo-chardev-echo-device application:
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

You need to use CANoe 17 SP3 or newer. 

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

You need to use CANoe 17 SP3 or newer.

Before you can connect CANoe to the SIL Kit network you should adapt the RegistryUri in /chardev/demos/SilKitConfig_CANoe.silkit.yaml to the IP address of your system where your sil-kit-registry is running (in case of a WSL2 Ubuntu image e.g. the IP address of Eth0).

When you will start the measurement, CANoe will subscribe only to the ``fromChardev`` topic and set itself up as
a publisher on the other, ``toChardev``.

After following the common demo steps above, first launch the adapter, which connects to the registry (to connect to other participants) and to QEMU,
to transmit the serial data written to ``/dev/ttyS1`` inside QEMU through SIL Kit, and vice-versa:
```
./bin/sil-kit-adapter-qemu --socket-to-chardev localhost:23456,Namespace::toChardev,VirtualNetwork=Default,Instance=EchoDevice,Namespace::fromChardev,VirtualNetwork:Default,Instance:Adapter --configuration ./chardev/demos/SilKitConfig_Adapter.silkit.yaml
Creating participant 'SilKitAdapterQemu' at silkit://localhost:8501
[2022-08-31 18:06:27.674] [SilKitAdapterQemu] [info] Creating participant 'SilKitAdapterQemu' at 'silkit://localhost:8501', SIL Kit version: 4.0.19
[2022-08-31 18:06:27.790] [SilKitAdapterQemu] [info] Connected to registry at 'tcp://127.0.0.1:8501' via 'tcp://127.0.0.1:49224' (silkit://localhost:8501)
connect success
    ...
```

Then, you can start a dump of data sent through the link from the outside:
```
root@silkit-qemu-demos-guest:~# cat /dev/ttyS1&
```
Finally, launch ``CANoe 17 SP3`` or newer and load the
``Qemu_Chardev_demo_CANoe_device.cfg`` from the ``demos/CANoe`` directory.

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

## Byte-oriented demos

These demos showcase binary transfer and conversion from the chardev channel to a DataPublisher-DataSubscriber pair for CANoe communication.

You need to use CANoe 17 SP3 or newer.

Before you can connect CANoe to the SIL Kit network you should adapt the ``RegistryUri`` in /chardev/demos/SilKitConfig_CANoe.silkit.yaml to the IP address of your system where your sil-kit-registry is running (in case of a WSL2 Ubuntu image e.g. the IP address of Eth0).

Then launch the adapter prepare to dialog with CANoe on its byte-oriented topics:
```
./bin/sil-kit-adapter-qemu --socket-to-chardev localhost:23456,Namespace::degrees_user,VirtualNetwork=Default,Instance=UserTemp,Namespace::degrees_sensor,VirtualNetwork:Default,Instance:SensorTemp --registry-uri silkit://localhost:8501 --log Debug
```

### Temperature control demo

In this demo, CANoe is sending a temperature value through the SPI link.

After following the common steps for all demos as well as steps for byte-oriented demo, you can setup your QEMU terminal to print the values received through the SPI link:
```
root@silkit-qemu-demos-guest:~# od -An -tu2 -w2 -v /dev/ttyS1 &
```

Then you can launch ``CANoe 17 SP3`` or newer and load the ``Qemu_Chardev_demo_CANoe_temperature_control.cfg`` from the ``demos/CANoe`` directory. When you will start the measurement, CANoe will set itself up as a publisher on ``degrees_user`` topic.

After starting the simulation, you can input a value in celsius in the panel named ``TemperaturePanel`` and click on send, you'll see the value being printed in the console of QEMU in Kelvins. This is what you would see if you send "27" twice, then "0" with CANoe:
```
root@silkit-qemu-demos-guest:~# od -An -tu2 -w2 -v /dev/ttyS1
  300
  300
  273
```

And here is what you'll see in the Adapter's log (note that binary values are not always printable):
```
[2023-08-03 17:26:50.875] [SilKitAdapterQemu] [debug] SIL Kit >> QEMU: ,☺
[2023-08-03 17:26:51.031] [SilKitAdapterQemu] [debug] SIL Kit >> QEMU: ,☺
[2023-08-03 17:26:53.571] [SilKitAdapterQemu] [debug] SIL Kit >> QEMU: ◄☺
```

### Temperature sensing demo

In this demo, CANoe is receiving a temperature value through the SPI link.

After following the common steps for all demos as well as steps for byte-oriented demo, you may setup the following bash function to send binary conversion of a given temperature value :
```
root@silkit-qemu-demos-guest:~# send () { printf $(printf '\%03o' $(($1&255)) $(($1>>8&255))) > /dev/ttyS1; }
```

This way, calling e.g. ``send 333`` from qemu console will send number ``333`` through the link by splitting it in MSB and LSB, printing octal commands with a first ``printf``, whose result will be interpreted by a second ``printf``.

Then you can launch ``CANoe 17 SP3`` or newer and load the ``Qemu_Chardev_demo_CANoe_temperature_sensing.cfg`` from the ``demos/CANoe`` directory. When you will start the measurement, CANoe will subscribe to the ``degrees_sensor`` topic.

When you start the simulation, the value in the panel named ``TemperaturePanel`` will reflect what being typed in the console of QEMU in Kelvins. For instance if you type the following commands:
```
root@silkit-qemu-demos-guest:~# send 273
root@silkit-qemu-demos-guest:~# send 300
```
CANoe would show "0" and "27" in the panel.

And here is what you'll see in the Adapter's log (note that binary values are not always printable):
```
[2023-08-03 17:39:59.938] [SilKitAdapterQemu] [debug] QEMU >> SIL Kit: ◄☺
[2023-08-03 17:40:02.137] [SilKitAdapterQemu] [debug] QEMU >> SIL Kit: ,☺
```

Take note that timing and other considerations may split the message.
