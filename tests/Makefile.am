AM_CFLAGS = \
	 -fPIC \
	 -Wall \
	 -I/opt/local/include \
	 -DTEONET_SYS_CONFIG_DIR=\""${sysconfdir}/teonet"\" \
	 $(DRELEASE_TEONE) \
	 -I../src \
	 -I../embedded/libpbl/src \
	 -I../embedded/teocli/libteol0 \
	 -I../embedded/teocli/libtrudp/src \
	 -I../embedded/teocli/libtrudp/libs/teobase/include \
	 -I../embedded/teocli/libtrudp/libs/teoccl/include \
	 -I/usr/include/libev

if TEO_THREAD	
AM_CFLAGS += -DTEO_THREAD
endif	

if MINGW
AM_CFLAGS += -I../../../confuse-2.7/src -I../../../libev-4.19/ -I../../../libtuntap-master
else
AM_CFLAGS += -fPIC
endif

AM_LDFLAGS =  -L/opt/local/lib -L../src

if MINGW
AM_LDFLAGS += -L../../../confuse-2.7/src -L../../../libev-4.19 -L../../../libtuntap-master
endif

LIBS = \
	../src/libteonet.la \
	-lcrypto -lcunit -lev

if TEO_THREAD	
LIBS += -pthread
endif

noinst_PROGRAMS = teonet_tst test_teonet pipe_ping

teonet_tst_SOURCES  = teonet_tst.c

test_teonet_SOURCES = \
	test_teonet.c \
	test_tr-udp.c \
	test_crypt.c \
	test_cque.c \
	test_teodb.c \
	test_tcp_proxy.c \
	test_subscribe.c \
	# end of test_teonet_SOURCES

# test_teonet_LDFLAGS = ../embedded/teocli/linux/libteocli.la

pipe_ping_SOURCES = pipe_ping.c
