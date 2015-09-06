AM_CFLAGS = \
	 -fPIC \
	 -Wall \
	 -g \
	 -I/opt/local/include \
	 -DTEONE_SYS_CONFIG_DIR=\""${sysconfdir}/teonet"\" \
	 $(DRELEASE_TEONE) \
	 -I../src \
	 -I../embedded/libpbl/src
	
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

LIBS = -lteonet -lcrypto -lcunit -lev
if TEO_THREAD	
LIBS += -pthread
endif

noinst_PROGRAMS = teonet_tst test_teonet

teonet_tst_SOURCES = teonet_tst.c
test_teonet_SOURCES = test_teonet.c test_net_tr-udp.c test_crypt.c test_cque.c \
		      test_teodb.c
