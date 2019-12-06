AM_CFLAGS = \
	 -fPIC \
	 -Wall \
	 -I/opt/local/include \
	 -DTEONET_SYS_CONFIG_DIR=\""${sysconfdir}/teonet"\" \
	 $(DRELEASE_TEONET) \
	 -I../src \
	 -I../embedded/libpbl/src \
	 -I../embedded/teocli/libteol0 \
	 -I../embedded/teocli/libtrudp/src \
	 -I../embedded/teocli/libtrudp/libs/teobase/include \
	 -I../embedded/teocli/libtrudp/libs/teoccl/include \
	 -I/usr/include/libtrudp \
	 -I/usr/include/libev

AM_CXXFLAGS = \
	 -g -Wall \
	 -std=c++14 \
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
AM_LDFLAGS += -L../../../libev-4.19 -L../../../confuse-2.7/src -L../../../libtuntap-master
endif

LIBS = \
	../src/libteonet.la \
	../embedded/teocli/linux/libteocli.la \
	-lcrypto

if TEO_THREAD
LIBS += -pthread
endif

noinst_PROGRAMS = teolarge teosend teomulti teomulti_t teotcp teotun teoterm \
	  teoack teoackm teocque teodb_ex teostream teol0cli \
	  teosscr teotru_load teocpp teocpp_2 teocquecpp teocquecpp_2 \
	  teodb_ex_cpp teodb_ex_cpp_2

# noinst_PROGRAMS += teol0cli_n teol0cli_ns
# teol0cli_n_SOURCES = ../embedded/teocli/main.c
# teol0cli_ns_SOURCES = ../embedded/teocli/main_select.c
# teol0cli_n_LDFLAGS = $(AM_LDFLAGS) -lev
# teol0cli_ns_LDFLAGS = $(AM_LDFLAGS) -lev

teoack_SOURCES = teoack.c
teoackm_SOURCES = teoackm.c
teocque_SOURCES = teocque.c
teocquecpp_SOURCES = teocquecpp.cpp
teocquecpp_2_SOURCES = teocquecpp_2.cpp
teodb_ex_SOURCES = teodb_ex.c ../src/modules/teodb_com.c
teodb_ex_cpp_SOURCES = teodb_ex_cpp.cpp ../src/modules/teodb_com.c
teodb_ex_cpp_2_SOURCES = teodb_ex_cpp_2.cpp ../src/modules/teodb_com.c
teolarge_SOURCES = teolarge.c
teomulti_SOURCES = teomulti.c
teomulti_t_SOURCES = teomulti_t.c
teosend_SOURCES = teosend.c
teotcp_SOURCES = teotcp.c
teoterm_SOURCES = teoterm.c
teotun_SOURCES = teotun.c
teostream_SOURCES = teostream.c
teol0cli_SOURCES = teol0cli.c
teosscr_SOURCES = teosscr.c
teotru_load_SOURCES = teotru_load.c
teocpp_SOURCES = teocpp.cpp
teocpp_2_SOURCES = teocpp_2.cpp

exampledir = $(datarootdir)/doc/@PACKAGE@/examples
example_DATA = teoack.c teoackm.c teocque.c teolarge.c teomulti.c teomulti_t.c \
	       teosend.c teotcp.c teoterm.c teotun.c teostream.c teol0cli.c \
	       teol0cli.py teosscr.c teotru_load.c teodb_ex.c teocpp.cpp teocpp_2.cpp \
	       teocquecpp teocquecpp_2 teodb_ex_cpp.cpp teodb_ex_cpp_2.cpp

teotru_load_CFLAGS = $(AM_CFLAGS) -std=c11

teomulti_t_LDFLAGS = $(AM_LDFLAGS) -pthread
teoackm_LDFLAGS = $(AM_LDFLAGS) -lev
teol0cli_LDFLAGS = $(AM_LDFLAGS) -lev

SUBDIRS = fossa
