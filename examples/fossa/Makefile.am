AM_CFLAGS = \
	 -fPIC \
	 -Wall \
	 -g \
	 -I/opt/local/include \
	 -DTEONE_SYS_CONFIG_DIR=\""${sysconfdir}/teonet"\" \
	 $(DRELEASE_TEONE) \
	 -I../../src \
	 -I../../embedded/libpbl/src \
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

LIBS = -lteonet -lcrypto

bin_PROGRAMS = teofossa

teofossa_SOURCES = teofossa.c fossa.c
