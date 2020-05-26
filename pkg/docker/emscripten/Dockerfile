FROM archlinux

RUN yes | pacman -Syu
RUN yes | pacman -Sy sudo vim git
RUN yes | pacman --noconfirm -Sy base-devel

RUN groupadd sudoers
RUN echo "build   ALL=(ALL) NOPASSWD: ALL" >> /etc/sudoers
RUN useradd --create-home --groups sudoers -m build && echo "build:build" | chpasswd

USER build
ENV HOME /home/build


WORKDIR /home/build
RUN yes | sudo pacman -Sy cmake ninja
RUN yes | sudo pacman -Sy emscripten
ENV PATH="/usr/lib/emscripten/:${PATH}"
ENV BINARYEN="/usr/"

# Use as many cores as possible when building pakages
RUN sudo sed -i '/#MAKEFLAGS="-j2"/c\MAKEFLAGS="-j$(nproc)"' /etc/makepkg.conf

RUN git clone https://aur.archlinux.org/yay.git /home/build/yay
WORKDIR /home/build/yay

RUN yes | sudo pacman -Sy go
RUN yes | makepkg -si


RUN yes | sudo pacman -Sy sdl2 glfw-x11 openal vulkan-icd-loader

WORKDIR /home/build/

RUN yes | sudo pacman -Sy devil faad2 libpng libjpeg freetype2 assimp

RUN yes | yay --noconfirm -S imgui-src

RUN yes | sudo pacman -Sy glfw-x11
RUN yes | sudo pacman -Sy libffi


RUN mkdir local-pkgs

# To repeat this build, the system requires the following prebuilt packages and their deps
ENV CORRADE corrade-dev.release-1-x86_64.pkg.tar.xz
ENV CORRADE_WASM emscripten-corrade-dev.wasm-1-any.pkg.tar.xz
ENV MAGNUM magnum-dev.release-1-x86_64.pkg.tar.xz
ENV MAGNUM_WASM emscripten-magnum-dev.wasm.webgl2-1-any.pkg.tar.xz
ENV MAGNUM_PLUGINS_WASM emscripten-magnum-plugins-dev.wasm.webgl2-1-any.pkg.tar.xz
ENV INT_MONOPTICON_WASM monopticon-magnum-integration-2019.10.r48.gcc90f56-1-x86_64.pkg.tar.xz

# corrade + magnum built normally & for emscripten-wasm-webgl2
COPY corrade/package/archlinux/${CORRADE} local-pkgs
COPY corrade/package/archlinux/${CORRADE_WASM} local-pkgs

COPY magnum/package/archlinux/${MAGNUM}  local-pkgs
COPY magnum/package/archlinux/${MAGNUM_WASM}  local-pkgs

# magnum integration & plugins
COPY magnum-plugins-git/${MAGNUM_PLUGINS_WASM} local-pkgs

COPY monopticon-magnum-integration/${INT_MONOPTICON_WASM} local-pkgs

# Install all local packages
RUN yes | sudo pacman -U local-pkgs/${CORRADE}
RUN yes | sudo pacman -U local-pkgs/${CORRADE_WASM}
RUN yes | sudo pacman -U local-pkgs/${MAGNUM}
RUN yes | sudo pacman -U local-pkgs/${MAGNUM_WASM}
RUN yes | sudo pacman -U local-pkgs/${MAGNUM_PLUGINS_WASM}
RUN yes | sudo pacman -U local-pkgs/${INT_MONOPTICON_WASM}

# Setup emscripten environment
RUN emcc --clear-config
# Download and build freetype emscripten port.
RUN emcc -s USE_FREETYPE=1 /usr/lib/emscripten/tests/freetype_test.c
