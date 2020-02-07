FROM archlinux

RUN yes | pacman -Syu
RUN yes | pacman -Sy sudo vim git
RUN yes | pacman --noconfirm -Sy base-devel

# Use as many cores as possible when building pakages
RUN sudo sed -i '/#MAKEFLAGS="-j2"/c\MAKEFLAGS="-j$(nproc)"' /etc/makepkg.conf

RUN groupadd sudoers
RUN echo "build   ALL=(ALL) NOPASSWD: ALL" >> /etc/sudoers
RUN useradd --create-home --groups sudoers -m build && echo "build:build" | chpasswd

USER build
ENV HOME /home/build

RUN git clone https://aur.archlinux.org/yay.git /home/build/yay
WORKDIR /home/build/yay
RUN yes | makepkg -si

WORKDIR /home/build
RUN yes | sudo pacman -Sy go
RUN yes | sudo pacman -Sy glfw-x11

RUN yes | yay --noconfirm -S magnum-git
RUN yes | yay -Sy --searchby maintainer=synnick zeek
RUN yes | yay --noconfirm -S magnum-plugins-git imgui-src monopticon-magnum-integration

RUN yes | yay -Sy --searchby maintainer=synnick monopticon
