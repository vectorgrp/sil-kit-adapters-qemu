# Chardev Demos and Adapter Setup

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

When the QEMU emulator boots the Debian image, the serial devices are in ``cooked echo`` mode, which sends back input
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
/path/to/SilKit-x.y.z-$platform/SilKit/bin/sil-kit-registry --listen-uri 'silkit://0.0.0.0:8501'
```


## Basic Chardev Echo
After following the common steps above, first launch the adapter, which connects to the registry (to connect to other participants) and to QEMU,
to transmit the serial data written to ``/dev/ttyS1`` inside QEMU through SIL Kit, and vice-versa:
```
./bin/sil-kit-adapter-qemu --socket-to-chardev localhost:23456,Namespace::toChardev,VirtualNetwork=Default,Instance=EchoDevice,Namespace::fromChardev,VirtualNetwork:Default,Instance:Adapter --configuration ./chardev/demos/SilKitConfig_Adapter.silkit.yaml
Creating participant 'SilKitAdapterQemu' at silkit://localhost:8501
[date time] [SilKitAdapterQemu] [info] Creating participant 'SilKitAdapterQemu' at 'silkit://localhost:8501', SIL Kit version: 4.0.19
[date time] [SilKitAdapterQemu] [info] Connected to registry at 'tcp://127.0.0.1:8501' via 'tcp://127.0.0.1:49224' (silkit://localhost:8501)
connect success
    ...
```

Then, you can start a dump of data sent through the link from the outside:
```
root@silkit-qemu-demos-guest:~# cat /dev/ttyS1&
```

Then, launch the following helping process in a separate window:
``` 
./bin/sil-kit-demo-chardev-echo-device
Creating participant 'EchoDevice' at silkit://localhost:8501
[date time] [EchoDevice] [info] Creating participant 'EchoDevice' at 'silkit://localhost:8501', SIL Kit version: 4.0.19
[date time] [EchoDevice] [info] Connected to registry at 'tcp://127.0.0.1:8501' via 'tcp://127.0.0.1:49242' (silkit://localhost:8501)
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
[date time] [SilKitAdapterQemu] [debug] Adapter >> SIL Kit: message1
[date time] [SilKitAdapterQemu] [debug] SIL Kit >> Adapter: message1
[date time] [SilKitAdapterQemu] [debug] Adapter >> SIL Kit: message10
[date time] [SilKitAdapterQemu] [debug] SIL Kit >> Adapter: message10
[date time] [SilKitAdapterQemu] [debug] Adapter >> SIL Kit: message11
[date time] [SilKitAdapterQemu] [debug] SIL Kit >> Adapter: message11

```
Take note that timing and other considerations may split the message, as thus:
```
[date time] [SilKitAdapterQemu] [debug] Adapter >> SIL Kit: m
[date time] [SilKitAdapterQemu] [debug] Adapter >> SIL Kit: essage1

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

**Note 1:** You can compile and run the demos on Windows even if QEMU is running in WSL.

**Note 2:** If you want to use UNIX domain sockets instead of TCP sockets for the QEMU network backend, the adapter can be started as follows

```
./bin/sil-kit-adapter-qemu --unix-socket-to-chardev PATH,Namespace::toChardev,VirtualNetwork=Default,Instance=EchoDevice,Namespace::fromChardev,VirtualNetwork:Default,Instance:Adapter --configuration ./chardev/demos/SilKitConfig_Adapter.silkit.yaml
```

where PATH needs to be replaced by an actual filesystem location representing the socket address. If you are using a Linux OS, you may choose PATH=/tmp/socket. In case of a Windows system, PATH=C:\Users\MyUser\AppData\Local\Temp\qemu.socket is a possible choice. 
Note that in ``./tools/run-silkit-qemu-demos-guest.sh``, respectively ``./tools/run-silkit-qemu-demos-guest.ps1``, the line

    -chardev socket,id=ttySPI,server=on,wait=off,host=0.0.0.0,port=23456

needs to be replaced by the following line:

    -chardev socket,id=ttySPI,server=on,wait=off,path=PATH

### Observing and/or testing the echo demo with CANoe (CANoe 17 SP3 or newer)

Before you can connect CANoe to the SIL Kit network you should adapt the `RegistryUri` in `./chardev/demos/SilKitConfig_CANoe.silkit.yaml` to the IP address of your system where your sil-kit-registry is running (in case of a WSL2 Ubuntu image e.g. the IP address of Eth0). The configuration file is referenced by both following CANoe use cases (Desktop Edition and Server Edition).

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
Load the ``Qemu_Chardev_demo_CANoe_observer.cfg`` from the ``./chardev/demos/CANoe`` directory and start the measurement.
#### Tests in CANoe Desktop Edition
Optionally you can also start the test unit execution of included test configuration. While the demo is running, these tests should be successful. The advised way to "run" the demo for the test to be successful while you focus on it is to execute the following commands in the QEMU image:
```
root@silkit-qemu-demos-guest:~# stty -echo raw -F /dev/ttyS1
root@silkit-qemu-demos-guest:~# while true; do echo test > /dev/ttyS1; sleep 1; done
```

### CANoe4SW Server Edition (Windows)
You can also run the same test set with ``CANoe4SW SE`` by executing the following PowerShell script ``./chardev/demos/CANoe4SW_SE/run.ps1``. The test cases are executed automatically and you should see a short test report in PowerShell after execution.

### CANoe4SW Server Edition (Linux)
You can also run the same test set with ``CANoe4SW SE (Linux)``. At first you have to execute the PowerShell script ``./chardev/demos/CANoe4SW_SE/createEnvForLinux.ps1`` on your Windows system by using tools of ``CANoe4SW SE (Windows)`` to prepare your test environment for Linux. In ``./chardev/demos/CANoe4SW_SE/run.sh`` you should set ``canoe4sw_se_install_dir`` to the path of your ``CANoe4SW SE`` installation in your WSL. Afterwards you can execute ``./chardev/demos/CANoe4SW_SE/run.sh`` in your WSL. The test cases are executed automatically and you should see a short test report in your terminal after execution.

## Demo with CANoe interaction (CANoe 17 SP3 or newer)
This demo showcases sending data from CANoe Desktop to the serial port of QEMU.

Before you can connect CANoe to the SIL Kit network you should adapt the `RegistryUri` in `./chardev/demos/SilKitConfig_CANoe.silkit.yaml` to the IP address of your system where your sil-kit-registry is running (in case of a WSL2 Ubuntu image e.g. the IP address of Eth0).

When you will start the measurement, CANoe will subscribe only to the ``fromChardev`` topic and set itself up as
a publisher on the other, ``toChardev``.

After following the common demo steps above, first launch the adapter, which connects to the registry (to connect to other participants) and to QEMU,
to transmit the serial data written to ``/dev/ttyS1`` inside QEMU through SIL Kit, and vice-versa:
```
./bin/sil-kit-adapter-qemu --socket-to-chardev localhost:23456,Namespace::toChardev,VirtualNetwork=Default,Instance=EchoDevice,Namespace::fromChardev,VirtualNetwork:Default,Instance:Adapter --configuration ./chardev/demos/SilKitConfig_Adapter.silkit.yaml
Creating participant 'SilKitAdapterQemu' at silkit://localhost:8501
[date time] [SilKitAdapterQemu] [info] Creating participant 'SilKitAdapterQemu' at 'silkit://localhost:8501', SIL Kit version: 4.0.19
[date time] [SilKitAdapterQemu] [info] Connected to registry at 'tcp://127.0.0.1:8501' via 'tcp://127.0.0.1:49224' (silkit://localhost:8501)
connect success
    ...
```

Then, you can start a dump of data sent through the link from the outside:
```
root@silkit-qemu-demos-guest:~# cat /dev/ttyS1&
```
Finally, launch CANoe and load the
``Qemu_Chardev_demo_CANoe_device.cfg`` from the ``./chardev/demos/CANoe`` directory.

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

## Byte-oriented demos (CANoe 17 SP3 or newer)

These demos showcase binary transfer and conversion from the chardev channel to a DataPublisher-DataSubscriber pair for CANoe communication.

Before you can connect CANoe to the SIL Kit network you should adapt the ``RegistryUri`` in `./chardev/demos/SilKitConfig_CANoe.silkit.yaml` to the IP address of your system where your sil-kit-registry is running (in case of a WSL2 Ubuntu image e.g. the IP address of Eth0).

Then launch the adapter and prepare to dialog with CANoe on its byte-oriented topics:
```
./bin/sil-kit-adapter-qemu --socket-to-chardev localhost:23456,Namespace::degrees_user,VirtualNetwork=Default,Instance=UserTemp,Namespace::degrees_sensor,VirtualNetwork:Default,Instance:SensorTemp --registry-uri silkit://localhost:8501 --log Debug
```

### Temperature control demo

In this demo, CANoe is sending a temperature value through the SPI link.

After following the common steps for all demos as well as steps for byte-oriented demo, you can setup your QEMU terminal to print the values received through the SPI link:
```
root@silkit-qemu-demos-guest:~# od -An -tu2 -w2 -v /dev/ttyS1 &
```

Then you can launch CANoe and load the ``Qemu_Chardev_demo_CANoe_temperature_control.cfg`` from the ``./chardev/demos/CANoe`` directory. When you will start the measurement, CANoe will set itself up as a publisher on ``degrees_user`` topic.

After starting the simulation, you can input a value in Celsius in the panel named ``TemperaturePanel`` and click on send, you'll see the value being printed in the console of QEMU in Kelvins. This is what you would see if you send "27" twice, then "0" with CANoe:
```
root@silkit-qemu-demos-guest:~# od -An -tu2 -w2 -v /dev/ttyS1
  300
  300
  273
```

And here is what you'll see in the Adapter's log (note that binary values are not always printable):
```
[date time] [SilKitAdapterQemu] [debug] SIL Kit >> Adapter: ,☺
[date time] [SilKitAdapterQemu] [debug] SIL Kit >> Adapter: ,☺
[date time] [SilKitAdapterQemu] [debug] SIL Kit >> Adapter: ◄☺
```

### Temperature sensing demo

In this demo, CANoe is receiving a temperature value through the SPI link.

After following the common steps for all demos as well as steps for byte-oriented demo, you may setup the following bash function to send binary conversion of a given temperature value :
```
root@silkit-qemu-demos-guest:~# send () { printf $(printf '\%03o' $(($1&255)) $(($1>>8&255))) > /dev/ttyS1; }
```

This way, calling e.g. ``send 333`` from QEMU console will send number ``333`` through the link by splitting it in MSB and LSB, printing octal commands with a first ``printf``, whose result will be interpreted by a second ``printf``.

Then you can launch CANoe and load the ``Qemu_Chardev_demo_CANoe_temperature_sensing.cfg`` from the ``./chardev/demos/CANoe`` directory. When you will start the measurement, CANoe will subscribe to the ``degrees_sensor`` topic.

When you start the simulation, the value in the panel named ``TemperaturePanel`` will reflect what being typed in the console of QEMU in Kelvins. For instance if you type the following commands:
```
root@silkit-qemu-demos-guest:~# send 273
root@silkit-qemu-demos-guest:~# send 300
```
CANoe would show "0" and "27" in the panel.

And here is what you'll see in the Adapter's log (note that binary values are not always printable):
```
[date time] [SilKitAdapterQemu] [debug] Adapter >> SIL Kit: ◄☺
[date time] [SilKitAdapterQemu] [debug] Adapter >> SIL Kit: ,☺
```

Take note that timing and other considerations may split the message.
