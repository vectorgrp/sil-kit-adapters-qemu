#!/bin/bash
scriptDir=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
silKitDir=/home/vector/SilKit/SilKit-4.0.50-ubuntu-18.04-x86_64-gcc/
# if "exported_full_path_to_silkit" environment variable is set (in pipeline script), use it. Otherwise, use default value
silKitDir="${exported_full_path_to_silkit:-$silKitDir}"

logDir=$scriptDir/logs # define a directory for .out files
mkdir -p $logDir # if it does not exist, create it

# create a timestamp for log files
timestamp=$(date +"%Y%m%d_%H%M%S")

# cleanup trap for child processes 
trap 'kill $(jobs -p) 2>&1 &>/dev/null; ps aux | grep qemu-system-x86_64 | grep -v grep | tr -s " " | cut -d " " -f 2 | xargs -r kill; exit' EXIT SIGHUP;

if [ ! -d "$silKitDir" ]; then
    echo "The var 'silKitDir' needs to be set to actual location of your SIL Kit"
    exit 1
fi

# check if user is root
if [[ $EUID -ne 0 ]]; then
    echo "This script must be run as root / via sudo!"
    exit 1
fi

# start SIL Kit registry
$silKitDir/SilKit/bin/sil-kit-registry --listen-uri 'silkit://0.0.0.0:8501' &> $logDir/sil-kit-registry_${timestamp}.out &
sleep 1 # wait 1 second for the creation/existense of the .out file
timeout 30s grep -q 'Press Ctrl-C to terminate...' <(tail -f $logDir/sil-kit-registry_${timestamp}.out -n +1) || { echo "[error] Timeout reached while waiting for sil-kit-registry to start"; exit 1; }

echo "[info] Starting QEMU image"
tmp_fifo3=$(mktemp -u)
tmp_fifo4=$(mktemp -u)
mkfifo $tmp_fifo3 $tmp_fifo4
exec 3<>$tmp_fifo3
exec 4<>$tmp_fifo4
rm $tmp_fifo3 $tmp_fifo4

{ $scriptDir/../../../tools/run-silkit-qemu-demos-guest.sh | tee $logDir/run-silkit-qemu-demos-guest_${timestamp}.out; } <&3 >&4 &

# set a timer for starting the QEMU image
echo "[info] Waiting for initialization"
timer=180 #seconds
while [ $timer -ne 0 ]
do
    sleep 1
    if grep -q "silkit-qemu-demos-guest login:" $logDir/run-silkit-qemu-demos-guest_${timestamp}.out; then
        echo "[info] QEMU image ready to log in"
        break
    fi
    timer=$(( $timer - 1 ))
done

if [[ $timer -eq 0 ]]; then
    echo "[error] Too long to display logging info on QEMU image"
    exit 1
fi

# log in
echo "[info] Log in into QEMU image"
echo "root" >&3
sleep 1
echo "root" >&3

$scriptDir/../../../bin/sil-kit-adapter-qemu --socket-to-chardev localhost:23456,Namespace::toChardev,VirtualNetwork=Default,Instance=EchoDevice,Namespace::fromChardev,VirtualNetwork:Default,Instance:Adapter --configuration ./chardev/demos/SilKitConfig_Adapter.silkit.yaml &> $logDir/sil-kit-adapter-qemu_${timestamp}.out &
$scriptDir/../../../bin/sil-kit-demo-chardev-echo-device &> $logDir/sil-kit-demo-chardev-echo-device_${timestamp}.out &

echo "[info] Sending data to /dev/ttyS1"
echo "stty -echo raw -F /dev/ttyS1" >&3
sleep 1
echo "while true; do echo test > /dev/ttyS1; sleep 1; done &" >&3

$scriptDir/run.sh
#capture returned value of run.sh script
exit_status=$?

echo "[info] Stopping QEMU image"
echo "shutdown now" >&3
timeout 30s grep -q 'reboot: Power down' <(tail -f $logDir/run-silkit-qemu-demos-guest_${timestamp}.out -n +1) || { echo "[error] Timeout reached while waiting for the QEMU image to shut down"; exit 1; }

#exit run_all.sh with same exit_status
exit $exit_status
