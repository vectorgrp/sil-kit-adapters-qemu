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
    cmake -S. -Bbuild -DSILKIT_PACKAGE_DIR=/path/to/SilKit-x.y.z-$platform/
    cmake --build build --parallel

The adapters and demo executables will be available in ``build/bin`` (depending on the configured build directory).
Additionally the ``SilKit`` shared library (e.g., ``SilKit[d].dll`` on Windows) is copied to that directory
automatically.

**Note:** There is a experimental adapter for the Chardev interface of QEMU. To enable the target in the CMake build system, add ``-DBUILD_QEMU_CHARDEV_ADAPTER=ON`` to the ``cmake`` command line. Like in the following command:

    cmake -S. -Bbuild -DSILKIT_PACKAGE_DIR=/path/to/SilKit-x.y.z-$platform/ -DBUILD_QEMU_CHARDEV_ADAPTER=ON

### Run the SilKitAdapterQemuEthernet
This application allows the user to attach simulated ethernet interface (``nic``) of a QEMU virtual machine to the
SIL Kit.

The application uses the *socket* backend provided by QEMU.
It can be configured for the QEMU virtual machine using the following command line argument of QEMU:

    -nic socket,listen=:12345

The argument of ``listen=`` specifies a TCP socket endpoint on which QEMU will listen for incoming connections.

All *outgoing* ethernet frames on that particular virtual ethernet interface inside of the virtual machine are sent to
all connected clients.
Any *incoming* data from any connected clients is presented to the virtual machine as an incoming ethernet frame on the
virtual interface.

Before you start the adapter there always needs to be a sil-kit-registry running already. Start it e.g. like this:

    ./path/to/SilKit-x.y.z-$platform/SilKit/bin/sil-kit-registry --listen-uri 'silkit://127.0.0.1:8501'

The application *optionally* takes the hostname and port of the configured socket as command line arguments:

    ./build/bin/SilKitAdapterQemuEthernet [hostname] [port]

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


