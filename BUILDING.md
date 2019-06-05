## Compiling on Unix systems

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

Install the zeek broker with its git submodule dependencies.

```bash
> git clone --recursive https://github.com/zeek/broker
> ./configure --disable-python --disable-docs --disable-tests
> cd build; make -j7 install
```

#### Step 4.

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

#### Step 5.

Compile Monopticon

```bash
> cd monopticon; mkdir build; cd build
> cmake ..
> cmake --build .
```
#### Install the pre-built packages

Clone this repository and install the packages:

```bash
> cd monopticon/pkg
> sudo pacman -U imgui-src-1.66b-1-any.pkg.tar
> sudo pacman -U magnum-integration-dev-1-x86_64.pkg.tar
> sudo pacman -U zeek-broker-1.1.2.49-1-x86_64.pkg.tar
> sudo pacman -U monopticon-0.1.0-1-x86_64.pkg.tar
```

## Arch Notes

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
