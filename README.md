Monopticon
==========

This was a fork of Wireshark version [2.6.8](https://github.com/wireshark/wireshark/tree/wireshark-2.6.8).

Now it is a standalone application that visualizes ethernet traffic in realtime. It leverages Zeek to capture packets and broker messages and then renders traffic flow with [Magnum](https://magnum.graphics/) and OpenGL.

All reactions to this project including silly questions are appreciated. Open an issue here or contact [Nick](https://nskelsey.com) directly.

New source code will live in `src/` and one-off tests in `src/expirements`

## Compliation
The first builds of this project have been tested on arch systems.

In addition to bro/Zeek, this project requires that `magnum` and `corrade` are installed on the system to build the target `evenbettercap`.

The documentation of the magnum project provides a quick solution for the installation of the required headers, source and object files.

- [magnum](https://doc.magnum.graphics/magnum/building.html)
- [corrade](https://doc.magnum.graphics/corrade/building-corrade.html)
- [magnum-integration](https://doc.magnum.graphics/magnum/building-integration.html)
- [imgui-src](https://github.com/mosra/archlinux/tree/master/imgui-src)

1. Install magnum-git # includes corrade-git
2. Install magnum-integration-git

```bash
> pacman -S magnum
> pacman -S corrade

```

3. Build the package imgui-src and install it.

```bash
# in your scratch dir
> git clone https://github.com/mosra/archlinux
> cd imgui-src
> makepkg -fp PKGBUILD
> pacman -U name-of.pkg.tar.xz
```

4. Build and install the package magnum-integration specifying usage with ImGui.

```bash
> git clone https://github.com/mosra/magnum-integration
> cd magnum-integration/package/archlinux
> use misc/PKGBUILD
> makepkg -fp PKGBUILD
> pacman -U name-of.pkg.tar.xz
```

5. Install zeek broker dependencies

```bash
> git clone https://github.com/zeek/broker
> cd broker
> git submodule foreach --recursive git pull origin master
> ./configure
> sudo make install
```

6. After resolving the dependencies compilation follows the cmake norm.

```bash
> cd monopticon; mkdir build; cd build
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
