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
COPY ${CORRADE} local-pkgs

COPY ${CORRADE_WASM} local-pkgs

COPY ${MAGNUM}  local-pkgs
COPY ${MAGNUM_WASM}  local-pkgs

# magnum integration & plugins
COPY ${MAGNUM_PLUGINS_WASM} local-pkgs

COPY ${INT_MONOPTICON_WASM} local-pkgs

# Install all local packages
RUN yes | sudo pacman -U local-pkgs/${CORRADE}
RUN yes | sudo pacman -U local-pkgs/${CORRADE_WASM}
RUN yes | sudo pacman -U local-pkgs/${MAGNUM}
RUN yes | sudo pacman -U local-pkgs/${MAGNUM_WASM}
RUN yes | sudo pacman -U local-pkgs/${MAGNUM_PLUGINS_WASM}
RUN yes | sudo pacman -U local-pkgs/${INT_MONOPTICON_WASM}

RUN yes | sudo pacman -Syyu

# Setup emscripten environment
RUN emcc --version

ENV EM_CONFIG $HOME/.emscripten
COPY ./emscripten-conf $HOME/.emscripten

# Download and build freetype emscripten port.
RUN emcc -s USE_FREETYPE=1 /usr/lib/emscripten/tests/freetype_test.c

ARG PROTOBUF_VERSION="3.9.0"

# Build specific version of protobuf
RUN cd $HOME && \
    curl https://github.com/protocolbuffers/protobuf/releases/download/v$PROTOBUF_VERSION/protobuf-cpp-$PROTOBUF_VERSION.tar.gz -L > protobuf-cpp-$PROTOBUF_VERSION.tar.gz && \
    tar xvzf ./protobuf-cpp-$PROTOBUF_VERSION.tar.gz

WORKDIR "${HOME}/protobuf-$PROTOBUF_VERSION"
RUN ./configure && make
RUN sudo make install && sudo ldconfig

WORKDIR $HOME

ENV PROTOBUF_PATCH 0001-patch-apply-kwonji.patch
COPY ${PROTOBUF_PATCH} local-pkgs

RUN mkdir protobuf && tar xvzf protobuf-cpp-$PROTOBUF_VERSION.tar.gz -C protobuf --strip-components 1 && \
    cd $HOME/protobuf && patch -p1 < ~/local-pkgs/${PROTOBUF_PATCH}

RUN cd $HOME/protobuf && \
    sh autogen.sh && \
    emconfigure ./configure --prefix=/usr/lib/emscripten/system/ && \
    emmake make && \
    sudo make install

# Rename the compiled wasm (shared object) archives to the expected extension
RUN cd $HOME/protobuf && \
   sudo cp src/.libs/libprotobuf.a /usr/lib/emscripten/system/lib/protobuf.bc && \
   sudo cp src/.libs/libprotobuf-lite.a /usr/lib/emscripten/system/lib/protobuf-lite.bc

# Include pugixml
ENV PUGI_VER="1.10"

RUN curl https://github.com/zeux/pugixml/releases/download/v1.10/pugixml-$PUGI_VER.tar.gz -L > pugixml.tar.gz && \
    tar xvzf pugixml.tar.gz

COPY Emscripten-wasm.cmake local-pkgs/

RUN mkdir $HOME//pugixml-$PUGI_VER/build && \
    cd $HOME/pugixml-$PUGI_VER/build && \
    cmake .. -DCMAKE_TOOLCHAIN_FILE=/home/build/local-pkgs/Emscripten-wasm.cmake -DCMAKE_PREFIX_PATH=/usr/lib/emscripten/system -DCMAKE_INSTALL_PREFIX=/usr/lib/emscripten/system && \
    cmake --build . && \
    sudo make install
