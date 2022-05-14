AC_INIT([g15stats],[3.0],[dmenelkir@gmail.com])

PACKAGE=g15stats
VERSION=3.0.1
AC_PREFIX_DEFAULT(/usr)
AC_CONFIG_AUX_DIR(config)
AM_INIT_AUTOMAKE($PACKAGE, $VERSION)
AC_CONFIG_HEADERS([config.h])
 
AC_PROG_CC
LT_INIT
AC_PROG_RANLIB

AC_CHECK_LIB([g15daemon_client], [g15daemon_version],,AC_MSG_ERROR(["libg15daemon_client (or its devel package) not found. please install it"]))
AC_CHECK_INCLUDES_DEFAULT
AC_PROG_EGREP

AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS(g15daemon_client.h,,AC_MSG_ERROR(["libg15daemon_client (or its devel package) could not be found.  please install it"]))
AC_CHECK_HEADERS(libg15render.h,,AC_MSG_ERROR(["libg15render (or its devel package) could not be found.  please install it"]))

PKG_CHECK_MODULES(GTOP,libgtop-2.0,,)
AC_ARG_WITH(gtop-includes,AS_HELP_STRING([--with-gtop-includes],[Path to libgtop includes (Autodetect)]),GTOP_CFLAGS="-I$withval",)
AC_ARG_WITH(gtop-libs,AS_HELP_STRING([--with-gtop-libs],[Path to libgtop libraries (Autodetect)]),GTOP_LIBS="-L$withval -lgtop-2.0 -lglib-2.0",)

CPPFLAGS="$CPPFLAGS $GTOP_CFLAGS"
CFLAGS="$CPPFLAGS $GTOP_CFLAGS"
LDFLAGS="$LDFLAGS $GTOP_LIBS"

AC_CONFIG_FILES([Makefile])
AC_OUTPUT
