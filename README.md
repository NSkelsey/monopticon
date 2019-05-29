Monopticon
==========

This was a fork of Wireshark version [2.6.8](https://github.com/wireshark/wireshark/tree/wireshark-2.6.8).

Now it is a standalone application that visualizes ethernet traffic in realtime. It leverages Zeek to capture packets and broker messages and then renders traffic flow with [Magnum](https://magnum.graphics/) and OpenGL.

All reactions to this project including silly questions are appreciated. Open an issue here or contact [Nick](https://nskelsey.com) directly.

New source code will live in `src/` and one-off tests in `src/expirements`

## Compliation
The first builds of this project have been tested on arch systems.

This project requires that the source of `magnum`, `corrade`, `imgui` `zeek` and its `broker` are installed on the system to build the target `ebc`.

The documentation of the magnum project provides a reasonable solution for the installation of the required headers, source and object files. Instead the linkage between magnum and imgui through the magnum-integration requires particular attention.

The relevant docs are listed here for your reference. Compilation of this program is an excerise in linking with cmake and patience so always remember that its not about the final binary distributed but the journey.

- [magnum](https://doc.magnum.graphics/magnum/building.html)
- [corrade](https://doc.magnum.graphics/corrade/building-corrade.html)
- [magnum-integration](https://doc.magnum.graphics/magnum/building-integration.html)
- [imgui-src](https://github.com/mosra/archlinux/tree/master/imgui-src)
- [zeek](https://docs.zeek.org/en/stable/install/install.html)
- [zeek/broker](https://github.com/zeek/broker)

### Steps

0. The following requirements are common enough that we can assume they are already installed.

```
> cmake
> g++
> git
> libssl
> pacman
> sdl2
```


1. Install magnum and corrade

```bash
> pacman -S magnum
> pacman -S corrade
```

2. Build the package imgui-src and install it.

```bash
# in your scratch dir
> git clone https://github.com/mosra/archlinux
> cd imgui-src
> makepkg -fp PKGBUILD
> pacman -U name-of.pkg.tar.xz
```

3. Build and install the package magnum-integration specifying usage with ImGui.

```bash
> git clone https://github.com/mosra/magnum-integration
> cd magnum-integration/package/archlinux
# NOTE here the PKGBUILD file must be modified to specify WITH_IMGUI=ON and a path.
# You can replace the file inside of mosra's directory with the one in this repository.
>  cp -f /path/to/monopticon/contrib/PKGBUILD ./PKGBUILD
> makepkg -fp PKGBUILD
> pacman -U name-of.pkg.tar.xz
```

4. Install the zeek broker and its dependencies.

```bash
> git clone https://github.com/zeek/broker
> cd broker
> git submodule init
> git submodule update
> cd 3rdpart/caf
> git submodule init
> git submodule update
> cd ../..
> ./configure
> make -j7 install
```

5. Install zeek

# TODO here create and maintain a package for zeek to ease the installation

```bash
> mkdir zeek; cd zeek;
> wget https://www.zeek.org/downloads/bro-2.6.1.tar.gz
> tar xvf bro-2.6.1.tar.gz
> cd bro-2.6.1
> ./configure --disable-broctl --disable-auxtools --disable-perftools --disable-python --disable-broker-tests
> sudo make -j7 install

# remove the root privs requirement to listen on bin/bro using setcap as the current user
> sudo setcap cap_net_raw,cap_net_admin=eip /usr/local/bro/bin/bro
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
> export LD_LIBRARY_PATH=/usr/local/lib; ./build/bin/ebc
```

```bash
# launches zeek for packet capture on the interface enp0s31f6
> /usr/local/bro/bin/bro -i enp0s31f6 -b bro-peer-connector.bro
```
