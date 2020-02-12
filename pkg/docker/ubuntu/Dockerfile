FROM ubuntu:bionic

RUN yes | apt update
RUN yes | apt install vim git sudo dpkg-dev debhelper cmake wget unzip

# Use as many cores as possible when building pakages

RUN groupadd sudoers
RUN echo "build   ALL=(ALL) NOPASSWD: ALL" >> /etc/sudoers
RUN useradd --create-home --groups sudoers -m build && echo "build:build" | chpasswd

USER build
ENV HOME /home/build

WORKDIR /home/build

RUN wget https://github.com/mosra/corrade/archive/v2019.10.zip
RUN unzip v2019.10.zip && rm v2019.10.zip

RUN cd corrade-2019.10 && ln -s package/debian .
RUN cd corrade-2019.10 && dpkg-buildpackage

RUN sudo dpkg -i corrade_2019.10_amd64.deb
RUN sudo dpkg -i corrade-dev_2019.10_amd64.deb

RUN wget https://github.com/mosra/magnum/archive/v2019.10.zip
RUN unzip v2019.10.zip && rm v2019.10.zip

RUN yes | sudo apt install parallel libgl-dev libopenal-dev libglfw3-dev libsdl2-dev

RUN cd magnum-2019.10 && ln -s package/debian .
RUN cd magnum-2019.10 && dpkg-buildpackage

RUN sudo dpkg -i magnum_2019.10_amd64.deb
RUN sudo dpkg -i magnum-dev_2019.10_amd64.deb

RUN yes | sudo apt install libdevil-dev libjpeg-dev libpng-dev libfaad-dev libfreetype6-dev libassimp-dev
RUN wget https://github.com/mosra/magnum-plugins/archive/v2019.10.zip
RUN unzip v2019.10.zip && rm v2019.10.zip

RUN cd magnum-plugins-2019.10 && ln -s package/debian .
RUN cd magnum-plugins-2019.10 && dpkg-buildpackage

RUN yes | sudo apt install libpng16-16 libbullet-dev libeigen3-dev libglm-dev
RUN sudo dpkg -i magnum-plugins_2019.10_amd64.deb


RUN sudo sh -c "echo 'deb http://download.opensuse.org/repositories/security:/zeek/xUbuntu_18.04/ /' > /etc/apt/sources.list.d/security:zeek.list"
RUN wget -nv https://download.opensuse.org/repositories/security:zeek/xUbuntu_18.04/Release.key -O Release.key
RUN sudo apt-key add - < Release.key
RUN sudo apt-get update
RUN sudo apt install -y debconf-utils
RUN echo "dma dma/mailname string build.io" | sudo debconf-set-selections
RUN echo "dma dma/relayhost string 127.0.0.1" | sudo debconf-set-selections
#RUN sudo debconf-set-selections <<< "dma dma/mailname string build.io"
RUN sudo apt install -yq dma
RUN sudo apt install -yq zeek

RUN wget https://github.com/mosra/magnum-integration/archive/v2019.10.zip
RUN unzip v2019.10.zip && rm v2019.10.zip

# Need to modify for IMGUI
RUN cd magnum-integration-2019.10 && ln -s package/debian .
#RUN cd magnum-integration-2019.10 && cp ~/mag-int/magnum-integration/debian/build.sh debian/build.sh
#RUN cd magnum-integration-2019.10 && dpkg-buildpackage
#
#RUN sudo dpkg -i magnum-integration_2019.10_amd64.deb

# monopticon deps

RUN sudo dpkg -i magnum-plugins-dev_2019.10_amd64.deb
RUN sudo apt -yq install libbroker-dev zeek-libcaf-dev libfreetype6-dev
