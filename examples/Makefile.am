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
AM_LDFLAGS += -L../../../libev-4.19 -L../../../confuse-2.7/src -L../../../libtuntap-master
endif

LIBS = -lteonet -lcrypto 
if TEO_THREAD	
LIBS += -pthread
endif

bin_PROGRAMS = teolarge teosend teomulti teomulti_t teotcp teotun teoterm

teolarge_SOURCES = teolarge.c
teosend_SOURCES = teosend.c
teomulti_t_SOURCES = teomulti_t.c
teomulti_SOURCES = teomulti.c
teotcp_SOURCES = teotcp.c
teotun_SOURCES = teotun.c
teoterm_SOURCES = teoterm.c

teomulti_t_LDFLAGS = $(AM_LDFLAGS) -pthread

SUBDIRS = fossa
