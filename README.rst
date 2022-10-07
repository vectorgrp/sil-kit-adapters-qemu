=================================
Vector SIL Kit Adapters QEMU
=================================

Overview
========

This collection of software is provided to illustrate how the `Vector SIL Kit <https://github.com/vectorgrp/sil-kit/>`_
can be attached to QEMU images.

In this repository lie instructions to create, set up, and launch a QEMU image, and a minimal development environment.

In addition, it provides working examples of necessary software to connect the running system to a Software-in-the-Loop
environment, as well as demo applications for the communication to happen.

Getting Started
===============

This section specifies steps you should do if you have just cloned the repository.

Before any of those topics, please change your current directory to the top-level in the ``sil-kit-adapters-qemu`` repository::

    wsl$ cd /path/to/sil-kit-adapters-qemu

Fetching third party software
--------------------------

The first thing that you should do is initializing the submodules to fetch the required third party software::

    git submodule update --init --recursive

Otherwise clone the standalone version of asio manually::

    git clone --branch asio-1-18-2 https://github.com/chriskohlhoff/asio.git third_party/asio


Building QEMU image
-------------------

This manual assumes you use WSL (Ubuntu) for running QEMU and use ``bash`` as your interactive shell.

Setup your WSL host (install ``virt-builder`` and a kernel image for use by ``virt-builder``)::

    wsl$ sudo ./tools/setup-host-wsl2-ubuntu.sh

Build the guest image::

    wsl$ sudo ./tools/build-silkit-qemu-demos-guest
    wsl$ sudo chmod go+rw silkit-qemu-demos-guest.qcow2


Running QEMU image
------------------

To start the guest image, run::

    wsl$ ./tools/run-silkit-qemu-demos-guest

By default, the options in this script will spawn the guest system in the same terminal. The password for the ``root``
user is ``root``.

QEMU is also set up to forward the guests SSH port on ``localhost:10022``, any interaction with the guest can then proceed via SSH::

    wsl$ ssh -p10022 -oStrictHostKeyChecking=no -oUserKnownHostsFile=/dev/null root@localhost

.. note:: The options to SSH deactivate host-key checking for this connection, otherwise you will have to edit your
  ``known_hosts`` file if you decide to rebuild the guest image.

Building the Adapters and Demos
-------------------------------

To build the demos, you'll need SilKit packages ``SilKit-4.0.5-$platform`` for your platform.

The demos are built using ``cmake`` (here with ``/path/to/sil-kit-adapters-qemu/build`` as the build-directory)::

    cmake -S. -Bbuild -DSK_DIR=/path/to/vib/SilKit-4.0.5-$platform/
    cmake --build build --parallel

The demo executables will be available in ``build/bin`` (depending on the configured build directory).
Additionally the ``SilKit`` shared library (e.g., ``SilKit[d].dll`` on Windows) is copied to that
directory automatically.


Demos
=====

Ethernet Demo
-------------

The aim of this demo is to showcase a simple adapter forwarding ethernet trafic from and to the QEMU image into the
Vector SIL Kit. Traffic being exchanged are ping (ICMP) requests, and the answering device only replies to them.

This demo is further explained in `eth/README.rst<eth/README.rst>`_

Time Sync Plugin
----------------------------
This is a experimental TCG Plugin for adding time synchronization to QEMU. 
Please refer to the README found in time_sync for more information.

To enable the target in the CMake build system, add ``-DBUILD_QEMU_TIMESYNC_PLUGIN=ON`` to the ``cmake`` command line after the ``-DSK_DIR=...`` argument.

Please note that the plugin currently requires the ``unistd.h`` header to be available.
It was tested to build successfully under Linux and WSL.

SilKitAdapterQemuSpi
--------------------
This is a experimental adapter to QEMU's SPI interface. 
Please refer to the README found in spi for more information.

To enable the target in the CMake build system, add ``-BUILD_QEMU_SPI_ADAPTER=ON`` to the ``cmake`` command line after the ``-DSK_DIR=...`` argument.
