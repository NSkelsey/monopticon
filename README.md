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
- Must configure local networks
- Must defend networks
- Penetrate networks
- Make _pew pew_ noises when pinging 8.8.8.8


All reactions to this project including silly questions are appreciated. Open an issue here or contact [Nick](https://nskelsey.com) directly.


The animation below demonstrates the output of Monopticon (v0.2.0) monitoring a local network from a span port. For more examples visit `src/expirements`.

![what it looks like](https://raw.githubusercontent.com/nskelsey/monopticon/master/contrib/screens/span-traffic.webm)

## Usage

1) Install the software following the commands documented below.
2) Prepare a suitable interface to capture packets on.

Read the file `mopt_iface_proto.sh` to ensure that it will work with your system.

3) Run:

```zsh
> monopticon
```

### OpenSuse Leap

ARM and x86 rpm packages are available on the [Open Build System](https://software.opensuse.org/package/monopticon?search_term=monopticon).


### Arch Linux

This project is developed primarily on Arch, so its possible to use it, but its constantly evolving.

A well written PKGBUILD is under development, but in the meantime see the monopticon package in the [AUR](https://aur.archlinux.org/packages/monopticon/) and [BUILDING.md](./BUILDING.md)
