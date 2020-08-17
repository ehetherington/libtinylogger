#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([tinylogger], [0.5.0], [ejh.1957.eh@gmail.com])
#AC_CONFIG_HEADERS([config.h])
AC_CONFIG_HEADER([config.h])
AC_CONFIG_SRCDIR([src])

LT_INIT([dlopen])
#AC_PROG_RANLIB

# use automake
#AM_INIT_AUTOMAKE
# added -Wno-portablilty to quiet "non-POSIX variable name" warnings
AM_INIT_AUTOMAKE([-Wno-portability subdir-objects foreign])

# Checks for programs.
AC_PROG_CC

# Checks for libraries.

# Checks for header files.
#AC_CHECK_HEADERS([arpa/inet.h netdb.h netinet/in.h stdlib.h string.h sys/socket.h sys/timeb.h unistd.h])
AC_CHECK_HEADERS([systemd/sd-daemon.h])

# Checks for typedefs, structures, and compiler characteristics.
#AC_CHECK_HEADER_STDBOOL

# Checks for library functions.
#AC_FUNC_ERROR_AT_LINE
#AC_CHECK_FUNCS([bzero ftime gethostbyaddr gethostbyname inet_ntoa socket])

AC_ARG_VAR([MAX_MSG_SIZE], [set maximum message size, setting it to 0 means no limit])
#AM_CONDITIONAL([SET_MAX_MSG_SIZE], [test x$MAX_MSG_SIZE != x])
AM_CONDITIONAL([SET_MAX_MSG_SIZE], [test -n "$MAX_MSG_SIZE"])

#AC_SUBST([top_srcdir])
AC_CONFIG_FILES(Makefile src/Makefile \
	demo/Makefile guide/Makefile doc/Makefile \
	doc/doxygen.conf)

AC_OUTPUT
