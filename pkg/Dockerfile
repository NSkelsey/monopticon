FROM archlinux

RUN yes | pacman -Syu
RUN yes | pacman -Sy sudo vim git
RUN echo -e "\ny\n" | pacman -Sy base-devel

RUN groupadd sudoers
RUN echo "build   ALL=(ALL) NOPASSWD: ALL" >> /etc/sudoers
CMD /bin/bash

RUN useradd --create-home --groups sudoers -m build && echo "build:build" | chpasswd

USER build
ENV HOME /home/build
WORKDIR /home/build

RUN git clone https://aur.archlinux.org/yay.git && cd yay

RUN yes | sudo pacman -Sy go

USER root
RUN echo "Set disable_coredump false" > /etc/sudo.conf

USER build

RUN cd yay && yes | makepkg -si


RUN yes | sudo pacman -Sy glfw-x11
RUN yes | yay --noconfirm -S magnum-git

RUN yes | yay --noconfirm -S magnum-plugins-git

RUN yes | pacman -Sy swig libmaxminddb

# The build as of 29/1/2020 worked up to here.
#RUN yes | yay --noconfirm -S zeek

#RUN yes | sudo pacman -Sy magnum

CMD /bin/bash

#RUN yay -S magnum-plugins-git
#RUN yay -S magnum-integration
#RUN yes | sudo yay zeek
#RUN yes | sudo yay monopticon


