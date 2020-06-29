AM_CPPFLAGS = -I$(top_srcdir)/src
AM_CFLAGS = -Wall -Wpedantic -Werror
AM_LDFLAGS = -static -lpthread

noinst_PROGRAMS = \
	hello_world \
	second formats \
	threads \
	demo \
	beehive \
	xml \
	json \
	xml-levels \
	parse_time

JAVAROOT = .
dist_noinst_JAVA = ParseXMLDate.java

hello_world_SOURCES = hello_world.c
hello_world_LDADD = ../src/libtinylogger.la

second_SOURCES = second.c
second_LDADD = ../src/libtinylogger.la

formats_SOURCES = formats.c
formats_LDADD = ../src/libtinylogger.la

demo_SOURCES = demo.c
demo_LDADD = ../src/libtinylogger.la

threads_SOURCES = threads.c
threads_LDADD = ../src/libtinylogger.la

beehive_SOURCES = beehive.c
beehive_LDADD = ../src/libtinylogger.la

xml_SOURCES = xml.c
xml_LDADD = ../src/libtinylogger.la

json_SOURCES = json.c
json_LDADD = ../src/libtinylogger.la

xml_levels_SOURCES = xml-levels.c
xml_levels_LDADD = ../src/libtinylogger.la

parse_time_SOURCES = parse_time.c
parse_time_LDADD = ../src/libtinylogger.la

