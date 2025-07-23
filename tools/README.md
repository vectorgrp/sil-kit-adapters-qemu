### Build QEMU image
Setup your WSL host:

    sudo ./tools/setup-host-wsl2-ubuntu.sh

**Note:** This script will install ``virt-builder`` and a kernel image for use by ``virt-builder``.

Build the guest image:

    sudo ./tools/build-silkit-qemu-demos-guest.sh
    sudo chmod go+rw silkit-qemu-demos-guest.qcow2


### Run QEMU image
To start the guest image, run:
- On a Linux system:
```
./tools/run-silkit-qemu-demos-guest.sh
```

- On a Windows system:
```
./tools/run-silkit-qemu-demos-guest.ps1
```

By default, the options in this script will spawn the guest system in the same terminal. The password for the ``root``
user is ``root``.

QEMU is also set up to forward the guest SSH port on ``localhost:10022``, any interaction with the guest can then
proceed via SSH from the host system:

    ssh -p10022 -oStrictHostKeyChecking=no -oUserKnownHostsFile=/dev/null root@localhost

**Note:** The options passed to SSH deactivate host-key checking for this connection, otherwise you will have to edit your
``known_hosts`` file if you decide to rebuild the guest image.