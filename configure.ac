dnl Process this file with autoconf to produce a configure script.
AC_PREREQ(2.52)

AC_INIT(mstardict, 0.2, roman.moravcik@gmail.com)
AC_CONFIG_SRCDIR(src/mstardict.cpp)

dnl Don't include maintainer make-rules by default
AM_MAINTAINER_MODE

AM_INIT_AUTOMAKE([dist-bzip2])

AM_CONFIG_HEADER(config.h)
AC_PROG_CC
AC_PROG_CPP
AC_PROG_CXX
AC_LANG([C++])
AC_PROG_CXXCPP
AC_PROG_MAKE_SET
AC_PROG_RANLIB

dnl ================================================================
dnl Gettext stuff.
dnl ================================================================
IT_PROG_INTLTOOL([0.35])
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION(0.14.1)
GETTEXT_PACKAGE=mstardict
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [Gettext package])
AM_ICONV

AC_FUNC_MMAP

AC_CHECK_LIB(z,zlibVersion,LIBS="$LIBS -lz",[AC_MSG_ERROR([zlib not found])])

PKG_CHECK_MODULES(MSTARDICT, glib-2.0 >= 2.6.1 hildon-1)

MSTARDICT_CFLAGS="-Wall $MSTARDICT_CFLAGS"

AC_SUBST(MSTARDICT_LIBS)
AC_SUBST(MSTARDICT_CFLAGS)

dnl fill cflags for libstardict
PKG_CHECK_MODULES(LIB_STARDICT, glib-2.0 >= 2.6.1 gtk+-2.0 >= 2.12 sigc++-2.0)
AC_SUBST(LIB_STARDICT_CFLAGS)

AC_OUTPUT([
 Makefile
 data/Makefile
 data/mstardict.desktop.in
 m4/Makefile
 src/lib/Makefile
 src/Makefile
 po/Makefile.in
])

echo "

Type \"make\" to compile.

"
