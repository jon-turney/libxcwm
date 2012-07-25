# Copyright (c) 2012 Jess VanDerwalker <washu@sonic.net>

AC_PREREQ(2.61)
AC_INIT([libxcwm], [0.1])
AM_INIT_AUTOMAKE([foreign dist-bzip2])
AM_MAINTAINER_MODE
AM_CONFIG_HEADER(config.h)

# Initialize libtool
AC_PROG_LIBTOOL

# Require xorg-macros: XORG_DEFAULT_OPTIONS BASE_CFLAGS
m4_ifndef([XORG_MACROS_VERSION],
          [m4_fatal([must install xorg-macros 1.16 or later before running autoconf/autogen])])
XORG_MACROS_VERSION(1.16)
XORG_DEFAULT_OPTIONS

AC_PROG_CC
AC_PROG_OBJC
AC_PROG_INSTALL

# Checks for libraries.
PKG_CHECK_MODULES(XCB, xcb-damage xcb-composite xcb-event xcb-xtest xcb-image xcb-keysyms xcb-icccm xcb-atom xcb-ewmh)

AC_CHECK_FUNC(dispatch_async,
              AC_DEFINE([HAVE_LIBDISPATCH], 1, [Define to 1 if you have the libdispatch (GCD) available])
              [])

AC_ARG_ENABLE(xtoq, AS_HELP_STRING([--enable-xtoq], [Build XtoQ.app for OS X (default: auto)]), [XTOQ=$enableval], [XTOQ=auto])
AC_MSG_CHECKING([if we should build XtoQ.app])
if test "x$XTOQ" = "xauto" ; then
    case $host_os in
        # darwin10 (Snow Leopard) and later is required for XtoQ
        darwin1*) XTOQ=yes ;;
        *) XTOQ=no ;;
    esac
fi
AC_MSG_RESULT($XTOQ)
AM_CONDITIONAL(XTOQ, [test "x$XTOQ" = "xyes"])

AC_CONFIG_FILES([Makefile
                 include/Makefile
                 man/Makefile
                 src/libxcwm/Makefile
                 src/Makefile
                 src/xtoq/bundle/Makefile
                 src/xtoq/Makefile])
AC_OUTPUT
