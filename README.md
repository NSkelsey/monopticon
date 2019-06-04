Monopticon
==========

This was a fork of Wireshark version [2.6.8](https://github.com/wireshark/wireshark/tree/wireshark-2.6.8).

Now it is a standalone application that visualizes ethernet traffic in realtime. It leverages Zeek to capture packets and broker messages and then renders traffic flow with [Magnum](https://magnum.graphics/) and OpenGL.

All reactions to this project including silly questions are appreciated. Open an issue here or contact [Nick](https://nskelsey.com) directly.

New source code will live in `src/` and one-off tests in `src/expirements`

## Usage

Build the software, prepare a suitable interface to capture packets on and then run the following command.

```bash
# launches the user interface
> export LD_LIBRARY_PATH=/usr/local/lib; ./build/bin/ebc
```

## Compilation
The first builds of this project have been tested on Arch systems.

This project requires that the source of `magnum`, `corrade`, `imgui` `zeek` and its `broker` are installed on the system to build the target `ebc`.

The documentation of the magnum project provides a reasonable solution for the installation of the required headers, source and object files. Instead the linkage between magnum and imgui through the magnum-integration requires particular attention.

The relevant docs are listed here for your reference. Compilation of this program is an excerise in linking with cmake and patience so always remember that its not about the final binary distributed but the journey.

- [magnum](https://doc.magnum.graphics/magnum/building.html)
- [corrade](https://doc.magnum.graphics/corrade/building-corrade.html)
- [magnum-integration](https://doc.magnum.graphics/magnum/building-integration.html)
- [imgui-src](https://github.com/mosra/archlinux/tree/master/imgui-src)
- [zeek](https://docs.zeek.org/en/stable/install/install.html)
- [zeek/broker](https://github.com/zeek/broker)


### Arch Linux

#### Step 0

Clone this repository.

And install the packages:

```
> cd monopticon/pkg
> sudo pacman -U magnum-integration-dev-1-x86_64.pkg.tar
> sudo pacman -U zeek-broker-1.1.2.49-1-x86_64.pkg.tar
> sudo pacman -U monopticon-0.1.0-1-x86_64.pkg.tar
```


#### Step 1

Prepare the dependency not listed in the aur by simply running:

```bash
> sudo pacman -U monopticon/pkg/imgui-src-1.66b-1-any.pkg.tar
```

Or build the package imgui-src yourself and install it.

```bash
# in a tmp dir
> git clone https://github.com/mosra/archlinux
> cd archlinux/imgui-src
> makepkg -fp PKGBUILD
> sudo pacman -U imgui-src-1.66b-1-any.pkg.tar
```

#### Step 2

Install packages saved in the aur

Build & install packages in the aur.

```bash
> yay -S magnum-git corrade-git zeek-broker
```

```bash
# NOTE here the PKGBUILD file must be modified to specify WITH_IMGUI=ON and a path.
# You can replace the file inside of mosra's directory with the one in this repository.
>  cp -f /path/to/monopticon/contrib/PKGBUILD ./PKGBUILD

> yay -S magnum-integration
```

#### Step 3

Build and install the monopticon package

```bash
> cd monopticon/pkg/arch

> makepkg # clones the current repository into $PWD/src
> sudo pacman -U monopticon-0.1.0-1-x86_64.pkg.tar
```


### Other Less Fortunate Linuxes

#### 1. The following requirements are common enough that we can assume they are already installed.

```
> cmake
> g++
> git
> libssl
> sdl2
```

#### 2. Install magnum and corrade

Left as an excerise for the reader

#### 3. Build the package imgui-src and install it.

ibid

#### 4. Build and install the package magnum-integration specifying usage with ImGui.

ibid

#### 5. Install the zeek broker with its git submodule dependencies.

```bash
> git clone --recursive https://github.com/zeek/broker
> ./configure --build-static
> cd build; make -j7 install
```

#### 6. Install zeek

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

#### 7. After resolving the dependencies compilation follows the cmake norm.

```bash
> cd monopticon; mkdir build; cd build
> cmake ..
> cmake --build .
```
