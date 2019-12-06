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
	-I/usr/include/libtrudp \
	-I/usr/include/libev \
	# end of AM_CFLAGS

if TEO_THREAD	
AM_CFLAGS += -DTEO_THREAD
endif

AM_LDFLAGS =  -L/opt/local/lib -L../src

LIBS = -lteonet -lcrypto
if TEO_THREAD	
LIBS += -pthread
endif

bin_PROGRAMS =

bin_PROGRAMS += teovpn
teovpn_SOURCES = teovpn.c ../src/libteonet.la

bin_PROGRAMS += teodb
teodb_SOURCES = teodb.c ../src/modules/teodb_com.c

bin_PROGRAMS += teoweb
teoweb_LDFLAGS = $(AM_LDFLAGS) \
	-pthread -lev -lconfuse -lcurl \
	../embedded/teocli/linux/libteocli.la \
	#end of teoweb_LDFLAGS

teoweb_SOURCES = teoweb.c \
	modules/teo_auth/teo_auth.c \
	modules/teo_web/teo_web_conf.c \
	modules/teo_web/teo_web.c \
	modules/teo_web/teo_ws.c \
	embedded/mongoose/mongoose.c \
	# end of teoweb_SOURCES


bin_PROGRAMS += teodsp
teodsp_LDFLAGS = $(AM_LDFLAGS) -lev ../embedded/teocli/linux/libteocli.la
teodsp_SOURCES = teodsp.c

bin_PROGRAMS += teogw
teogw_SOURCES = teogw.c

EXTRA_DIST = modules/teo_web/teo_ws.h \
	modules/teo_web/teo_web.h \
	modules/teo_web/teo_web_conf.h \
	modules/teo_auth/teo_auth.h \
	embedded/mongoose/mongoose.h \
	#end of EXTRA_DIST
