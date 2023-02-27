# Vector SIL Kit Adapters for QEMU
This collection of software is provided to illustrate how the [Vector SIL Kit](https://github.com/vectorgrp/sil-kit/)
can be attached to running [QEMU](https://www.qemu.org/) processes.

This repository contains instructions to create, set up, and launch a QEMU image, and a minimal development environment.

The main contents are working examples of necessary software to connect the running system to a SIL Kit environment,
as well as complimentary demo applications for some communication to happen.

## Getting Started
Those instructions assume you use WSL (Ubuntu) or a Linux OS for running QEMU and building and running the adapter (nevertheless it is also possible to do this directly on a Windows system, with the exception of setting up the QEMU image), and use ``bash`` as your interactive
shell.

This section specifies steps you should do if you have just cloned the repository.

Before any of those topics, please change your current directory to the top-level in the ``sil-kit-adapters-qemu``
repository:

    cd /path/to/sil-kit-adapters-qemu

### Fetch Third Party Software
The first thing that you should do is initializing the submodules to fetch the required third party software:

    git submodule update --init --recursive

Otherwise clone the standalone version of asio manually:

    git clone --branch asio-1-18-2 https://github.com/chriskohlhoff/asio.git third_party/asio

### Build the Adapters and Demos
To build the adapters and demos, you'll need a SIL Kit package ``SilKit-x.y.z-$platform`` for your platform. You can download them directly from [Vector SIL Kit Releases](https://github.com/vectorgrp/sil-kit/releases). The easiest way would be to download it with your web browser, unzip it and place it on your Windows file system, where it also can be accessed by WSL.

The adapters and demos are built using ``cmake``:

    mkdir build
    cmake -S. -Bbuild -DSILKIT_PACKAGE_DIR=/path/to/SilKit-x.y.z-$platform/ -D CMAKE_BUILD_TYPE=Release
    cmake --build build --parallel

The adapter and demo executables will be available in ``build/bin`` (depending on the configured build directory).
Additionally the ``SilKit`` shared library (e.g., ``SilKit[d].dll`` on Windows) is copied to that directory
automatically.

### Run the SilKitAdapterQemu
This application allows the user to attach simulated ethernet interface (``nic``) and/or character devices (e.g. ``SPI``) of a QEMU virtual machine to the
SIL Kit.

The application uses the *socket* backend provided by QEMU.
It can be configured for the QEMU virtual machine using the following command line arguments of QEMU:

    -netdev socket,listen=:12345
    -chardev socket,server=on,wait=off,host=0.0.0.0,port=23456

The arguments of ``listen=`` and ``hots=``&``port=`` specifies a TCP socket endpoint on which QEMU will listen for incoming connections, 
which SilKitAdapterQemu will establish.

All *outgoing* ethernet frames on that particular virtual ethernet interface inside of the virtual machine are sent to
all connected clients.
Any *incoming* data from any connected clients is presented to the virtual machine as an incoming ethernet frame on the
virtual interface.
All characters sent to the SPI associated to the chardev will be sent to the topic specified to SilKitAdapterQemu.
All characters published on the subscribed topic by SilKitAdapterQemu will be sent to the SPI of the guest.

Before you start the adapter there always needs to be a sil-kit-registry running already. Start it e.g. like this:

    ./path/to/SilKit-x.y.z-$platform/SilKit/bin/sil-kit-registry --listen-uri 'silkit://0.0.0.0:8501'

The application takes the following command line arguments:

    SilKitAdapterQemu [--name <participant's name>]
      [--registry-uri silkit://<host>:<port>]
      [--log <Trace|Debug|Warn|Info|Error|Critical|off>]
     [[--socket-to-ethernet <host>:<port>,network=<network's name>[:<controller's name>]]]
     [[--socket-to-chardev
         <host>:<port>,
        [<namespace>::]<inbound topic name>[~<subscriber's name>]
           [[,<label key>:<optional label value>
            |,<label key>=<mandatory label value>
           ]],
        [<namespace>::]<outbound topic name>[~<publisher's name>]
           [[,<label key>:<optional label value>
            |,<label key>=<mandatory label value>
           ]]
     ]]

There needs to be at least one ``--socket-to-chardev`` or ``--socket-to-ethernet`` argument. Each socket must be unique.

**Note:** Be aware that the QEMU image needs to be running already before you start the adapter application.

## Setup QEMU image
With the following instructions you can setup your own QEMU image which can be used for the demos below: [tools/README.md](tools/README.md)

## Ethernet Demo
The aim of this demo is to showcase a simple adapter forwarding ethernet traffic from and to the QEMU image through
Vector SIL Kit. Traffic being exchanged are ping (ICMP) requests, and the answering device replies only to them.

This demo is further explained in [eth/README.md](eth/README.md).

## Chardev Demo
This demo application allows the user to attach a simulated character device (chardev) interface (pipe) of a QEMU image to the SIL Kit in the form of a DataPublisher/DataSubscriber.

This demo is further explained in [chardev/README.md](chardev/README.md).


