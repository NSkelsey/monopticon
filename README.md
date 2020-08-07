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

## In a browser:

[Check it out](https://nskelsey.com/secret-url/web-samples/)


## Local Usage

To use Monopticon locally you'll need to build and install a few components. The instructions for installing each piece are listed below.

- [mux_server](https://github.com/NSkelsey/monopticon/tree/master/contrib/expirements/ws/mux_server)
- [monitor_server](https://github.com/NSkelsey/monopticon/tree/master/src/scripts)
- [web page](https://github.com/NSkelsey/monopticon/tree/master/src/web)


## Documentation

For a detailed description of the architecture of Monopticon read the [design document](https://github.com/NSkelsey/monopticon/wiki/Design-and-Architecture).
