=================================
Vector Integration Bus QEmu Demos
=================================

This is a set of demos which show how the Vector Integration Bus can be attached to QEmu processes.

The goal is to provide documentation and some examples on how to set up QEmu and the development environment.

Overview
========

This demo consists of two separate components: the QEMU based guest image contains a live
Linux kernel that reacts to ICMP echo requests on its virtual network interface.
The VIB component contains a socket client that connects to the virtual QEMU network interface via its
exported socket and implements a transport to a virtual VIB Ethernet bus named "Eth1".
::
  
  +-------[ QEMU ]---------+                                +--------[ VIB ]--------+
  | Debian 11              |<== [listening socket 12345] ==>|  QemuSocketClient     |
  |   virtual NIC vib0     |                                |   <=> virtual Eth1    |
  +------------------------+                                +----------+------------+
                                                                       |
                                                      Vector CANoe <=> ´

IbDemoEthernetQemuAdapter
-------------------------

This demo application allows the user to attach simulated ethernet interface (``nic``) of a QEmu virtual machine to the
IntegrationBus.

The demo uses the *socket* backend provided by QEmu.
It can be configured for the QEmu virtual machine using the following command line argument of QEmu:

::

    -nic socket,listen=:12345

The argument of ``listen=`` specifies a TCP socket endpoint on which QEmu will listen for incoming connections.

All *outgoing* ethernet frames on that particular virtual ethernet interface inside of the virtual machine are sent to
all connected clients.
Any *incoming* data from any connected clients is presented to virtual machine as an incoming ethernet frame on the
virtual interface.

The demo *optionally* takes the hostname and port of the configured socket as command line arguments::

    ./build/bin/IbDemoEthernetQemuAdapter [hostname] [port]

IbDemoEthernetIcmpEchoDevice
----------------------------
This demo application implements a very simple IntegrationBus participant with a single simulated ethernet controller.
The application will reply to an ARP request and respond to ICMPv4 Echo Requests directed to it's hardcoded MAC address
(``01:23:45:67:89:ab``) and IPv4 address (``192.168.12.35``).

Time Sync Plugin
----------------------------
This is a experimental TCG Plugin for adding time synchronization to QEMU. 
Please refer to the README found in demos/TimeSyncPlugin for more information.

To enable the target in the CMake build system, add ``-DBUILD_QEMU_TIMESYNC_PLUGIN=ON`` to the ``cmake`` command line after the ``-DIB_DIR=...`` argument.

Please note that the plugin currently requires the ``unistd.h`` header to be available.
It was tested to build successfully under Linux and WSL.

Building the Demos
==================
The demos are built using ``cmake`` (here with ``/path/to/vib-qemu-demos/build`` as the build-directory)::

    cd /path/to/vib-qemu-demos

If you cloned the repositoy please call::

    git submodule update --init --recursive

Otherwise clone the standalone version of asio manually::

    git clone --branch asio-1-18-2 https://github.com/chriskohlhoff/asio.git third_party/asio

To build the demos, you'll need VIB packages ``IntegrationBus-3.7.14-$platform`` for your platform.
Then you can build the demos::

    cmake -S. -Bbuild -DIB_DIR=/path/to/vib/IntegrationBus-3.7.14-$platform/
    cmake --build build --parallel

The demo executables are available in ``build/bin`` (depending on the configured build directory).
Additionally the ``IntegrationBus`` shared library (e.g., ``IntegrationBus[d].dll`` on Windows) is copied to that
directory automatically.

Running the QEmu and the Demos
==============================

This manual assumes you use WSL (Ubuntu) for running QEmu and use ``bash`` as your interactive shell.

First change your current directory to the top-level in the ``vib-qemu-demos`` repository::

    wsl$ cd /path/to/vib-qemu-demos

Setup your WSL host (install ``virt-builder`` and a kernel image for use by ``virt-builder``)::

    wsl$ sudo ./tools/setup-host-wsl2-ubuntu.sh

Building and Running the QEmu Image
-----------------------------------

Build the guest image and start it::

    wsl$ sudo ./tools/build-vib-qemu-demos-guest
    wsl$ sudo chmod go+rw vib-qemu-demos-guest.qcow2
    wsl$ ./tools/run-vib-qemu-demos-guest

QEmu forwards the guests SSH port on ``10022``, any interaction with the guest can then proceed via SSH::

    wsl$ ssh -p10022 -oStrictHostKeyChecking=no -oUserKnownHostsFile=/dev/null root@localhost

The password for the ``root`` user is ``root``.

.. note:: The options to SSH deactivate host-key checking for this connection, otherwise you will have to edit your
  ``known_hosts`` file if you decide to rebuild the guest image.

Running the Demo Applications
-----------------------------

