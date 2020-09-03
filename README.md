## libTinyLogger

### Small Yet Flexible Logger In C.

This is a small logging library for small Linux projects. It logs messages from
a single process to a file and/or a stream such as stderr. It has many message
formats, including JSON and XML.

Thread safe. There are message formats with thread id and/or thread name
provided.

Custom message formats may be created without modifying the library. A timestamp
formatter with second, millisecond, microsecond and nanosecond resolution is
provided.

It may be compiled directly with a target program, or installed as a shared
library.

### Features:

- Output may be directed to a stream or a file.
- Messages are filtered by a log level.
- It produces output in a few different formats.
  - Pre-defined formats for systemd, standard and debug use.
  - Elapsed time can be used in place of date/time
  - Structured output in XML and JSON
  - User defined formatters are possible.
- thread safe, with formats that print thread id and name
- logrotate support. Flushes, closes, and re-opens a log file on receipt of a
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
   - [basic](#log_fmt_basic)
   - [systemd](#log_fmt_systemd)
   - [standard](#log_fmt_standard)
   - [debug](#log_fmt_debug)
   - [debug_tid](#log_fmt_debug_tid)
   - [debug_tname](#log_fmt_debug_tname)
   - [debug_tall](#log_fmt_debug_tall)
   - [elapsed_time](#log_fmt_elapsed_time) (Elapsed time from channel
        initialization. Starting time may be reset).
   - [XML](#log_fmt_xml) (Structured format).
   - [JSON](#log_fmt_json) (Structured format).
4. [Memory dumps](#memory-dumps) Memory dumps may be used with any format.
5. [Other samples](#other-samples)
   - [Two stream output](#two-stream-output)
   - [Thread ID and name](#thread-id-name)
6. Additional Information
   - A [guide](guide/guide.md) (markdown in the guide subdirectory)

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
the library. Additional info in [quick-start](guide/quick-start.md).

### Installation as a library is optional
This is an Autotools package. For installation into /usr/local/lib,
installation is very easy. To see what get installed and where, install it in
"alternate root" by specifying DESTDIR when installing:
```
$ autoreconf -i
$ ./configure
$ make
$ DESTDIR=/tmp/destdir make install
```
libtinylogger static and dynamic libraries will be installed in $DESTDIR/usr/local/lib
and the header file will be installed into $DESTDIR/usr/local/include.

To actually install,
```
$ sudo make install
```
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

### basic <a name="log_fmt_basic"/>
Output messages with just the user message.

```
eth0     AF_PACKET (17)
```

### systemd <a name="log_fmt_systemd">
Output messages in systemd compatible format. Systemd log messages have their
log level enclosed in angle brackets prepended to the user message. When viewed
through journalctl, timestamps are added, so only the log level and user message
are needed.

```
<7>eth0     AF_PACKET (17)
```

### standard <a name="log_fmt_standard">
Output messages with timestamp, level and message.

```
2020-05-25 16:55:18.821 DEBUG   eth0     AF_PACKET (17)
```

### debug <a name="log_fmt_debug">
Output messages with timestamp, level, source code file, function, and line
number, and finally the message.

```
2020-05-25 17:28:17.011 INFO   test-logger.c:main:110 eth0     AF_PACKET (17)
```

### debug_tid <a name="log_fmt_debug_tid">
log_fmt_debug with thread id added
```
2020-05-25 17:28:17.011 INFO   65623 test-logger.c:main:110 eth0     AF_PACKET (17)
```

### debug_tname <a name="log_fmt_debug_tname">
log_fmt_debug with thread name added
```
2020-05-25 17:28:17.011 INFO   thread_2 test-logger.c:main:110 eth0     AF_PACKET (17)
```

### debug_tall <a name="log_fmt_debug_tall">
log_fmt_debug with thread id and thread name added
```
2020-05-25 17:28:17.011 INFO   65623:thread_2 test-logger.c:main:110 eth0     AF_PACKET (17)
```

### elapsed_time <a name="log_fmt_elapsed_time">
log_fmt_debug with elapsed time timestamp
```
0.000001665 INFO    formats.c:main:172 this message has elapsed time
0.000010344 INFO    formats.c:main:173 this message has elapsed time
0.000018740 INFO    formats.c:main:174 this message has elapsed time
```


### XML <a name="log_fmt_xml">
Output messages in java.util.logging.XMLFormatter format.

```
<?xml version="1.0" encoding="UTF-8" standalone="no"?>
<!DOCTYPE log SYSTEM "logger.dtd">
<log>
<record>
  <date>2020-08-15T23:10:49.081-04:00</date>
  <millis>1597547449081</millis>
  <nanos>969853</nanos>
  <sequence>1</sequence>
  <logger>tinylogger</logger>
  <level>INFO</level>
  <class>xml.c</class>
  <method>main</method>
  <thread>389042</thread>
  <message>Special chars are escaped in the &quot;&lt;xml&gt;&quot; format. &amp; it&apos;s msg #0.</message>
</record>
<record>
  <date>2020-08-15T23:10:49.082-04:00</date>
  <millis>1597547449082</millis>
  <nanos>112552</nanos>
  <sequence>2</sequence>
  <logger>tinylogger</logger>
  <level>INFO</level>
  <class>xml.c</class>
  <method>main</method>
  <thread>389042</thread>
  <message>Special chars are escaped in the &quot;&lt;xml&gt;&quot; format. &amp; it&apos;s msg #1.</message>
</record>
</log>
```
### JSON <a name="log_fmt_json">
There is a companion [reader](https://github.com/ehetherington/JSON-LogReader)
for files produced in the JSON format.

Output message in json format.

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

## Memory dumps <a name="memory-dumps"/>
Regions of memory can be formatted to hex + ascii and appended to the user
message in all the above formats.

Instead of using log_info(...) and friends, log_memory(...) is available.

With a channel set up with the debug format,
```
log_memory(LL_INFO, buf, sizeof(buf), "hello, %s", "world");
```
produces:
```
2020-08-17 21:08:38.418 INFO    log_mem.c:main:55 hello, world
  0000  00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f  ................
  0016  10 11 12 13 14 15 16 17 18 19 1a 1b 1c 1d 1e 1f  ................
  0032  20 21 22 23 24 25 26 27 28 29 2a 2b 2c 2d 2e 2f   !"#$%&'()*+,-./
  0048  30 31 32 33 34 35 36 37 38 39 3a 3b 3c 3d 3e 3f  0123456789:;<=>?
  0064  40 41 42 43 44 45 46 47 48 49 4a 4b 4c 4d 4e 4f  @ABCDEFGHIJKLMNO
  0080  50 51 52 53 54 55 56 57 58 59 5a 5b 5c 5d 5e 5f  PQRSTUVWXYZ[\]^_
  0096  60 61 62 63 64 65 66 67 68 69 6a 6b 6c 6d 6e 6f  `abcdefghijklmno
  0112  70 71 72 73 74 75 76 77 78 79 7a 7b 7c 7d 7e 7f  pqrstuvwxyz{|}~.
  0128  80 81 82 83 84 85 86 87 88 89 8a 8b 8c 8d 8e 8f  ................
  0144  90 91 92 93 94 95 96 97 98 99 9a 9b 9c 9d 9e 9f  ................
  0160  a0 a1 a2 a3 a4 a5 a6 a7 a8 a9 aa ab ac ad ae af  ................
  0176  b0 b1 b2 b3 b4 b5 b6 b7 b8 b9 ba bb bc bd be bf  ................
  0192  c0 c1 c2 c3 c4 c5 c6 c7 c8 c9 ca cb cc cd ce cf  ................
  0208  d0 d1 d2 d3 d4 d5 d6 d7 d8 d9 da db dc dd de df  ................
  0224  e0 e1 e2 e3 e4 e5 e6 e7 e8 e9 ea eb ec ed ee ef  ................
  0240  f0 f1 f2 f3 f4 f5 f6 f7 f8 f9 fa fb fc fd fe ff  ................
  0256  00 01 02 03 04 05 06 07                          .
```
The memory dump is appended to the normal user message, so the whole thing is
enclosed in a single message in XML or JSON formats.

With a channel set up with the JSON format, logging a 24 byte slice (for
brevity) of that memory region,
```
log_memory(LL_INFO, buf + 0x20, 24, "hello, %s", "world");
```
produces:
```
{
  "records" : [  {
    "isoDateTime" : "2020-08-17T21:08:38.418348026-04:00",
    "timespec" : {
      "sec" : 1597712918,
      "nsec" : 418348026
    },
    "sequence" : 1,
    "logger" : "tinylogger",
    "level" : "INFO",
    "file" : "log_mem.c",
    "function" : "main",
    "line" : 61,
    "threadId" : 502942,
    "threadName" : "log_mem",
    "message" : "hello, world\n  0000  20 21 22 23 24 25 26 27 28 29 2a 2b 2c 2d 2e 2f   !\"#$%&'()*+,-./\n  0016  30 31 32 33 34 35 36 37                          01234567        "
  } ]
}
```

The hex dump samples were produced by a demo program called log_mem.c

## Other Samples <a name="other-samples"/>
### Messages simultaneously logged by systemd/journalctl and to debug file <a name="two-stream-output"/>

Note different formatting and extra "debug" level message due to different log level
filtering.

-- Output in systemd compatible format to stderr, logged by systemd and viewed by journalctl

```
    May 29 08:39:31 raspberrypi wol-broadcaster[519]: pkt from obscured-for-safety dot com (11.111.111.111:37329) was not a magic packet
    May 29 08:39:31 raspberrypi wol-broadcaster[519]: packet contents:
    May 29 08:39:31 raspberrypi wol-broadcaster[519]:   0000  47 45 54 20 2f 20 48 54 54 50 2f 31 2e 31 0d 0a  GET / HTTP/1.1..
    May 29 08:39:31 raspberrypi wol-broadcaster[519]:   0010  48 6f 73 74 3a 20 77 77 77 0d 0a 0d 0a           Host: www....
```

-- The same messages logged to a debug file using standard output format. The
DEBUG message appears because this stream uses a different log level.

```
    2020-05-29 08:39:31.285 DEBUG   received pkt from obscured-for-safety dot com (11.111.111.111:37329)
    2020-05-29 08:39:31.285 INFO    pkt from obscured-for-safety dot com (11.111.111.111:37329) was not a magic packet
    2020-05-29 08:39:31.285 INFO    packet contents:
    2020-05-29 08:39:31.286 INFO      0000  47 45 54 20 2f 20 48 54 54 50 2f 31 2e 31 0d 0a  GET / HTTP/1.1..
    2020-05-29 08:39:31.286 INFO      0010  48 6f 73 74 3a 20 77 77 77 0d 0a 0d 0a           Host: www....
```

### Message with thread id and thread name <a name="thread-id-name"/>
This is the Linux thread id, not the pthread_id.
```
2020-05-25 17:28:17.011 DEBUG   65623:thread_2 test-logger.c:main:110 eth0     AF_PACKET (17)
```
The linux thread id is used rather than the posix thread id. It can be accessed, for instance, by:
```
$ ps H -C example-program -o 'pid tid cmd comm'
```
Examples with multiple threads and message formats that display thread id are:
[threads.c](demo/threads.c) and [beehive.c](demo/beehive.c).


