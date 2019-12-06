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
	-DPACKAGE_DATA_DIR=\""$(pkgdatadir)"\" \
	# end of AM_CPPFLAGS

AM_CFLAGS = \
	 -std=gnu11 \
	 -Wall \
	 -Wno-unused-function \
	 -I/opt/local/include \
	 -DUSE_LIBEV=1 \
	 -DTEONET_SYS_CONFIG_DIR=\""${sysconfdir}/teonet"\" \
	 $(DRELEASE_TEONET) \
	 -I../embedded/libpbl/src \
	 -I../embedded/teocli/libteol0 \
	 -I../embedded/teocli/libtrudp/src \
	 -I../embedded/teocli/libtrudp/libs/teobase/include \
	 -I../embedded/teocli/libtrudp/libs/teoccl/include \
	 -I../app/embedded/jsmn \
	 -I/usr/include/libev \
	 -DEV_USE_EPOLL=1 \
	 -fno-strict-aliasing \
	 # end of AM_CFLAGS

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
	-std=c++17 \
	-fPIC \
	-Wall \
	-DUSE_LIBEV=1 \
	-I/opt/local/include \
	# end of AM_CXXFLAGS

AM_LDFLAGS = -L/opt/local/lib

LIBS = -lev -lcrypt -ltuntap \
	../embedded/libpbl/src/libpbl.a \
	../embedded/teocli/linux/.libs/libteocli.la \
	# end of LIBS

# Not embed confuse flag
if NOT_E_CONFUSE
AM_CFLAGS += -I../../confuse-2.7/src
LIBS += -lconfuse
endif

# Mac OS only
if DARWIN
else
LIBS += -luuid -lcrypto
endif

lib_LTLIBRARIES = libteonet.la

localedir = $(datadir)/locale
DEFS = @DEFS@ -DLOCALEDIR=\"${localedir}\"

#nodist_include_HEADERS = config/config.h.in
teonetincludedir=$(includedir)/teonet
#teobaseincludedir=$(includedir)/teobase
#teocclincludedir=$(includedir)/teoccl
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
    net_recon.h \
    net_split.h \
    tr-udp.h \
    tr-udp_.h \
    tr-udp_stat.h \
    pidfile.h \
    teonet.hpp \
    config/config.h \
    config/conf.h \
    config/opt.h \
    modules/cque.h \
    modules/l0-server.h \
    modules/net_cli.h \
    modules/net_tcp.h \
    modules/net_term.h \
    modules/net_tun.h \
    modules/stream.h \
    modules/subscribe.h \
    modules/tcp_proxy.h \
    modules/teodb.h \
    modules/teodb_com.h \
    modules/vpn.h \
    modules/logging_server.h \
    modules/logging_client.h \
    modules/async_calls.h \
    modules/log_reader.h \
    modules/metric.h \
    utils/teo_memory.h \
    utils/string_arr.h \
    utils/utils.h \
    utils/rlutil.h \
		# end of nobase_teonetinclude_HEADERS

teonetinclude_HEADERS = \
	../embedded/libpbl/src/pbl.h \
	../embedded/teocli/libteol0/teonet_l0_client.h \
	../app/embedded/jsmn/jsmn.h \
	\
	../embedded/teocli/libtrudp/src/udp.h \
	../embedded/teocli/libtrudp/src/trudp.h \
	../embedded/teocli/libtrudp/src/packet.h \
	../embedded/teocli/libtrudp/src/trudp_api.h \
	../embedded/teocli/libtrudp/src/trudp_const.h \
	../embedded/teocli/libtrudp/src/write_queue.h \
	../embedded/teocli/libtrudp/src/packet_queue.h \
	../embedded/teocli/libtrudp/src/trudp_channel.h \
	../embedded/teocli/libtrudp/src/trudp_send_queue.h \
	../embedded/teocli/libtrudp/src/trudp_utils.h \
	../embedded/teocli/libtrudp/src/trudp_receive_queue.h \
	# end of teonetinclude_HEADERS


libteonet_la_SOURCES = \
    crypt.c \
    daemon.c \
    ev_mgr.c \
    hotkeys.c \
    net_arp.c \
    net_com.c \
    net_core.c \
    net_multi.c \
    net_recon.c \
    net_split.c \
    tr-udp.c \
    tr-udp_stat.c \
    pidfile.c \
    config/conf.c \
    config/opt.c \
    modules/cque.c \
    modules/l0-server.c \
    modules/net_cli.c \
    modules/net_tcp.c \
    modules/net_term.c \
    modules/net_tun.c \
    modules/stream.c \
    modules/subscribe.c \
    modules/tcp_proxy.c \
    modules/teodb.c \
    modules/teodb_com.c \
    modules/vpn.c \
    modules/logging_server.c \
    modules/logging_client.c \
    modules/async_calls.c \
    modules/log_reader.c \
    modules/metric.c \
    utils/teo_memory.c \
    utils/string_arr.c \
    utils/utils.c \
    utils/base64.c \
    ../app/embedded/jsmn/jsmn.c \
		# end of libteonet_la_SOURCES


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

libteonet_la_LDFLAGS = $(AM_LDFLAGS) \
	-version-info  $(LIBRARY_CURRENT):$(LIBRARY_REVISION):$(LIBRARY_AGE) \
	# 49:0:0 \
	# end of libteonet_la_LDFLAGS

uninstall-local:
	-rm -r $(teonetincludedir)
	-rm -r $(teobaseincludedir)
	-rm -r $(teocclincludedir)

