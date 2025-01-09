# Ethernet Demo with QMP remote control
This demo is based on the ethernet demo showcased within this repo as well. To keep it as simple as possible and focus on the remote control aspect of this demo, the application ``sil-kit-demo-ethernet-icmp-echo-device`` is replaced by a CANoe network node with an IPv4 address (``192.168.7.34``) as the counter part for the ping demo with the QEMU image.

The QEMU QMP interface is the key element to make it possible to remotely control the QEMU image. It enables us to send JSON like control or query commands to the QEMU image. This makes it possible to do things like restarting the SUT or sending keystrokes for typing in commands into a terminal. 

Detailed documentation regarding QMP can be found here:
- https://qemu-project.gitlab.io/qemu/interop/qemu-qmp-ref.html# 
- https://wiki.qemu.org/Documentation/QMP

To make QEMU expose the QMP interface and act as a QMP server it needs to be started with an additional CLI command (the ``run-silkit-qemu-demos-guest.sh`` script already includes this command):

    -qmp tcp:localhost:4444,server,wait=off

The option ``wait=off`` makes a client connection optional. A client can connect on the fly while QEMU is not waiting for its further execution.

On the SIL Kit Adapaters QEMU side of things the important part for connecting to QMP as a client is the following:

    --socket-to-chardev localhost:4444,Namespace::toQMP,VirtualNetwork=Default,Instance=CANoe,Namespace::fromQMP,VirtualNetwork:Default,Instance:Adapter

If CANoe should be a participant in this SIL Kit network as well, it is important to provide a .vCDL file to CANoe with matching publish/subscribe namespaces and topic names. For this particular demo the .vCDL is given in [Datasource_qmp.vcdl](demos/CANoe/Datasource_qmp.vcdl).

# Running the Demos

## Running the Demo Applications

If your QEMU image is already running, now is a good point to start the ``sil-kit-registry`` and ``sil-kit-adapter-qemu`` - which connects the QEMU virtual ethernet
interface and QMP interface with the SIL Kit - in separate terminals:

    /path/to/SilKit-x.y.z-$platform/SilKit/bin/sil-kit-registry --listen-uri 'silkit://0.0.0.0:8501'
    
    ./bin/sil-kit-adapter-qemu --socket-to-ethernet localhost:12345,network=Ethernet1 --socket-to-chardev localhost:4444,Namespace::toQMP,VirtualNetwork=Default,Instance=CANoe,Namespace::fromQMP,VirtualNetwork:Default,Instance:Adapter --configuration ./qmp/demos/SilKitConfig_Adapter.silkit.yaml    

## Interactive ICMP Ping and Pong with CANoe (CANoe 17 SP3 or newer)

Before you connect CANoe to the SIL Kit network you should adapt the ``RegistryUri`` in ``./qmp/demos/SilKitConfig_CANoe.silkit.yaml`` to the IP address of your system where your sil-kit-registry is running (in case of a WSL Ubuntu image e.g. the IP address of Eth0). The configuration file is referenced by all following CANoe use cases (Desktop Edition and Server Edition).

Load the ``Qemu_qmp_demo.cfg`` from the ``./qmp/demos/CANoe`` directory and start the measurement.

When the QEMU image boots up, the network interface created for hooking up with the Vector SIL Kit (``silkit0``) is ``up``.
It automatically assigns the static IP ``192.168.7.2/24`` to the interface.

Apart from SSH you can also log into the QEMU guest with the user ``root`` with password ``root`` directly from the terminal where you have started the image.

Type in the following command to ping the CANoe network node from the QEMU terminal:

    root@silkit-qemu-demos-guest:~# ping 192.168.7.34

The ping requests should all receive responses.

You should see output similar to the following from the ``sil-kit-adapter-qemu`` application:

    ...
    [date time] [SilKitAdapterQemu] [debug] SIL Kit >> Demo: ACK for ETH Message with transmitId=2
    [date time] [SilKitAdapterQemu] [debug] QEMU >> SIL Kit: Ethernet frame (98 bytes, txId=2)
    [date time] [SilKitAdapterQemu] [debug] SIL Kit >> QEMU: Ethernet frame (98 bytes)
    ...

In the CANoe trace window you should also be able to see the icmpv4 traffic. Ping requests coming from the QEMU SUT and replies to this requests coming from the CANoe network node.

