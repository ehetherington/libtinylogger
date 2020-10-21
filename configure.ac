#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([tinylogger], [0.6.0], [ejh.1957.eh@gmail.com])
#AC_CONFIG_HEADERS([config.h])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_SRCDIR([src])

LT_INIT([dlopen])
#AC_PROG_RANLIB

# use automake
#AM_INIT_AUTOMAKE
# added -Wno-portablilty to quiet "non-POSIX variable name" warnings
AM_INIT_AUTOMAKE([-Wno-portability subdir-objects foreign])


# Configure option: --enable-timezone[=no].
# appears in config.h
AC_ARG_ENABLE([timezone],
    [AS_HELP_STRING([--enable-timezone],
    [append timezone to JSON format timestamps (default is no)])],
    [enable_timezone=yes], [enable_timezone=no])
AS_IF([test "x$enable_timezone" = xyes],
    AC_DEFINE([ENABLE_TIMEZONE], [1],
		[Define to 1 to enable timezone in JSON timestamps])
    )

# used in src/Makefile.am
# for quick-start, set in quick-start/config.h
AC_ARG_VAR([MAX_MSG_SIZE], [set maximum message size, setting it to 0 means no limit])
AM_CONDITIONAL([SET_MAX_MSG_SIZE], [test -n "$MAX_MSG_SIZE"])

# appears in config.h
AC_ARG_VAR([ZONEINFO_DIR], [zoneinfo database dir])
if test -n "$ZONEINFO_DIR"; then
	quoted_dir="\"$ZONEINFO_DIR\""
	AC_DEFINE_UNQUOTED([ZONEINFO_DIR], $quoted_dir,
		[define to override default of /usr/share/zoneinf0])
fi

# Checks for programs.
AC_PROG_CC

AC_CHECK_PROG([javac])
AM_CONDITIONAL([HAVE_JAVAC], [test -n "$javac"])

AC_CHECK_PROG([doxygen])
AM_CONDITIONAL([HAVE_DOXYGEN], [test -n "$doxygen"])


# Checks for libraries.

# Checks for header files.
#AC_CHECK_HEADERS([arpa/inet.h netdb.h netinet/in.h stdlib.h string.h sys/socket.h sys/timeb.h unistd.h])
AC_CHECK_HEADERS([systemd/sd-daemon.h])

# Checks for typedefs, structures, and compiler characteristics.
#AC_CHECK_HEADER_STDBOOL

# Checks for library functions.
#AC_FUNC_ERROR_AT_LINE
#AC_CHECK_FUNCS([bzero ftime gethostbyaddr gethostbyname inet_ntoa socket])

#dll_prefix="dll_"
#AC_SUBST([dll_prefix])

#AC_SUBST([dll_prefix], ["log_"])
#AC_SUBST([dll_storage_class], ["static "])

AC_SUBST([dll_prefix], [""])
# remember trailing ' ' for dll_storage_class
AC_SUBST([dll_storage_class], ["static "])
AC_SUBST([dll_data_type], ["char *"])

AC_CONFIG_FILES(Makefile src/Makefile \
	demo/Makefile guide/Makefile doc/Makefile utils/Makefile \
	doc/doxygen.conf \
	src/linked-list.h src/linked-list.c)

AC_OUTPUT
