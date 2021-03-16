# Arch Linux package build

To create Arch Linux package and install it to this machine execute next commands:

    ./make-tarball.sh
    makepkg -si

Get AUR repository:

    git clone ssh://aur@aur.archlinux.org/teonet.git teonet-aur

Create new SRCINFO in teonet-aur folder:

    makepkg --printsrcinfo > .SRCINFO

## Install Yay helper to setup AUR packages

Yay is an AUR helper. It is also a Pacman wrapper. It is a popular tool for managing
packages on Arch Linux. It provides a lot of extra functionality including searching,
tab-completion, and dependency related features.

Steps to install Yay on Arch Linux:

Update your system:

    sudo pacman -Syyu

Install Git:

    sudo pacman -S git

Clone the repository:

    git clone https://aur.archlinux.org/yay.git

Move to the directory:

    cd yay

Build it:

    makepkg -si

Test it by installing a package:

    yay -S teonet
