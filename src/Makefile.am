EXTRA_DIST = logrotate.service alternatives/sd-daemon.h
EXTRA_DIST += linked-list.h.in linked-list.c.in
CLEANFILES = linked-list.h linked-list.c

# -Wshadow didn't pick up local timezone hiding global timezone from time.h
AM_CFLAGS = -Wall -Wpedantic -Werror -Wextra -Wshadow
AM_CPPFLAGS =

# for use with demo/json-timezones.c
# Makes src/json-formatter.c always update the Olson timezone string
# WARNING!!! Not for general use - only for demo/json-timezones!!!
#AM_CPPFLAGS += -DTIMEZONE_TEST

if SET_MAX_MSG_SIZE
AM_CPPFLAGS += -DMAX_MSG_SIZE=$(MAX_MSG_SIZE)
endif

lib_LTLIBRARIES = libtinylogger.la
include_HEADERS = tinylogger.h

libtinylogger_la_SOURCES = \
	tinylogger.c tinylogger.h \
	private.h \
	formatters.c \
	xml_formatter.c \
	json_formatter.c \
	hexformat.c \
	timezone.c

libtinylogger_la_CFLAGS = -pthread $(AM_CFLAGS)
libtinylogger_la_LDFLAGS = -version-info 0:0:0

# the linked-list source is now included in timezone.c - not separately compiled
#nodist_libtinylogger_la_SOURCES = linked-list.h linked-list.c
#BUILT_SOURCES = linked-list.h linked-list.c

# The recommendations generated by "./libtool --finish /usr/local/lib" are
# important, but if you are installing in /usr/local/lib and want to link
# and run as you would expect with system libs, use ldconfig without any args.
# ldconfig -n /usr/local/lib doesn't actually work. ldconfig (with no args)
# will work if /usr/local/lib is configured in /etc/ld.so.conf, possibly by
# /etc/ld.so.conf.d/libc.conf

LDCONFIG_REMINDER = "===== ldconfig -n /usr/local/lib doesn't work, use ldconfig with no args instead ====="

update_ldconfig:
	@if [ $(USER) = root ]; then ldconfig ; else echo $(LDCONFIG_REMINDER); fi

install-exec-hook: update_ldconfig

uninstall-hook: update_ldconfig

.PHONY: update_ldconfig
