# Arch Linux package build

To create Arch Linux package and install it to this machine execute next commands:

    ./make-tarball.sh
    makepkg -si

Get AUR repository:

    git clone ssh://aur@aur.archlinux.org/teonet.git teonet-aur
    
Create new SRCINFO in teonet-aur folder:

    makepkg --printsrcinfo > .SRCINFO
    
