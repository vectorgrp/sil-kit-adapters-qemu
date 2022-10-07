#!/bin/sh

cat >/etc/network/interfaces <<EOF
# This file describes the network interfaces available on your system
# and how to activate them. For more information, see interfaces(5).

source /etc/network/interfaces.d/*

# The loopback network interface
auto lo
iface lo inet loopback

auto internet0
allow-hotplug internet0
iface internet0 inet dhcp

auto vib0
allow-hotplug vib0
iface vib0 inet static
  address 192.168.12.34
  netmask 255.255.255.0
EOF

cat >/etc/systemd/network/10-vib-qemu-demos-guest-internet0.link <<EOF
[Match]
MACAddress=00:11:22:33:44:55

[Link]
Name=internet0
EOF

cat >/etc/systemd/network/10-vib-qemu-demos-guest-vib0.link <<EOF
[Match]
MACAddress=52:54:56:53:4B:51

[Link]
Name=vib0
EOF

