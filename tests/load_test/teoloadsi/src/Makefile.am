## Process this file with automake to produce Makefile.in
#

AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" $(TEOLOADSI_CFLAGS)

AM_CFLAGS = \
	 -Wall \
	 -g \
	 -std=c11 \
	 -I$(HOME)/Projects/teonet/src/ \
	 -I$(HOME)/Projects/teonet/embedded/libpbl/src \
	 -I$(HOME)/Projects/teonet/embedded/teocli/libteol0 \
	 -I$(HOME)/Projects/teonet/embedded/libtrudp/src \
	 -I/usr/include/libtrudp \
	 -I/usr/include/teonet

AM_LDFLAGS = \
 	 -L$(HOME)/Projects/teonet/src/

AM_CXX = clang++
AM_CXXFLAGS = \
         -g -Wall \
	 -std=c++11 \
	 -I/opt/localionclude \
	 -I$(HOME)/Projects/teonet/src/ \
	 -I$(HOME)/Projects/teonet/embedded/libpbl/src \
	 -I$(HOME)/Projects/teonet/embedded/teocli/libteol0 \
	 -I$(HOME)/Projects/teonet/embedded/libtrudp/src \
	 -I/usr/include/libtrudp \
	 -I/usr/include/teonet

## Debug info
if DEBUG
  AM_CFLAGS += -g3 -O0
  AM_CXXFLAGS += -g3 -O0
else
  AM_CFLAGS += -O2
  AM_CXXFLAGS += -O2
endif

LIBS =-lteonet -lcrypto

bin_PROGRAMS = teoloadsi

teoloadsi_SOURCES = \
	main.c

#teoloadsi_LDFLAGS =

teoloadsi_LDADD = $(teoloadsi_LIBS)
