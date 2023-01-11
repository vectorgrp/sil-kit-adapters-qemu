# Ethernet Demo and Adapter Setup
This demo consists of two separate components: the QEMU based guest image contains a live
Linux kernel that reacts to ICMP echo requests on its virtual network interface.
The SIL Kit component contains a socket client that connects to the virtual QEMU network interface via its
exported socket and implements a transport to a virtual SIL Kit Ethernet bus named "qemu_demo".

    +-------[ QEMU ]---------+                                +------[ SIL Kit ]------+
    | Debian 11              |<== [listening socket 12345] ==>|  QemuSocketClient     |
    |   virtual NIC silkit0  |                                |   <=> virtual Eth1    |
    +------------------------+                                +----------+------------+
                                                                         |
                                                           Vector CANoe <=> 

## SilKitDemoEthernetIcmpEchoDevice
This demo application implements a very simple SIL Kit participant with a single simulated ethernet controller.
The application will reply to an ARP request and respond to ICMPv4 Echo Requests directed to it's hardcoded MAC address
(``01:23:45:67:89:ab``) and IPv4 address (``192.168.12.35``).

# Running the Demos

## Running the Demo Applications

Now is a good point to start the ``sil-kit-registry``, ``SilKitAdapterQemuEthernet`` - which connects the QEMU virtual ethernet
interface with the SIL Kit - and the ``SilKitDemoEthernetIcmpEchoDevice`` in separate terminals:

    ./path/to/SilKit-x.y.z-$platform/SilKit/bin/sil-kit-registry --listen-uri 'silkit://127.0.0.1:8501'
    
    ./build/bin/SilKitAdapterQemuEthernet
    Creating participant 'EthernetQemu' at silkit://localhost:8501
    [2022-08-19 16:42:59.847] [EthernetQemu] [info] Creating participant 'EthernetQemu' at 'silkit://localhost:8501', SIL Kit version: 4.0.2
    [2022-08-19 16:42:59.963] [EthernetQemu] [info] Connected to registry at 'tcp://127.0.0.1:8501' via 'tcp://127.0.0.1:59986' (silkit://localhost:8501)
    Creating ethernet controller 'EthernetQemu_Eth1'
    Creating QEMU ethernet connector for 'localhost:12345'
    connect success
    ...
    
    ./build/bin/SilKitDemoEthernetIcmpEchoDevice
    Creating participant 'EthernetDevice' at silkit://localhost:8501
    [2022-08-19 16:43:47.092] [EthernetDevice] [info] Creating participant 'EthernetDevice' at 'silkit://localhost:8501', SIL Kit version: 4.0.2
    [2022-08-19 16:43:47.213] [EthernetDevice] [info] Connected to registry at 'tcp://127.0.0.1:8501' via 'tcp://127.0.0.1:60007' (silkit://localhost:8501)
    Creating ethernet controller 'EthernetDevice_Eth1'
    Press enter to stop the process...
    ...
    
The demo applications will produce output when they send and receive Ethernet frames from QEMU or the Vector SIL Kit.

**Note:** You can compile and run the demos on Windows even if QEMU is running in WSL.

## Starting CANoe 16
You can also start ``CANoe 16 SP3`` and load the ``Qemu_Ethernet_adapter_CANoe.cfg`` from the ``CANoe`` directory and start the
measurement.

## ICMP Ping and Pong
When the virtual machine boots, the network interface created for hooking up with the Vector SIL Kit (``silkit0``) is ``up``.
It automatically assigns the static IP ``192.168.12.34/24`` to the interface.

Apart from SSH you can also log into the QEMU guest with the user ``root`` with password ``root``.

Then ping the demo device four times:

    root@silkit-qemu-demos-guest:~# ping -c4 192.168.12.35

The ping requests should all receive responses.

You should see output similar to the following from the ``SilKitAdapterQemuEthernet`` application:

    SIL Kit >> Demo: ACK for ETH Message with transmitId=1
    QEMU >> SIL Kit: Ethernet frame (98 bytes, txId=1)
    SIL Kit >> Demo: ACK for ETH Message with transmitId=2
    QEMU >> SIL Kit: Ethernet frame (98 bytes, txId=2)
    SIL Kit >> QEMU: Ethernet frame (98 bytes)
    SIL Kit >> Demo: ACK for ETH Message with transmitId=3
    QEMU >> SIL Kit: Ethernet frame (98 bytes, txId=3)
    SIL Kit >> QEMU: Ethernet frame (98 bytes)
    SIL Kit >> Demo: ACK for ETH Message with transmitId=4
    QEMU >> SIL Kit: Ethernet frame (98 bytes, txId=4)
    SIL Kit >> QEMU: Ethernet frame (98 bytes)

    
And output similar to the following from the ``SilKitDemoEthernetIcmpEchoDevice`` application:

    SIL Kit >> Demo: Ethernet frame (98 bytes)
    EthernetHeader(destination=EthernetAddress(33:33:00:00:00:02),source=EthernetAddress(52:54:56:53:4b:51),etherType=EtherType(34525))
    SIL Kit >> Demo: Ethernet frame (98 bytes)
    EthernetHeader(destination=EthernetAddress(ff:ff:ff:ff:ff:ff),source=EthernetAddress(52:54:56:53:4b:51),etherType=EtherType::Arp)
    ArpIp4Packet(operation=ArpOperation::Request,senderHardwareAddress=EthernetAddress(52:54:56:53:4b:51),senderProtocolAddress=192.168.12.34,targetHardwareAddress=EthernetAddress(00:00:00:00:00:00),targetProtocolAddress=192.168.12.35)
    Reply: EthernetHeader(destination=EthernetAddress(52:54:56:53:4b:51),source=EthernetAddress(01:23:45:67:89:ab),etherType=EtherType::Arp)
    Reply: ArpIp4Packet(operation=ArpOperation::Reply,senderHardwareAddress=EthernetAddress(01:23:45:67:89:ab),senderProtocolAddress=192.168.12.35,targetHardwareAddress=EthernetAddress(52:54:56:53:4b:51),targetProtocolAddress=192.168.12.34)
    SIL Kit >> Demo: ACK for ETH Message with transmitId=1
    Demo >> SIL Kit: Ethernet frame (98 bytes, txId=1)
    SIL Kit >> Demo: Ethernet frame (98 bytes)
    EthernetHeader(destination=EthernetAddress(01:23:45:67:89:ab),source=EthernetAddress(52:54:56:53:4b:51),etherType=EtherType::Ip4)
    Ip4Header(totalLength=84,identification=61312,dontFragment=1,moreFragments=0,fragmentOffset=0,timeToLive=64,protocol=Ip4Protocol::ICMP,checksum=45458,sourceAddress=192.168.12.34,destinationAddress=192.168.12.35) + 64 bytes payload
    Icmp4Header(type=Icmp4Type::EchoRequest,code=,checksum=1764) + 60 bytes payload
    Reply: EthernetHeader(destination=EthernetAddress(52:54:56:53:4b:51),source=EthernetAddress(01:23:45:67:89:ab),etherType=EtherType::Ip4)
    Reply: Ip4Header(totalLength=84,identification=61312,dontFragment=1,moreFragments=0,fragmentOffset=0,timeToLive=64,protocol=Ip4Protocol::ICMP,checksum=45458,sourceAddress=192.168.12.35,destinationAddress=192.168.12.34)
    Reply: Icmp4Header(type=Icmp4Type::EchoReply,code=,checksum=1764)
    SIL Kit >> Demo: ACK for ETH Message with transmitId=2
    Demo >> SIL Kit: Ethernet frame (98 bytes, txId=2)

If CANoe is connected to the Sil Kit, all Ethernet traffic should be visible there as well.
