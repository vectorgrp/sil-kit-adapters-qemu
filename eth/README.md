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
(``01:23:45:67:89:ab``) and IPv4 address (``192.168.7.35``).

# Running the Demos

## Running the Demo Applications

Now is a good point to start the ``sil-kit-registry``, ``SilKitAdapterQemu`` - which connects the QEMU virtual ethernet
interface with the SIL Kit - and the ``SilKitDemoEthernetIcmpEchoDevice`` in separate terminals:

    ./path/to/SilKit-x.y.z-$platform/SilKit/bin/sil-kit-registry --listen-uri 'silkit://0.0.0.0:8501'
    
    ./build/bin/SilKitAdapterQemu --socket-to-ethernet localhost:12345,network=qemu_demo --configuration ./eth/demos/SilKitConfig_Adapter.silkit.yaml
    
    ./build/bin/SilKitDemoEthernetIcmpEchoDevice
    
The demo applications will produce output when they send and receive Ethernet frames from QEMU or the Vector SIL Kit.

**Note:** You can compile and run the demos on Windows even if QEMU is running in WSL.

## ICMP Ping and Pong
When the virtual machine boots, the network interface created for hooking up with the Vector SIL Kit (``silkit0``) is ``up``.
It automatically assigns the static IP ``192.168.7.2/24`` to the interface.

Apart from SSH you can also log into the QEMU guest with the user ``root`` with password ``root``.

Then ping the demo device:

    root@silkit-qemu-demos-guest:~# ping 192.168.7.35

The ping requests should all receive responses.

You should see output similar to the following from the ``SilKitAdapterQemu`` application:

    ...
    [2023-02-21 16:49:16.977] [SilKitAdapterQemu] [debug] SIL Kit >> Demo: ACK for ETH Message with transmitId=2
    [2023-02-21 16:49:16.977] [SilKitAdapterQemu] [debug] QEMU >> SIL Kit: Ethernet frame (98 bytes, txId=2)
    [2023-02-21 16:49:16.985] [SilKitAdapterQemu] [debug] SIL Kit >> QEMU: Ethernet frame (98 bytes)
    ...

    
And output similar to the following from the ``SilKitDemoEthernetIcmpEchoDevice`` application:

    ...
    SIL Kit >> Demo: Ethernet frame (98 bytes)
    EthernetHeader(destination=EthernetAddress(01:23:45:67:89:ab),source=EthernetAddress(52:54:56:53:4b:51),etherType=EtherType::Ip4)
    Ip4Header(totalLength=84,identification=14418,dontFragment=1,moreFragments=0,fragmentOffset=0,timeToLive=64,protocol=Ip4Protocol::ICMP,checksum=29409,sourceAddress=192.168.7.2,destinationAddress=192.168.7.35) + 64 bytes payload
    Icmp4Header(type=Icmp4Type::EchoRequest,code=,checksum=15189) + 60 bytes payload
    Reply: EthernetHeader(destination=EthernetAddress(52:54:56:53:4b:51),source=EthernetAddress(01:23:45:67:89:ab),etherType=EtherType::Ip4)
    Reply: Ip4Header(totalLength=84,identification=14418,dontFragment=1,moreFragments=0,fragmentOffset=0,timeToLive=64,protocol=Ip4Protocol::ICMP,checksum=29409,sourceAddress=192.168.7.35,destinationAddress=192.168.7.2)
    Reply: Icmp4Header(type=Icmp4Type::EchoReply,code=,checksum=15189)
    SIL Kit >> Demo: ACK for ETH Message with transmitId=2
    Demo >> SIL Kit: Ethernet frame (98 bytes, txId=2)
    ...


## Adding CANoe (16 SP3 or newer) as a participant
If CANoe is connected to the SIL Kit, all ethernet traffic is visible there as well. You can also execute a test unit which checks if the ICMP Ping and Pong is happening as expected.

Before you can connect CANoe to the SIL Kit network you should adapt the ``RegistryUri`` in ``demos/SilKitConfig_CANoe.silkit.yaml`` to the IP address of your system where your sil-kit-registry is running (in case of a WSL Ubuntu image e.g. the IP address of Eth0). The configuration file is referenced by both following CANoe use cases (Desktop Edition and Server Edition).

### CANoe Desktop Edition
Load the ``Qemu_Ethernet_adapter_CANoe.cfg`` from the ``demos/CANoe`` directory and start the measurement. Optionally you can also start the test unit execution of included test configuration. While the demo is running these tests should be successful.

### CANoe4SW Server Edition (Windows)
You can also run the same test set with ``CANoe4SW SE`` by executing the following powershell script ``demos/CANoe4SW_SE/run.ps1``. The test cases are executed automatically and you should see a short test report in powershell after execution.

### CANoe4SW Server Edition (Linux)
You can also run the same test set with ``CANoe4SW SE (Linux)``. At first you have to execute the powershell script ``demos/CANoe4SW_SE/createEnvForLinux.ps1`` on your windows system by using tools of ``CANoe4SW SE (Windows)`` to prepare your test environment for Linux. In ``demos/CANoe4SW_SE/run.sh`` you should set ``canoe4sw_se_install_dir`` to the path of your ``CANoe4SW SE`` installation in your WSL. Afterwards you can execute ``demos/CANoe4SW_SE/run.sh`` in your WSL. The test cases are executed automatically and you should see a short test report in your terminal after execution.


