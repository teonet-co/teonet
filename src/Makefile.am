# bin_PROGRAMS = ksmqtt_broker echo_server echo_client timer_watcher
#
# AM_CFLAGS = -I../include

# ksmqtt_broker_SOURCES = ksmqtt_broker.c
# timer_watcher_SOURCES = timer_watcher.c
# echo_serv_SOURCES = echo_serv.c
# echo_client_SOURCES = echo_client.c
#
# LIBS = -L../lib ../lib/libev.a -lm

# SUBDIRS = app

pkgconfigdir = $(libdir)/pkgconfig
#nodist_pkgconfig_DATA = libteonet.pc

if RELEASE_TEONET
  DRELEASE_TEONET = -DRELEASE_TEONET=1
else
  DRELEASE_TEONET =
endif

AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(localedir)"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(pkgdatadir)"\"

AM_CFLAGS = \
	 -Wall \
	 -g \
	 -I/opt/local/include \
	 -DTEONET_SYS_CONFIG_DIR=\""${sysconfdir}/teonet"\" \
	 $(DRELEASE_TEONET) \
	 -I../embedded/libpbl/src/

if TEO_THREAD
AM_CFLAGS += -DTEO_THREAD
endif

if MINGW
AM_CFLAGS += -I../../libev-4.19 -I../../libtuntap-master
else
AM_CFLAGS += -fPIC
endif

AM_CXX = clang++
AM_CXXFLAGS = \
	-std=c++11 \
	-fPIC \
	-Wall \
	-g \
	-I/opt/local/include

AM_LDFLAGS = -L/opt/local/lib

LIBS = -lev -ltuntap ../embedded/libpbl/src/libpbl.a

# Not embed confuse flag
if NOT_E_CONFUSE
AM_CFLAGS += -I../../confuse-2.7/src
LIBS += -lconfuse
endif

# Mac OS only
if DARWIN
else
LIBS += -luuid -lcrypt
endif

lib_LTLIBRARIES = libteonet.la

## Debug info
if DEBUG
  AM_CFLAGS += -g3 -O0
  AM_CXXFLAGS += -g3 -O0
else
  AM_CFLAGS += -O2
  AM_CXXFLAGS += -O2
endif

localedir = $(datadir)/locale
DEFS = @DEFS@ -DLOCALEDIR=\"${localedir}\"

#nodist_include_HEADERS = config/config.h.in
teonetincludedir=$(includedir)/teonet

#teonetinclude_HEADERS = 
nobase_teonetinclude_HEADERS = \
    crypt.h \
    daemon.h \
    ev_mgr.h \
    hotkeys.h \
    net_arp.h \
    net_com.h \
    net_core.h \
    net_multi.h \
    net_split.h \
    tr-udp.h \
    tr-udp_.h \
    tr-udp_stat.h \
    pidfile.h \
    teonet.h \
    config/config.h \
    config/conf.h \
    config/opt.h \
    modules/cque.h \
    modules/net_cli.h \
    modules/net_tcp.h \
    modules/net_term.h \
    modules/net_tun.h \
    modules/tcp_proxy.h \
    modules/teodb.h \
    modules/vpn.h \
    utils/string_arr.h \
    utils/utils.h \
    utils/rlutil.h

teonetinclude_HEADERS = \
    ../embedded/libpbl/src/pbl.h

libteonet_la_SOURCES = \
    crypt.c \
    daemon.c \
    ev_mgr.c \
    hotkeys.c \
    net_arp.c \
    net_com.c \
    net_core.c \
    net_multi.c \
    net_split.c \
    tr-udp.c \
    tr-udp_stat.c \
    pidfile.c \
    teonet.c \
    config/conf.c \
    config/opt.c \
    modules/cque.c \
    modules/net_cli.c \
    modules/net_tcp.c \
    modules/net_term.c \
    modules/net_tun.c \
    modules/tcp_proxy.c \
    modules/teodb.c \
    modules/vpn.c \
    utils/string_arr.c \
    utils/utils.c

teonetconfdir=$(sysconfdir)/teonet
teonetconf_DATA = config/teonet.conf.default

# Extra distributive
EXTRA_DIST = config/teonet.conf.default ../docs/man/man3

# Not embed confuse flag
if NOT_E_CONFUSE
else
#libteonet_la_SOURCES += ../lib/confuse/confuse.c lexer.l
endif

# AM_LFLAGS=-Pcfg_yy -olex.yy.c
#lexer.c: lexer.l

libteonet_la_LDFLAGS = $(AM_LDFLAGS) -version-info 11:0:0 

uninstall-local:
	-rm -r $(teonetincludedir)
