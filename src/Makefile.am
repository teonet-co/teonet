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
	 -I../embedded/teocli \
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
	# end of LIBS

#	../embedded/teocli/linux/.libs/libteocli.la 

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
#teobasedir = $(pkgincludedir)/teobase
#teoccldir = $(pkgincludedir)/teoccl
#teoclidir = $(pkgincludedir)/teocli

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
	../embedded/teocli/libteol0/teocli_api.h \
	../embedded/teocli/libteol0/teonet_l0_client_crypt.h \
	../embedded/teocli/libteol0/teonet_l0_client_options.h \
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


####################
teobasedir = $(pkgincludedir)/teobase
teobase_HEADERS = \
	$(top_srcdir)/embedded/teocli/libtrudp/libs/teobase/include/teobase/api.h \
	$(top_srcdir)/embedded/teocli/libtrudp/libs/teobase/include/teobase/logging.h \
	$(top_srcdir)/embedded/teocli/libtrudp/libs/teobase/include/teobase/mutex.h \
	$(top_srcdir)/embedded/teocli/libtrudp/libs/teobase/include/teobase/platform.h \
	$(top_srcdir)/embedded/teocli/libtrudp/libs/teobase/include/teobase/socket.h \
	$(top_srcdir)/embedded/teocli/libtrudp/libs/teobase/include/teobase/time.h \
  $(top_srcdir)/embedded/teocli/libtrudp/libs/teobase/include/teobase/types.h \
	$(top_srcdir)/embedded/teocli/libtrudp/libs/teobase/include/teobase/windows.h \
	# end of teobase_HEADERS

teoccldir = $(pkgincludedir)/teoccl
teoccl_HEADERS = \
	$(top_srcdir)/embedded/teocli/libtrudp/libs/teoccl/include/teoccl/array_list.h \
	$(top_srcdir)/embedded/teocli/libtrudp/libs/teoccl/include/teoccl/hash.h \
	$(top_srcdir)/embedded/teocli/libtrudp/libs/teoccl/include/teoccl/map.h \
	$(top_srcdir)/embedded/teocli/libtrudp/libs/teoccl/include/teoccl/memory.h \
	$(top_srcdir)/embedded/teocli/libtrudp/libs/teoccl/include/teoccl/queue.h \
	# end of teoccl_HEADERS

tinyaesdir = $(pkgincludedir)/libtinycrypt/tiny-AES-c/
tinyaes_HEADERS = \
	$(top_srcdir)/embedded/teocli/libtinycrypt/tiny-AES-c/aes.hpp \
	$(top_srcdir)/embedded/teocli/libtinycrypt/tiny-AES-c/aes.h \
	# end of tinyaes_HEADERS

tinyecdhdir = $(pkgincludedir)/libtinycrypt/tiny-ECDH-c/
tinyecdh_HEADERS = \
	$(top_srcdir)/embedded/teocli/libtinycrypt/tiny-ECDH-c/ecdh.h \
	# end of tinyecdh_HEADERS

tinycryptdir = $(pkgincludedir)/libtinycrypt
tinycrypt_HEADERS = \
	$(top_srcdir)/embedded/teocli/libtinycrypt/tinycrypt.h \
	# end of tinycrypt_HEADERS

####################


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
    ../embedded/teocli/libteol0/teonet_l0_client.c \
    ../embedded/teocli/libteol0/teonet_l0_client_options.c \
    ../embedded/teocli/libteol0/teonet_l0_client_crypt.c \
    \
    ../embedded/teocli/libtinycrypt/tinycrypt.c \
    ../embedded/teocli/libtinycrypt/tiny-AES-c/aes.c \
    ../embedded/teocli/libtinycrypt/tiny-ECDH-c/ecdh.c \
    \
    ../embedded/teocli/libtrudp/libs/teobase/src/teobase/logging.c \
    ../embedded/teocli/libtrudp/libs/teobase/src/teobase/mutex.c \
    ../embedded/teocli/libtrudp/libs/teobase/src/teobase/socket.c \
    ../embedded/teocli/libtrudp/libs/teobase/src/teobase/time.c \
    \
    ../app/embedded/jsmn/jsmn.c \
    \
    ../embedded/teocli/libtrudp/libs/teoccl/src/teoccl/map.c \
    ../embedded/teocli/libtrudp/libs/teoccl/src/teoccl/hash.c \
    ../embedded/teocli/libtrudp/libs/teoccl/src/teoccl/queue.c \
    ../embedded/teocli/libtrudp/libs/teoccl/src/teoccl/memory.c \
    ../embedded/teocli/libtrudp/src/packet.c \
    ../embedded/teocli/libtrudp/src/packet_queue.c \
    ../embedded/teocli/libtrudp/src/trudp.c \
    ../embedded/teocli/libtrudp/src/trudp_options.c \
    ../embedded/teocli/libtrudp/src/trudp_channel.c \
    ../embedded/teocli/libtrudp/src/trudp_receive_queue.c \
    ../embedded/teocli/libtrudp/src/trudp_send_queue.c \
    ../embedded/teocli/libtrudp/src/trudp_stat.c \
    ../embedded/teocli/libtrudp/src/trudp_utils.c \
    ../embedded/teocli/libtrudp/src/udp.c \
    ../embedded/teocli/libtrudp/src/write_queue.c \
    ../embedded/teocli/libtrudp/src/trudp_ev.c \
    # end of libteonet_la_SOURCES


teonetconfdir=$(sysconfdir)/teonet
teonetconf_DATA = config/teonet.conf.default

# Extra distributive
EXTRA_DIST = config/teonet.conf.default ../docs/man/man3

libteonet_la_LDFLAGS = $(AM_LDFLAGS) \
	-version-info  $(LIBRARY_CURRENT):$(LIBRARY_REVISION):$(LIBRARY_AGE) \
	# end of libteonet_la_LDFLAGS

uninstall-local:
	-rm -r $(teonetincludedir)
	-rm -r $(teobaseincludedir)
	-rm -r $(teocclincludedir)

