## Process this file with automake to produce Makefile.in

include aminclude.am

ACLOCAL_AMFLAGS = -I m4

if DX_COND_man

# You'd probably want to post-process man pages and installed the patched
# versions.
#man3_MANS = @DX_DOCDIR@/man/man3/teonet.c.1
man3_MANS = @DX_DOCDIR@/man/man3/*.3
$(man3_MANS): doxygen-doc
DX_CLEANFILES = -rf @DX_DOCDIR@/*

endif

SUBDIRS = \
	embedded/libpbl/src \
	src \
	app \
	tests \
	examples \
	sh \
	po \
	# end of SUBDIRS

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = src/libteonet.pc

teonetdocdir = ${prefix}/doc/teonet
teonetdoc_DATA = \
	README \
	COPYING \
	AUTHORS \
	ChangeLog \
	INSTALL \
	NEWS \
	# end of teonetdoc_DATA

teonetsysconfdir = ${sysconfdir}/teonet

#teonetsysconf_DATA = conf/teonet.conf \
#		conf/teonet-8005.conf

INTLTOOL_FILES = intltool-extract.in \
	intltool-merge.in \
	intltool-update.in \
	# end of INTLTOOL_FILES

EXTRA_DIST = \
	$(teonetdoc_DATA) \
	$(INTLTOOL_FILES) \
	$(DX_CONFIG) \
	# tests/test-suite.sh \
	# end of EXTRA_DIST

DISTCLEANFILES = intltool-extract \
	intltool-merge \
	intltool-update \
	po/.intltool-merge-cache \
	@DX_DOCDIR@ \
	# end of DISTCLEANFILES

MOSTLYCLEANFILES = $(DX_CLEANFILES)


install-exec-hook:
	$(MKDIR_P) $(DESTDIR)$(libdir)/cmake/teonet
	cp -f cmake/libteonet-config.cmake $(DESTDIR)$(libdir)/cmake/teonet
	cp -f cmake/libteonet-config-version.cmake $(DESTDIR)$(libdir)/cmake/teonet

# Remove doc directory on un-install
uninstall-local:
	-rm -r $(DESTDIR)$(libdir)/cmake/teonet
	-rm -r $(teonetdocdir)
	-rm -r $(teonetsysconfdir)

run:	app/teovpn
	app/teovpn just-make --port=9057

build-tests:
	tests/test_teonet

test:	tests/test_teonet
	tests/test_teonet

deb-package:
	sh/make_libtuntap.sh
	sh/make_deb.sh @PACKAGE_VERSION@