Until this point we have not used the QMP interface at all. So lets get started with entering QMP command mode by pressing the button ``Enter Command Mode`` in the CANoe configuration ``DemoPanel``. This sends the command ``{ "execute": "qmp_capabilities" }`` to a CANoe Distributed Object which has a SIL Kit Binding and therefore is connected via SIL Kit to our SUTs QMP interface.

After this the QMP interface is ready to receive all QMP commands your QEMU version is capable of via the same communication channel. To get an idea what the supported commands are, you can press the button ``Query Commands``. All the commands listed afterwards can be executed by entering them in the ``TextToSend`` edit field in ``CANoe.toQMP`` section in the ``DemoPanel``.

Some of this commands, including a set of suitable arguments, are available as shortcuts bound to buttons on the ``DemoPanel``. Let's walk through a remote controlled ping and pong demo scenario step by step:

- Let's start with a restart of the QEMU image by pressing the ``Restart SUT`` button. The icmpv4 traffic in the CANoe trace window will stop. In the terminal you have started the QEMU image before you should see the boot-up sequence.
- You could also press the ``Screenshot SUT (PNG)`` or ``Screenshot SUT (PPM)`` button to make a screenshot of the current terminal session used by QMP. Whenever the boot-up sequence is finished you should see a login prompt there (see Note below regarding preconditions for screenshot shortcut functionality).
- If you meet the preconditions and are able to use ``Screenshot SUT (PNG)`` you will see the screenshot in the CANoe panel ``TerminalScreenshot`` automatically.
- Once you can see the login prompt press the button ``Login to SUT`` to login. With another screenshot you can double check if the login as user ``root`` was successful.
- After that press the ``Send Ping`` button. It sends the ping command and a following blank.
- After that you can send the IP address you want to ping by keystrokes. To ping CANoe from the QEMU SUT you should type in ``192.168.7.34`` by pressing the corresponding buttons. When in doubt if you have typed in the right IP address you can make a screenshot again and correct possible typos by using the ``Send Backspace`` button. Execute the ping instructions by pressing ``Send Enter``.
- Now you should see icmpv4 traffic again in the CANoe trace window.
- After pressing the button ``Send Ctrl + C`` this traffic will stop happening.

**Note:** The screenshot option for the PNG file format needs QEMU version 7.1 or newer. Only with the PNG file format the panel ``TerminalScreenshot`` in CANoe can be used. Screenshots made with the PPM file format can be opened with Visual Studio Code + a PPM Plugin (e.g. PBM/PPM/PGM viewer for VSC) or manually with a dedicated image software. Both screenshot shortcuts assume that you have read and write access to drive ``D:`` on your Windows host system and running the QEMU image in WSL. If this is not the case the QMP calls in the CAPL file have to be adapted.


## Automated ICMP Ping and Pong with CANoe (CANoe 17 SP3 or newer)
What has been done in an interactive manner before can also be done automated by a test set which sends out QMP commands and checks the ethernet traffic if it matches the test verdict. While the QEMU image and the adapter are running these tests should be successful.

### CANoe Desktop Edition
Load the ``Qemu_qmp_demo.cfg`` from the ``./qmp/demos/CANoe`` directory and start the measurement if not already running. Start the execution of the test set in the included test configuration.

### CANoe4SW Server Edition (Windows)
You can also run the same test set with ``CANoe4SW SE`` by executing the following PowerShell script ``./qmp/demos/CANoe4SW_SE/run.ps1``. The test cases are executed automatically and you should see a short test report in PowerShell after execution.

### CANoe4SW Server Edition (Linux)
You can also run the same test set with ``CANoe4SW SE (Linux)``. At first you have to execute the PowerShell script ``./qmp/demos/CANoe4SW_SE/createEnvForLinux.ps1`` on your Windows system by using tools of ``CANoe4SW SE (Windows)`` to prepare your test environment for Linux. In ``./qmp/demos/CANoe4SW_SE/run.sh`` you should set ``canoe4sw_se_install_dir`` to the path of your ``CANoe4SW SE`` installation in your WSL. Afterwards you can execute ``./qmp/demos/CANoe4SW_SE/run.sh`` in your WSL. The test cases are executed automatically and you should see a short test report in your terminal after execution.


