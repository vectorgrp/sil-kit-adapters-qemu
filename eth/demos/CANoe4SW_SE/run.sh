#!/bin/bash
PSScriptRoot=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
canoe4sw_se_install_dir="/home/vector/CANoe4SW_SE_16_4_4"

#run tests
$canoe4sw_se_install_dir/canoe4sw-se "$PSScriptRoot/Default.venvironment" -d "$PSScriptRoot/working-dir" --verbosity-level "2" --test-unit "$PSScriptRoot/TestUnit.vtestunit"  --show-progress "tree-element"

