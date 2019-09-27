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
	 -I../embedded/teocli/libtrudp/libs/teobase/libs/teoccl/include \
	 -I/usr/include/libtrudp \
	 -I../embedded/teocli \
	 -I/usr/include/libev
	
## Debug info
if DEBUG
  AM_CFLAGS += -g3 -O0
#  AM_CXXFLAGS += -g3 -O0
else
  AM_CFLAGS += -O2
#  AM_CXXFLAGS += -O2
endif	
	
if TEO_THREAD	
AM_CFLAGS += -DTEO_THREAD
endif

#if MINGW
#AM_CFLAGS += -I../../../confuse-2.7/src -I../../../libev-4.19/ -I../../../libtuntap-master
#else
#AM_CFLAGS += -fPIC
#endif

AM_LDFLAGS =  -L/opt/local/lib -L../src

#if MINGW
#AM_LDFLAGS += -L../../../confuse-2.7/src -L../../../libev-4.19 -L../../../libtuntap-master
#endif

LIBS = -lteonet -lcrypto
if TEO_THREAD	
LIBS += -pthread
endif

bin_PROGRAMS = teovpn teodb teoweb teodsp teogw

teovpn_SOURCES = teovpn.c ../src/libteonet.la
teodb_SOURCES = teodb.c ../src/modules/teodb_com.c
teoweb_SOURCES = teoweb.c \
		 modules/teo_auth/teo_auth.c modules/teo_web/teo_web_conf.c \
		 modules/teo_web/teo_web.c modules/teo_web/teo_ws.c \
		 embedded/mongoose/mongoose.c
teodsp_SOURCES = teodsp.c
teogw_SOURCES = teogw.c

teoweb_LDFLAGS = $(AM_LDFLAGS) -pthread -lev -lconfuse -lcurl
teodsp_LDFLAGS = $(AM_LDFLAGS) -lev

EXTRA_DIST = modules/teo_web/teo_ws.h \
	     modules/teo_web/teo_web.h \
	     modules/teo_web/teo_web_conf.h \
	     modules/teo_auth/teo_auth.h \
	     embedded/mongoose/mongoose.h
	