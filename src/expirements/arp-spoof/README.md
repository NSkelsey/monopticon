Introduction
------------

The goal of this expirement is to demonstrate how ARP posioning changes values
inside of the ARP table attacked devices.

Expiremental Setup
------------------
Inside of an isolated network, an attacker performing an ARP spoofing attack,
man in the middles http requests to a website running on a device in the local
network.


- 1 Netgear Max Unmanaged Switch
- 2 Raspberry Pi 3 running Raspbian-Stretch (v4.14)
- 1 Lenovo T480 running Monopticon v0.1.1 (deadbeef) and Bettercap v2.22

### Network Design

The photo below shows the yellow ethernet cables

The addresses are statically shown below in the diagram.

10.0.0.1 aa:aa: # Andrea
10.0.0.2 bb:bb: # Bruno
10.0.0.3 cc:cc: # Cocco

[img](src)


### Attack Results

As shown the once the Lenovo machine starts broadcasting IP addresses

[img](src)
