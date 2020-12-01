AM_CPPFLAGS = -I$(top_srcdir)/src
AM_CFLAGS = -Wall -Wpedantic -Werror -Wextra
AM_LDFLAGS = -static -lpthread

noinst_SCRIPTS = check-symbols regression gen-json-examples
CLEANFILES = $(noinst_SCRIPTS)  # for make clean to remove them

EXTRA_DIST = check-symbols.sh regression.sh

# need to add this rule (normally a default rule)
%: %.sh
	cat $< > $@
	chmod a+x $@

noinst_PROGRAMS = file-to-json

file_to_json_SOURCES = file-to-json.c
file_to_json_LDADD = ../src/libtinylogger.la
