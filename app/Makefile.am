AM_CFLAGS = \
	 -fPIC \
	 -Wall \
	 -g \
	 -I/opt/local/include \
	 -DTEONET_SYS_CONFIG_DIR=\""${sysconfdir}/teonet"\" \
	 $(DRELEASE_TEONE) \
	 -I../src \
	 -I../embedded/libpbl/src

#if MINGW
#AM_CFLAGS += -I../../../confuse-2.7/src -I../../../libev-4.19/ -I../../../libmcrypt-2.5.7/lib -I../../../libtuntap-master
#else
#AM_CFLAGS += -fPIC
#endif

AM_LDFLAGS =  -L/opt/local/lib -L../src

#if MINGW
#AM_LDFLAGS += -L../../../libev-4.19 -L../../../confuse-2.7/src -L../../../libtuntap-master -L../../../libmcrypt-2.5.7/lib
#endif

#LIBS = ../src/libteonet.la
LIBS = -lteonet -lcrypto

bin_PROGRAMS = teovpn teosend

teovpn_SOURCES = teovpn.c

teosend_SOURCES = teosend.c
