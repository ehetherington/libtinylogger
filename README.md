## libTinyLogger

### Small Yet Flexible Logger In C.

This is a small logging facility intended for small Linux projects. It logs
mesages from a single process to a file and/or a stream such as stderr.

It may be compiled directly with a target program, or installed as a library.

### Features:

- Output may be directed to a stream or a file.
- Messages are filtered by a log level.
- It produces output in a few different formats.
  - Pre-defined formats for systemd, standard and debug use.
  - User defined formatters are possible.
  - Structured output in XML and Json
- multi-thread support, with formats that print thread id and name
- logrotate support. Flushes, closes, and opens a log file on receipt of a
  signal from logrotate.
- tested on 64 and 32 bit Linux
  - RHEL 8.1 4.18.0-193.1.2.el8_2.x86_64 
  - Raspian Buster 4.19.97-v7l+

## Table of contents
1. [Getting Started](#getting_started)
2. [Examples](#examples)
   - [Hello world](#hello-world)
   - [Two streams](#two-streams)
3. [Output formats](#output-formats)
   - [basic](#basic)
   - [systemd](#systemd)
   - [standard](#standard)
   - [debug](#debug)
   - [XML](#xml)
   - [Json](#json)
4. [Example Output](#example-output)
   - [Two stream output](#two-stream-output)
   - [Custom formats](#custom-formats)
5. [Additional Information](guide/guide.md)

## Getting started <a name="getting_started"></a>
### Quick Start
There is a directory set up with a minimum build environment called
quick-start.
```
$ cd quick-start
$ ./setup.sh
$ make
```
This builds a static library called libtinylogger.a which can be linked to your
project. It also builds the examples, which demonstrate different features of
the library.

### Installation as a library is optional
This is an Autotools package. For installation into /usr/local/lib,
installation is very easy. To give it a test drive, 

```
$ autoreconf -i
$ ./configure
$ make
$ DESTDIR=/tmp/destdir make install
```

libtinylogger static and dynamic librarys will be installed in $DESTDIR/usr/local/lib
and the header file will be installed into $DESTDIR/usr/local/include.

If you have doxygen, man pages can be made and installed into /usr/local/man.

## Examples <a name="examples"></a>
### The obligatory minimal program would be: <a name="hello-world"></a>

Source code: [hello_world.c](demo/hello_world.c)

~~~{.c}
    #include "tinylogger.h"

    int main(void) {
        log_info("hello, %s\n", "world");
    }
~~~

To compile with library installed:

    gcc -o hello_world hello_world.c -pthread -lpthread -ltinylogger

Minimum Makefile without library installed:
```
TINY_LOGGER_HDRS = config.h tinylogger.h private.h
TINY_LOGGER_SRCS = tinylogger.c formatters.c xml_formatter.c logrotate.c

LDFLAGS = -lpthread

hello_world: hello_world.c $(TINY_LOGGER_HDRS) $(TINY_LOGGER_SRCS)
```

The output to stderr would be:

    2020-05-30 18:59:49.014 INFO    hello, world

### Output to two streams simultaneously <a name="two-streams"></a>

Source code: [second.c](demo/second.c)

-- The first stream is to stderr in systemd compatible format. It passes messages
of INFO and above.

-- The second stream logs to a file in debug format. It passes messages of TRACE
and above.

~~~{.c}
#include "tinylogger.h"

#define DEBUG_PATHNAME "/tmp/second.log"
int main(void) {
	/*
	 * Change main stderr format to systemd and add output to a file.
	 * Set a second file output stream to DEBUG_PATHNAME.
	 *  set log level to FINE
	 *  set format to debug
	 *  turn line buffering on (useful with tail -f, for example)
	 */
	LOG_CHANNEL *ch1 = log_open_channel_s(stderr, LL_INFO, log_fmt_systemd);
	LOG_CHANNEL *ch2 = log_open_channel_f(DEBUG_PATHNAME, LL_FINE, log_fmt_debug, true);
	log_notice("this message will be printed to both");
	log_info("this message will be printed to both");
	log_debug("this message will be printed to file only");
	log_finer("this message will not be printed at all");
	log_close_channel(ch1);
	log_close_channel(ch2);

	return 0;
}
~~~

The output to stderr for systemd would be:

~~~~~
    <5>this message will be printed to both
    <6>this message will be printed to both
~~~~~

The output to /tmp/second.log would be:

~~~
    2020-05-30 19:22:11.541 NOTICE  second.c:main:14 this message will be printed to both
    2020-05-30 19:22:11.541 INFO    second.c:main:15 this message will be printed to both
    2020-05-30 19:22:11.541 DEBUG   second.c:main:16 this message will be printed to file only
~~~

## Pre-configured output formats available <a name="output-formats"/>
Based on the output format selected by the user, a use of log_debug() will
produce a variety of output formats. Given the following line of code:

~~~{.c}
    log_debug("%s AF_PACKET(%d)", ifname, address_family);
~~~

### basic <a name="basic"/>
Output messages with just the user message.

-- Example:

~~~
    eth0     AF_PACKET (17)
~~~

### systemd <a name="systemd">
Output messages in systemd compatible format. Systemd log messages have their
log level inclosed in angle brackets prepended to the user message. When viewed
through journalctl, timestamps are added, so only the log level and user message
are needed.

-- Example:

~~~
    <7>eth0     AF_PACKET (17)
~~~

### standard <a name="standard">
Output messages with timestamp, level and message.

-- Example:

~~~
    2020-05-25 16:55:18.821 DEBUG   eth0     AF_PACKET (17)
~~~

### debug <a name="debug">
Output messages with timestamp, level, source code file, function, and line
number, and finally the message.

-- Example:

~~~
    2020-05-25 17:28:17.011 DEBUG   test-logger.c:main:110 eth0     AF_PACKET (17)
~~~

### XML <a name="xml">
Output messages in java.util.logging.XMLFormatter format.

-- Example:

```
<record>
  <date>2020-06-19T22:43:01.915644710-04:00:00</date>
  <millis>1592620981915</millis>
  <nanos>644710</nanos>
  <sequence>2</sequence>
  <logger>tinylogger</logger>
  <level>INFO</level>
  <class>xml.c</class>
  <method>main</method>
  <thread>1002084</thread>
  <message>This message uses the &quot;&lt;xml&gt;&quot; format. It&apos;s msg #2.</message>
</record>
```
### Json <a name="json">
Output message in json format.

-- Example:
```
{
  "records" : [  {
    "isoDateTime" : "2020-07-31T15:12:56.408789290-04:00",
    "timespec" : {
      "sec" : 1596222776,
      "nsec" : 408789290
    },
    "sequence" : 1,
    "logger" : "tinylogger",
    "level" : "INFO",
    "file" : "json-hello.c",
    "function" : "main",
    "line" : 32,
    "threadId" : 164933,
    "threadName" : "json-hello",
    "message" : "hello world (msg 0)"
  },  {
    "isoDateTime" : "2020-07-31T15:12:56.408893063-04:00",
    "timespec" : {
      "sec" : 1596222776,
      "nsec" : 408893063
    },
    "sequence" : 2,
    "logger" : "tinylogger",
    "level" : "INFO",
    "file" : "json-hello.c",
    "function" : "main",
    "line" : 32,
    "threadId" : 164933,
    "threadName" : "json-hello",
    "message" : "hello world (msg 1)"
  } ]
}
```

## Example Output <a name="example-output"/>
### Messages simultaneously logged by systemd/journalctl and to debug file <a name="two-stream-output"/>

Note different formatting and extra "debug" level message due to different log level
filtering.

-- Output in systemd compatible format to stderr, logged by systemd and viewed by journalctl

~~~
    May 29 08:39:31 raspberrypi wol-broadcaster[519]: pkt from obscured-for-safety dot com (11.111.111.111:37329) was not a magic packet
    May 29 08:39:31 raspberrypi wol-broadcaster[519]: packet contents:
    May 29 08:39:31 raspberrypi wol-broadcaster[519]:   0000  47 45 54 20 2f 20 48 54 54 50 2f 31 2e 31 0d 0a  GET / HTTP/1.1..
    May 29 08:39:31 raspberrypi wol-broadcaster[519]:   0010  48 6f 73 74 3a 20 77 77 77 0d 0a 0d 0a           Host: www....
~~~

-- The same messages logged to a debug file using standard output format

~~~
    2020-05-29 08:39:31.285 DEBUG   received pkt from obscured-for-safety dot com (11.111.111.111:37329)
    2020-05-29 08:39:31.285 INFO    pkt from obscured-for-safety dot com (11.111.111.111:37329) was not a magic packet
    2020-05-29 08:39:31.285 INFO    packet contents:
    2020-05-29 08:39:31.286 INFO      0000  47 45 54 20 2f 20 48 54 54 50 2f 31 2e 31 0d 0a  GET / HTTP/1.1..
    2020-05-29 08:39:31.286 INFO      0010  48 6f 73 74 3a 20 77 77 77 0d 0a 0d 0a           Host: www....
~~~

### Message with thread id and thread name
This is the Linux thread id, not the pthread_id.
```
2020-05-25 17:28:17.011 DEBUG   65623:thread_2 test-logger.c:main:110 eth0     AF_PACKET (17)
```
The linux thread id is more for use with utilities such as ps. It is the thread
id displayed, for instance, by:
```
$ ps H -C threads -o 'pid tid cmd comm'
```
Examples with multiple threads and message formats that display thread id are:
[threads.c](demo/threads.c) and [beehive.c](demo/beehive.c).


### Demo that displays all pre-configured outputs and a custom format <a name="custom-formats"/>
A custom format may be created. This [demo program](demo/formats.c) shows
how to create a custom format and demonstrates it and all the predefined
ones.

This is the output from that program:
```
this message uses the basic format
<6>this message uses the systemd format
2020-06-13 09:41:04 INFO    this message uses the standard format
2020-06-13 09:41:04.678 INFO    formats.c:main:114 this message uses the debug format
2020-06-13 09:41:04.678 INFO    327645 formats.c:main:117 this message uses the debug_tid format
2020-06-13 09:41:04.678 INFO    formats formats.c:main:120 this message uses the debug_tname format
2020-06-13 09:41:04.678 INFO    327645:formats formats.c:main:123 this message uses the debug_tall format
June 13, 2020, 09:41:04 INFO    327645:formats this message uses a CUSTOM format
junio 13, 2020, 09:41:04 INFO    formats.c:main:139 this message uses a another CUSTOM format
```
