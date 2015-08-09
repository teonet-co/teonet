## Process this file with automake to produce Makefile.in

include aminclude.am

if DX_COND_man

# You'd probably want to post-process man pages and installed the patched
# versions.
#man3_MANS = @DX_DOCDIR@/man/man3/teonet.c.1
#$(man3_MANS): doxygen-doc
#DX_CLEANFILES = -rf @DX_DOCDIR@/*

endif

SUBDIRS = embedded/libpbl/src src po tests examples app  #src/app src/libtuntap src/ksvpn src po dist examples tests ascripts conf

teonetdocdir = ${prefix}/doc/teonet
teonetdoc_DATA = \
	README\
	COPYING\
	AUTHORS\
	ChangeLog\
	INSTALL\
	NEWS

teonetsysconfdir = ${sysconfdir}/teonet
#teonetsysconf_DATA = conf/teonet.conf \
#		     conf/teonet-8005.conf

INTLTOOL_FILES = intltool-extract.in \
	intltool-merge.in \
	intltool-update.in

EXTRA_DIST = $(teonetdoc_DATA) \
	$(INTLTOOL_FILES) \
	tests/test-suite.sh \
	$(DX_CONFIG)

DISTCLEANFILES = intltool-extract \
	intltool-merge \
	intltool-update \
	po/.intltool-merge-cache \
	@DX_DOCDIR@

MOSTLYCLEANFILES = $(DX_CLEANFILES)

# Remove doc directory on un-install
uninstall-local:
	-rm -r $(teonetdocdir)
	-rm -r $(teonetsysconfdir)

run:	app/teovpn
	app/teovpn just-make --port=9057 --show_peers
	
build-tests: tests/test_teonet
	
test:	tests/test_teonet
	tests/test_teonet
