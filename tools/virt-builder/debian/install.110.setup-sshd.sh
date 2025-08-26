#!/bin/sh
# SPDX-FileCopyrightText: Copyright 2025 Vector Informatik GmbH
# SPDX-License-Identifier: MIT

# generate host keys
dpkg-reconfigure openssh-server

# allow login with password to root
sed -i -e 's/^[ \t#]*PermitRootLogin[ \t].*$/PermitRootLogin yes/' /etc/ssh/sshd_config

# enable and start the ssh service
systemctl enable ssh
systemctl restart ssh

