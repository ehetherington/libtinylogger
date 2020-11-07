AM_CPPFLAGS = -I$(top_srcdir)/src
AM_CFLAGS = -Wall -Wpedantic -Werror -Wextra
AM_LDFLAGS = -static -lpthread

COMMON_LIBS = ../src/libtinylogger.la libdemo.a

noinst_LIBRARIES = libdemo.a

libdemo_a_SOURCES = \
	demo-utils.h \
	demo-utils.c
#libdemo_a_CFLAGS = 

noinst_PROGRAMS = \
	hello_world \
	second \
	formats \
	stream-of-logs \
	levels \
	threads \
	beehive \
	xml \
	json \
	xml-levels \
	logrotate \
	clocks \
	log_mem \
	check-timezone \
	json-timezones \
	perf-test

JAVAROOT = .
if HAVE_JAVAC
dist_noinst_JAVA = ParseXMLDate.java
endif

hello_world_SOURCES = hello_world.c
hello_world_LDADD = ../src/libtinylogger.la

log_mem_SOURCES = log_mem.c
log_mem_LDADD = ../src/libtinylogger.la

second_SOURCES = second.c
second_LDADD = ../src/libtinylogger.la

formats_SOURCES = formats.c
formats_LDADD = ../src/libtinylogger.la

stream_of_logs_SOURCES = stream-of-logs.c
stream_of_logs_LDADD = ../src/libtinylogger.la

threads_SOURCES = threads.c
threads_LDADD = $(COMMON_LIBS)

logrotate_SOURCES = logrotate.c
logrotate_LDADD = $(COMMON_LIBS)

beehive_SOURCES = beehive.c
beehive_LDADD = $(COMMON_LIBS)

xml_SOURCES = xml.c
xml_LDADD = $(COMMON_LIBS)

json_SOURCES = json.c
json_LDADD = $(COMMON_LIBS)

levels_SOURCES = levels.c
levels_LDADD = ../src/libtinylogger.la

xml_levels_SOURCES = xml-levels.c
xml_levels_LDADD = ../src/libtinylogger.la

clocks_SOURCES = clocks.c
clocks_LDADD = ../src/libtinylogger.la

check_timezone_SOURCES = check-timezone.c
check_timezone_LDADD = ../src/libtinylogger.la

json_timezones_SOURCES = json-timezones.c
json_timezones_LDADD = ../src/libtinylogger.la

perf_test_SOURCES = perf-test.c
perf_test_LDADD = $(COMMON_LIBS)

