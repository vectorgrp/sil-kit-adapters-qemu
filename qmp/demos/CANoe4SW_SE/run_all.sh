#!/bin/bash
scriptDir=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
silKitDir=/home/vector/SilKit/SilKit-4.0.50-ubuntu-18.04-x86_64-gcc/
# if "exported_full_path_to_silkit" environment variable is set (in pipeline script), use it. Otherwise, use default value
silKitDir="${exported_full_path_to_silkit:-$silKitDir}"

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
$silKitDir/SilKit/bin/sil-kit-registry --listen-uri 'silkit://0.0.0.0:8501' -s &> $scriptDir/sil-kit-registry.out &
sleep 1 # wait 1 second for the creation/existense of the .out file
timeout 30s grep -q 'Registered signal handler' <(tail -f $scriptDir/sil-kit-registry.out) || { echo "[error] Timeout reached while waiting for sil-kit-registry to start"; exit 1; }

echo "[info] Starting QEMU image"
tmp_fifo3=$(mktemp -u)
tmp_fifo4=$(mktemp -u)
mkfifo $tmp_fifo3 $tmp_fifo4
exec 3<>$tmp_fifo3
exec 4<>$tmp_fifo4
rm $tmp_fifo3 $tmp_fifo4

{ $scriptDir/../../../tools/run-silkit-qemu-demos-guest.sh | tee $scriptDir/run-silkit-qemu-demos-guest.out; } <&3 >&4 &

# set a timer for starting the QEMU image
echo "[info] Waiting for initialization"
timer=180 #seconds
while [ $timer -ne 0 ]
do
    sleep 1
    if grep -q "silkit-qemu-demos-guest login:" $scriptDir/run-silkit-qemu-demos-guest.out; then
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

$scriptDir/../../../bin/sil-kit-adapter-qemu --socket-to-ethernet localhost:12345,network=qemu_demo --socket-to-chardev localhost:4444,Namespace::toQMP,VirtualNetwork=Default,Instance=CANoe,Namespace::fromQMP,VirtualNetwork:Default,Instance:Adapter --configuration ./qmp/demos/SilKitConfig_Adapter.silkit.yaml &> $scriptDir/sil-kit-adapter-qemu.out &

echo "[Info] Ping 192.168.7.34" 
echo "ping 192.168.7.34" >&3

$scriptDir/run.sh
#capture returned value of run.sh script
exit_status=$?

echo "[info] Stopping QEMU image"
echo "shutdown now" >&3
sleep 10

#exit run_all.sh with same exit_status
exit $exit_status
