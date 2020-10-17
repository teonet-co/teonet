AM_CFLAGS = \
	 -fPIC \
	 -Wall \
	 -g \
	 -I/opt/local/include \
	 -DTEONET_SYS_CONFIG_DIR=\""${sysconfdir}/teonet"\" \
	 $(DRELEASE_TEONET) \
	 -I../../src \
	 -I../../embedded/libpbl/src \
	 -I../../embedded/teocli/libteol0 \
	 -I../../embedded/teocli/libtrudp/src \
	 -I../../embedded/teocli/libtrudp/libs/teobase/include \
	 -I../../embedded/teocli/libtrudp/libs/teoccl/include \
	 -I/usr/include/libtrudp \
	 -I/usr/include/libev \
	 -pthread

if MINGW
AM_CFLAGS += -I../../../confuse-2.7/src -I../../../libev-4.19/ -I../../../libtuntap-master
else
AM_CFLAGS += -fPIC
endif

AM_LDFLAGS =  -L/opt/local/lib -L../../src

if MINGW
AM_LDFLAGS += -L../../../libev-4.19 -L../../../confuse-2.7/src -L../../../libtuntap-master
endif

LIBS = \
	../../src/libteonet.la \
	-lcrypto -lev

#bin_PROGRAMS = teofossa
noinst_PROGRAMS = teofossa

teofossa_SOURCES = teofossa.c fossa.c fossa.h
#EXTRA_DIST = fossa.h

exampledir = $(datarootdir)/doc/@PACKAGE@/examples/fossa
example_DATA = fossa.c fossa.h teofossa.c
