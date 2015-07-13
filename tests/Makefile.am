AM_CFLAGS = \
	 -fPIC \
	 -Wall \
	 -g \
	 -I/opt/local/include \
	 -DKSMESH_SYS_CONFIG_DIR=\""${sysconfdir}/ksmesh"\" \
	 $(DRELEASE_KSMESH) \
	 -I../src \
	 -I../embedded/libpbl/src

if MINGW
AM_CFLAGS += -I../../../confuse-2.7/src -I../../../libev-4.19/ -I../../../libmcrypt-2.5.7/lib -I../../../libtuntap-master
else
AM_CFLAGS += -fPIC
endif

AM_LDFLAGS =  -L/opt/local/lib -L../src

if MINGW
AM_LDFLAGS += -L../../../libev-4.19 -L../../../confuse-2.7/src -L../../../libtuntap-master -L../../../libmcrypt-2.5.7/lib
endif

#LIBS = ../libksmesh.la
LIBS = -lteonet 
#-lcrypt


bin_PROGRAMS = teonet_tst  # ksmqtt_broker echo_server echo_client timer_watcher

teonet_tst_SOURCES = teonet_tst.c
#clitest_SOURCES = clitest.c

# ksmqtt_broker_SOURCES = ksmqtt_broker.c
# timer_watcher_SOURCES = timer_watcher.c
# echo_serv_SOURCES = echo_serv.c
# echo_client_SOURCES = echo_client.c
#
# LIBS = -L../lib ../lib/libev.a -lm
