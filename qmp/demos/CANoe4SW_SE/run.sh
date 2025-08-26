#!/bin/bash
# SPDX-FileCopyrightText: Copyright 2025 Vector Informatik GmbH
# SPDX-License-Identifier: MIT

PSScriptRoot=$( cd -- "$( dirname -- "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )
canoe4sw_se_install_dir="/opt/vector/canoe4sw-se"

#run tests
$canoe4sw_se_install_dir/canoe4sw-se "$PSScriptRoot/Default.venvironment" -d "$PSScriptRoot/working-dir" --verbosity-level "2" --test-unit "$PSScriptRoot/testPingViaQmp.vtestunit"  --show-progress "tree-element"

