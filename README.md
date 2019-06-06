Monopticon
==========

This was a fork of Wireshark version [2.6.8](https://github.com/wireshark/wireshark/tree/wireshark-2.6.8).

Now it is a stand alone application that visualizes ethernet traffic in realtime. It leverages Zeek to capture packets and broker messages and then renders traffic flow with [Magnum](https://magnum.graphics/) and OpenGL.

All reactions to this project including silly questions are appreciated. Open an issue here or contact [Nick](https://nskelsey.com) directly.

New source code will live in `src/` and one-off tests in `src/expirements`

## Usage

1) Install the software following the commands documented below.
2) Prepare a suitable interface to capture packets on.

Read the file mopt_iface_proto.sh to ensure that it will work with your system.

3) Run:

```bash
> monopticon
```

### Arch Linux

Download the imgui-src package for the imgui headers.

```bash
> wget https://github.com/NSkelsey/monopticon/raw/master/pkg/imgui-src-1.66b-1-any.pkg.tar
> pacman -U imgui-src-1.66b-1-any.pkg.tar
```

Install monopticon and its dependencies from the Arch user repository.

```bash
> yay monopticon
```
