#!/bin/bash

iprint() {
  echo -e $*
}
fRED() {
  iprint "\x1b[1;31m"
}
fGREEN() {
  iprint "\x1b[1;32m"
}
aRST() {
  iprint "\x1b[0m"
}
fBROWN() {
  iprint "\x1b[22;33m"
}
die() {
  iprint "\n`fRED`FAILED";
  if [[ $# -ge 1 ]]; then
    iprint "`fGREEN`:`fBROWN` " $*
  fi
  iprint "`aRST`\n"
  exit 1; 
}

iprint "\n\t`fBROWN`Cleaning build artifacts`aRST`\n"
make clean

iprint "\n\t`fBROWN`Cleaning build scripts`aRST`\n"
make distclean

iprint "\n\t`fBROWN`Inspecting build artifacts`aRST`\n"
find ./ -name *.o

iprint "\n\t`fBROWN`Creating build scripts`aRST`\n"
./autogen.sh || die "Can't create 'configure' script"

iprint "\n\t`fBROWN`Configuring `fGREEN`DEBUG`fBROWN` build`aRST`\n"
./configure --prefix=/dbg CXXFLAGS="-g -O0" CFLAGS="-g -O0" || die

iprint "\n\t`fBROWN`Now you can run `fGREEN`'make'`fBROWN` to compile `fGREEN`DEBUG`fBROWN` teonet`aRST`\n"