Now is a good point to start the ``IbRegistry``, ``IbDemoEthernetQemu`` - which connects the QEmu virtual ethernet
interface with the integration bus - and the ``ObDemoEthernetDevice`` in separate terminals::

    wsl$ /path/to/vib/3.7.14/IntegrationBus/bin/IbRegistry
    
    wsl$ ./build/bin/IbDemoEthernetQemuAdapter
    Creating participant 'EthernetQemu' in domain 42
    [2022-05-30 09:20:46.651] [EthernetQemu] [info] Creating ComAdapter for Participant EthernetQemu, IntegrationBus-Version: 3.7.14 2022 VIB Sprint 20, Middleware: VAsio
    [2022-05-30 09:20:46.759] [EthernetQemu] [info] Connected to registry tcp://localhost:8542,
    [2022-05-30 09:20:46.762] [EthernetQemu] [info] Time provider: WallclockProvider
    [2022-05-30 09:20:46.763] [EthernetQemu] [info] Participant EthernetQemu has joined the IB-Domain 42
    Creating ethernet controller 'Eth1'
    Creating QEmu ethernet connector for 'localhost:12345'
    connect success
    ...
    
    wsl$ ./build/bin/IbDemoEthernetIcmpEchoDevice
    Creating participant 'EthernetDevice' in domain 42
    [2022-05-30 09:20:21.252] [EthernetDevice] [info] Creating ComAdapter for Participant EthernetDevice, IntegrationBus-Version: 3.7.14 2022 VIB Sprint 20, Middleware: VAsio
    [2022-05-30 09:20:21.363] [EthernetDevice] [info] Connected to registry tcp://localhost:8542,
    [2022-05-30 09:20:21.366] [EthernetDevice] [info] Time provider: WallclockProvider
    [2022-05-30 09:20:21.367] [EthernetDevice] [info] Participant EthernetDevice has joined the IB-Domain 42
    Creating ethernet controller 'Eth1'
    Press enter to stop the process...
    ...
    
The demo applications will produce output when they send and receive Ethernet frames from QEmu or the Vector Integration Bus.

Starting CANoe 16
-----------------

You can also start ``CANoe 16`` and load the ``EthernetDemoAsync.cfg`` from the ``vib-canoe-demos`` and start the
measurement.

Please note that you can compile and run the demos on Windows even if QEmu is running in WSL.

ICMP Ping and Pong
------------------

When the virtual machine boots, the network interface created for hooking up with the IntegrationBus (``vib0``) is ``up``.
It automatically assigns the static IP ``192.168.12.34/24`` to the interface.

Apart from SSH you can also log into the QEmu guest with the user ``root`` with password ``root``.

Then ping the demo device four times::

    guest# ping -c4 192.168.12.35

The ping requests should all receive responses.

You should see output similar to the following from the ``IbDemoEthernetQemuAdapter`` application::

    IB >> Demo: ACK for ETH Message with transmitId=1
    QEmu >> IB: Ethernet frame (70 bytes, txId=1)
    IB >> Demo: ACK for ETH Message with transmitId=2
    QEmu >> IB: Ethernet frame (60 bytes, txId=2)
    IB >> QEmu: Ethernet frame (60 bytes)
    IB >> Demo: ACK for ETH Message with transmitId=3
    QEmu >> IB: Ethernet frame (98 bytes, txId=3)
    IB >> QEmu: Ethernet frame (98 bytes)
    IB >> Demo: ACK for ETH Message with transmitId=4
    QEmu >> IB: Ethernet frame (98 bytes, txId=4)
    IB >> QEmu: Ethernet frame (98 bytes)
    
And output similar to the following from the ``IbDemoEthernetIcmpEchoDevice`` application::

    IB >> Demo: Ethernet frame (70 bytes)
    EthernetHeader(destination=EthernetAddress(33:33:00:00:00:02),source=EthernetAddress(52:54:56:53:4b:51),etherType=EtherType(34525))
    IB >> Demo: Ethernet frame (60 bytes)
    EthernetHeader(destination=EthernetAddress(ff:ff:ff:ff:ff:ff),source=EthernetAddress(52:54:56:53:4b:51),etherType=EtherType::Arp)
    ArpIp4Packet(operation=ArpOperation::Request,senderHardwareAddress=EthernetAddress(52:54:56:53:4b:51),senderProtocolAddress=192.168.12.34,targetHardwareAddress=EthernetAddress(00:00:00:00:00:00),targetProtocolAddress=192.168.12.35)
    Reply: EthernetHeader(destination=EthernetAddress(52:54:56:53:4b:51),source=EthernetAddress(01:23:45:67:89:ab),etherType=EtherType::Arp)
    Reply: ArpIp4Packet(operation=ArpOperation::Reply,senderHardwareAddress=EthernetAddress(01:23:45:67:89:ab),senderProtocolAddress=192.168.12.35,targetHardwareAddress=EthernetAddress(52:54:56:53:4b:51),targetProtocolAddress=192.168.12.34)
    IB >> Demo: ACK for ETH Message with transmitId=1
    Demo >> IB: Ethernet frame (60 bytes, txId=1)
    IB >> Demo: Ethernet frame (98 bytes)
    EthernetHeader(destination=EthernetAddress(01:23:45:67:89:ab),source=EthernetAddress(52:54:56:53:4b:51),etherType=EtherType::Ip4)
    Ip4Header(totalLength=84,identification=61312,dontFragment=1,moreFragments=0,fragmentOffset=0,timeToLive=64,protocol=Ip4Protocol::ICMP,checksum=45458,sourceAddress=192.168.12.34,destinationAddress=192.168.12.35) + 64 bytes payload
    Icmp4Header(type=Icmp4Type::EchoRequest,code=,checksum=1764) + 60 bytes payload
    Reply: EthernetHeader(destination=EthernetAddress(52:54:56:53:4b:51),source=EthernetAddress(01:23:45:67:89:ab),etherType=EtherType::Ip4)
    Reply: Ip4Header(totalLength=84,identification=61312,dontFragment=1,moreFragments=0,fragmentOffset=0,timeToLive=64,protocol=Ip4Protocol::ICMP,checksum=45458,sourceAddress=192.168.12.35,destinationAddress=192.168.12.34)
    Reply: Icmp4Header(type=Icmp4Type::EchoReply,code=,checksum=1764)
    IB >> Demo: ACK for ETH Message with transmitId=2
    Demo >> IB: Ethernet frame (98 bytes, txId=2)

If CANoe is connected to the integration bus, all Ethernet traffic should be visible there as well.
