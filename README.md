Monopticon
==========

This was a fork of Wireshark version [2.6.8](https://github.com/wireshark/wireshark/tree/wireshark-2.6.8).

Now it is a standalone application that visualizes ethernet traffic in realtime. It leverages Zeek to capture packets and broker messages and then renders traffic flow with [Magnum](https://magnum.graphics/) and OpenGL.

All reactions to this project including silly questions are appreciated. Open an issue here or contact [Nick](https://nskelsey.com) directly.

New source code will live in `src/` and one-off tests in `src/expirements`

## Installation

In addition to bro/Zeek, this project requires that `magnum` and `corrade` are installed on the system to build the target `evenbettercap`.

After resolving the dependencies compilation follows the cmake norm:


```bash
> mkdir build; cd build
> cmake ..
> cmake --build .
```

## Usage

Select a suitable interface to capture packets on and simply run the following two commands in two different terminals.

```bash
# launches the user interface
> ./build/bin/evenbettercap

# launches zeek for packet capture on the interface en1
> /usr/local/bro/bin/bro -i en1 -b bro-peer-connector.bro
```
