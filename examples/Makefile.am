AM_CFLAGS = \
	 -fPIC \
	 -Wall \
	 -g \
	 -I/opt/local/include \
	 -DTEONET_SYS_CONFIG_DIR=\""${sysconfdir}/teonet"\" \
	 $(DRELEASE_TEONET) \
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
AM_LDFLAGS += -L../../../libev-4.19 -L../../../confuse-2.7/src -L../../../libtuntap-master
endif

LIBS = -lteonet -lcrypto 
if TEO_THREAD	
LIBS += -pthread
endif

noinst_PROGRAMS = teolarge teosend teomulti teomulti_t teotcp teotun teoterm \
		  teoack teoackm teocque teostream teol0cli

teoack_SOURCES = teoack.c
teoackm_SOURCES = teoackm.c
teocque_SOURCES = teocque.c
teolarge_SOURCES = teolarge.c
teomulti_SOURCES = teomulti.c
teomulti_t_SOURCES = teomulti_t.c
teosend_SOURCES = teosend.c
teotcp_SOURCES = teotcp.c
teoterm_SOURCES = teoterm.c
teotun_SOURCES = teotun.c
teostream_SOURCES = teostream.c
teol0cli_SOURCES = teol0cli.c 
#../embedded/libteol0/teonet_l0_client.c

exampledir = $(datarootdir)/doc/@PACKAGE@/examples
example_DATA = teoack.c teoackm.c teocque.c teolarge.c teomulti.c teomulti_t.c \
	       teosend.c teotcp.c teoterm.c teotun.c teostream.c teol0cli.c

teomulti_t_LDFLAGS = $(AM_LDFLAGS) -pthread
teoackm_LDFLAGS = $(AM_LDFLAGS) -lev
teol0cli_LDFLAGS = $(AM_LDFLAGS) -lev 

SUBDIRS = fossa
