#
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT(teoasync_c, 0.0.1, [kirill@scherba.ru])
AC_DEFINE([PACKAGE_DESCRIPTION], ["Teonet async test application"], [Application description])
AC_DEFINE([PACKAGE_DEPENDENCIES], ["libteonet-dev"], [Application dependencies])
AC_CONFIG_SRCDIR([src/main.c])
AC_CONFIG_HEADERS([src/config.h])

m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])
#AM_INIT_AUTOMAKE(AC_PACKAGE_NAME, AC_PACKAGE_VERSION)
AM_INIT_AUTOMAKE()
AM_MAINTAINER_MODE

# Checks for programs.
AC_PROG_CXX
AC_PROG_AWK
AC_PROG_CC
AC_PROG_CPP
AC_PROG_INSTALL
AC_PROG_LN_S
AC_PROG_MAKE_SET
AC_PROG_RANLIB

# Checks for libraries.
AC_CHECK_LIB([teonet], [ksnetEvMgrInitPort])
AC_CHECK_LIB([crypto], [OPENSSL_config])

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h unistd.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

# Checks for library functions.

# ***************************************************************************
# Internatinalization
# ***************************************************************************
GETTEXT_PACKAGE=teoasync_c
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [GETTEXT package name])
AM_GLIB_GNU_GETTEXT
IT_PROG_INTLTOOL([0.35.0])

# ***************************************************************************
# Documentation
# ***************************************************************************

# Checks for doxygen
AC_CHECK_PROG(HAVE_DOXYGEN, doxygen, true, false)
AM_CONDITIONAL(HAVE_DOXYGEN, $HAVE_DOXYGEN)
if test $HAVE_DOXYGEN = "false"; then
    AC_MSG_WARN([*** doxygen not found, docs will not be built])
else

DX_HTML_FEATURE(ON)
DX_CHM_FEATURE(OFF)
DX_CHI_FEATURE(OFF)
DX_MAN_FEATURE(OFF)
DX_RTF_FEATURE(OFF)
DX_XML_FEATURE(OFF)
DX_PDF_FEATURE(OFF)
DX_PS_FEATURE(OFF)
DX_INIT_DOXYGEN([teoasync_c], Doxyfile, docs)

fi

# ***************************************************************************
# Debug
# ***************************************************************************

AC_ARG_ENABLE(debug,
AS_HELP_STRING([--enable-debug],
               [enable debugging, default: no]),
[case "${enableval}" in
             yes) debug=true ;;
             no)  debug=false ;;
             *)   AC_MSG_ERROR([bad value ${enableval} for --enable-debug]) ;;
esac],
[debug=false])

AM_CONDITIONAL(DEBUG, test x"$debug" = x"true")

# ***************************************************************************

AM_PROG_LIBTOOL


# PKG_CHECK_MODULES(TEOASYNC_C, [glib-2.0 ])

AC_OUTPUT([
Makefile
src/Makefile
po/Makefile.in
])
