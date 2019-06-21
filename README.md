Monopticon
==========

This was a fork of Wireshark version [2.6.8](https://github.com/wireshark/wireshark/tree/wireshark-2.6.8).

Now it is a stand alone application that visualizes ethernet traffic in realtime. It leverages [Zeek](https://www.zeek.org/) to capture packets and broker messages and then renders traffic flow with [Magnum](https://magnum.graphics/) and OpenGL.

The goal of the software is four fold:
- Provide simple visual network diagnostics to resolve configuration issues.
- Demonstrate attacks, information leakage and erroneous devices in local networks and traffic flows.
- Simplify network reconnaissance and manage man-in-the-middle attacks.
- Define network elements symbolically to simplify the explanation and diffusion of knowledge about computer networks.

This software might be useful to you if you:
- must configure local networks
- must defend networks
- try to penetrate networks
- make _pew pew_ noises when pinging 8.8.8.8


All reactions to this project including silly questions are appreciated. Open an issue here or contact [Nick](https://nskelsey.com) directly.


The animation below demonstrates the output of Monopticon (v0.2.0) while capturing packets from a machine scanning a local network. For more examples visit `src/expirements`.

![what it looks like](https://raw.githubusercontent.com/nskelsey/monopticon/master/contrib/screens/t5.gif)

## Usage

1) Install the software following the commands documented below.
2) Prepare a suitable interface to capture packets on.

Read the file mopt_iface_proto.sh to ensure that it will work with your system.

3) Run:

```zsh
> monopticon
```

### Arch Linux

Download the imgui-src package for the imgui headers.

```zsh
> wget https://github.com/NSkelsey/monopticon/raw/master/pkg/imgui-src-1.66b-1-any.pkg.tar
> sudo pacman -U imgui-src-1.66b-1-any.pkg.tar
```

Install monopticon and its dependencies from the Arch user repository.

```zsh
> yay monopticon
```

Authorize the zeek binary to capture packets without sudo (for every user 0_o)
```zsh
> sudo setcap cap_net_raw,cap_net_admin=eip /usr/local/bro/bin/zeek
```
