=================================
Vector Integration Bus QEmu Demos
=================================

This is a set of demos which show how the Vector Integration Bus can be attached to QEmu processes.

The goal is to provide documentation and some examples on how to set up QEmu and the development environment.

This Demo consists of two separate components: the QEMU based guest image contains a live
Linux kernel that reacts to ICMP echo requests on its virtual network interface.
The VIB component contains a socket client that connects to the virtual QEMU network interface via its
exported socket and implements a transport to a virtual VIB Ethernet bus named "Eth1".
::
  
  +-------[ QEMU ]---------+                                +--------[ VIB ]--------+
  | Debian 11              |<== [listening socket 12345] ==>|  QemuSocketClient     |
  |   virtual NIC vib0     |                                |   <=> virtual ETH1    |
  +------------------------+                                +----------+------------+
                                                                       |
                                                      Vector CANoe <=> ´
  

Overview
========

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

The demo *optionally* takes the hostname and port of the configured socket as command line arguments.

IbDemoEthernetIcmpEchoDevice
----------------------------
This demo application implements a very simple IntegrationBus participant with a single simulated ethernet controller.
The application will reply to an ARP request and respond to ICMPv4 Echo Requests directed to it's hardcoded MAC address
(``01:23:45:67:89:ab``) and IPv4 address (``192.168.12.35``).

Building the Demos
==================
The demos are built using ``cmake`` (here with ``/path/to/vib-qemu-demos/build`` as the build-directory)::

    cd /.../vib-qemu-demos

If you cloned the repositoy please call::

    git submodule update --init --recursive

Otherwise clone the standalone version of asio manually::

    git clone --branch asio-1-18-2 https://github.com/chriskohlhoff/asio.git third_party/asio

To build the demos, you'll need VIB packages ``IntegrationBus-3.7.14-$platform`` for your platform.
Then you can build the demos::

    cmake -S. -Bbuild -DIB_DIR=/path/to/vib/IntegrationBus-3.7.14-???/
    cmake --build build --parallel

The demo executables are available in ``build/bin`` (depending on the configured build directory).
Additionally the ``IntegrationBus`` shared library (e.g. ``IntegrationBus[d]?.dll`` on Windows) is copied to that
directory automatically.

Running the QEmu and the Demos
==============================

This manual assumes you use WSL (Ubuntu) for running QEmu and use ``bash`` as your interactive shell.

First change your current directory to the top-level in the ``vib-qemu-demos`` repository::

    wsl$ cd /.../vib-qemu-demos

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

Running the Demo applications
-----------------------------

Now is a good point to start the ``IbRegistry``, ``IbDemoEthernetQemu`` - which connects the QEmu virtual ethernet
interface with the integration bus - and the ``ObDemoEthernetDevice`` in separate terminals::

    wsl$ /path/to/vib/3.7.14/IntegrationBus/lib/cmake/IntegrationBus/bin/IbRegistry
    wsl$ ./build/bin/IbDemoEthernetQemuAdapter
    wsl$ ./build/bin/IbDemoEthernetIcmpEchoDevice

Starting CANoe 16
-----------------

You can also start ``CANoe 16`` and load the ``EthernetDemoAsync.cfg`` from the ``vib-canoe-demos`` and start the
measurement.

Please note that you can compile and run the demos on Windows even if QEmu is running in WSL.

Assigning an IP to the QEmu NIC
-------------------------------

When the virtual machine boots, the network interface created for hooking up with the IntegrationBus is ``down``.
To activate it (without having an IP address assigned)::

    guest# ip link set vib0 up

And to add an IP address to the interface::

    guest# ip addr add 192.168.12.34/24 dev vib0

Then ping the demo device four times::

    guest# ping -c4 192.168.12.35

The ping requests should all receive responses.
If CANoe is connected to the integration bus, all Ethernet traffic should be visible there as well.
