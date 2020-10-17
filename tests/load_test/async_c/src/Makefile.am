## Process this file with automake to produce Makefile.in
#

localteonet   = $(HOME)/Projects/teonet

teonetinclude = -I$(localteonet)/src \
		-I$(localteonet)/src/config \
                -I$(localteonet)/embedded/libpbl/src \
	        -I$(localteonet)/embedded/teocli/libteol0 \
	        -I$(localteonet)/embedded/libtrudp/src \
		-I$(localteonet)/embedded/libtrudp/embedded/teoccl/src \
	        -I$(includedir)/teonet \
		-I$(includedir)/teonet/config \
		-I$(includedir)/libccl \
		-I$(includedir)/libtrudp	
	
teonetlib     = -L$(localteonet)/src/

AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" $(TEOWGMASTER_CFLAGS)

AM_CFLAGS = \
	 -Wall \
	 -g \
	 -std=c11 \
	 $(teonetinclude)

AM_LDFLAGS = \
 	 -L$(localteonet)/src/

AM_CXX = clang++
AM_CXXFLAGS = \
         -g -Wall \
	 -std=c++17 \
	 $(teonetinclude)

## Debug info
if DEBUG
  AM_CFLAGS += -g3 -O0
  AM_CXXFLAGS += -g3 -O0
else
  AM_CFLAGS += -O2
  AM_CXXFLAGS += -O2
endif

LIBS = -pthread -lteonet

bin_PROGRAMS = teoasync_c 

teoasync_c_SOURCES = \
	main.cpp 

#teoasync_c_LDFLAGS =

teoasync_c_LDADD = $(teoasync_c_LIBS)
