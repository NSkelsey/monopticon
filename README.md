Monopticon
==========


This is an application that visualizes ethernet traffic in realtime. It leverages [Zeek](https://www.zeek.org/) to capture packets and broker messages and then renders traffic flow with [Magnum](https://magnum.graphics/) and OpenGL.

[![Build Status](https://travis-ci.org/NSkelsey/monopticon.svg?branch=webopticon)](https://travis-ci.org/nskelsey/monopticon)

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


The animation below demonstrates the output of Monopticon (v0.3.0) monitoring a local network from a span port. For more examples visit `contrib/expirements`.

![what it looks like](https://nskelsey.com/res/span-traffic.gif)

## Usage

1) Install the software following the commands documented below.
2) Prepare a suitable interface to capture packets on.

Read the file `monpt_iface_proto.sh` to ensure that it will work with your system.

3) Run:

```zsh
> monopticon
```

### OpenSuse Leap

ARM and x86 rpm packages are available on the [Open Build System](https://software.opensuse.org/package/monopticon?search_term=monopticon).


### Arch Linux

In an alternative rolling universe the following command may function:

```zsh
> yay monopticon
```

### Building

See `INSTALL.md` for notes on building.

For true believers there is a Dockerfile in `pkg/docker` that should work out of the box.
