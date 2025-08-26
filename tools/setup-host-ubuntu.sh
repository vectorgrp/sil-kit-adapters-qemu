#!/bin/bash
# SPDX-FileCopyrightText: Copyright 2025 Vector Informatik GmbH
# SPDX-License-Identifier: MIT

apt update
apt install linux-virtual libguestfs-tools -y
apt install qemu-kvm -y
