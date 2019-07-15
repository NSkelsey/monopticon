## Compiling on Unix systems

## Installation
The first builds of this project have been tested on Arch systems and attempted on OSX

This project requires that the source of `magnum`, `corrade`, `imgui` `zeek` and its `broker` are installed on the system to build the target `ebc`.

The documentation of the magnum project provides a reasonable solution for the installation of the required headers, source and object files. Instead the linkage between magnum and imgui through the magnum-integration requires particular attention.

The relevant docs are listed here for your reference. Compilation of this program is an excerise in linking with cmake and patience so always remember that its not about the final binary distributed but the journey.

- [magnum](https://doc.magnum.graphics/magnum/building.html)
- [corrade](https://doc.magnum.graphics/corrade/building-corrade.html)
- [magnum-integration](https://doc.magnum.graphics/magnum/building-integration.html)
- [imgui-src](https://github.com/mosra/archlinux/tree/master/imgui-src)
- [zeek](https://docs.zeek.org/en/stable/install/install.html)
- [zeek/broker](https://github.com/zeek/broker)


#### Step 1.

The following requirements are common enough that we can assume they are already installed.

```
> cmake
> g++
> git
> libssl
> sdl2
```

#### Step 2.

Follow the instructions for magnum and corrade.


#### Step 3.

Build imgui and magnum-integration.

Execute the following script to install imgui:

```bash
#!/bin/bash
mkdir imgui-src; cd imgui-src
wget https://github.com/ocornut/imgui/archive/v1.66b.tar.gz
tar xf imgui-1.66b.tar.gz
install -m755 -d imgui-1.66b
for i in imgui.h imgui_internal.h imconfig.h imstb_textedit.h imstb_rectpack.h imstb_truetype.h imgui.cpp imgui_widgets.cpp imgui_draw.cpp imgui_demo.cpp; do
      install -m644 $i imgui-1.66b/opt/imgui/$i
done
```

Execute the following commands to build and install magnum-integration.

> git clone https://github.com/mosra/magnum-integration
> cd magnum-integration; mkdir build; cd build
> cmake .. -DWITH_IMGUI=ON
> make -j7
> sudo make install


#### Step 4.

Install the zeek broker with its git submodule dependencies.

```bash
> git clone --recursive https://github.com/zeek/broker
> ./configure --disable-python --disable-docs --disable-tests
> cd build; make -j7 install
```

#### Step 5.

Install Zeek

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

#### Step 6.

Compile Monopticon

```bash
> cd monopticon; mkdir build; cd build
> cmake ..
> cmake --build .
```
## Arch Notes

#### Custom Config

Soft link local scripts without repeating `make install`
```zsh
> sudo ln -fs /home/synnick/projects/monopticon/src/scripts/l2_eth_monitor.bro /usr/local/share/monopticon/scripts/l2_eth_monitor.bro
> sudo ln -fs /home/synnick/projects/monopticon/monopt_iface_proto.sh /usr/local/bin/monopt_iface_proto
```


#### Package Preparation

These are the steps to produce the packages above.

#### Step 1

Build the package imgui-src yourself and install it.

```bash
# in a tmp dir
> git clone https://github.com/mosra/archlinux
> cd archlinux/imgui-src
> makepkg -fp PKGBUILD
```

#### Step 2

Build & install packages in the aur.

```bash
> yay -S magnum-git corrade-git zeek-broker
```

```bash
# NOTE here the PKGBUILD file must be modified to specify WITH_IMGUI=ON and a path.
# You can replace the file inside of mosra's directory with the one in this repository.
>  cp -f monopticon/contrib/PKGBUILD ./PKGBUILD

> yay -S magnum-integration
```

#### Step 3

Build and install the monopticon package

```bash
> cd monopticon/pkg/arch

> makepkg # clones the current repository into $PWD/src
> sudo pacman -U monopticon-0.1.0-1-x86_64.pkg.tar
```

#### Pre-built packages

Clone this repository and install the packages:

```bash
> cd monopticon/pkg
> sudo pacman -U imgui-src-1.66b-1-any.pkg.tar
> sudo pacman -U magnum-integration-dev-1-x86_64.pkg.tar
> sudo pacman -U zeek-broker-1.1.2.49-1-x86_64.pkg.tar
> sudo pacman -U monopticon-0.1.0-1-x86_64.pkg.tar
```
