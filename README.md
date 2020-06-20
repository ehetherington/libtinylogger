## libTinyLogger

## Description
### Small Yet Flexible Logger In C.

This is a small logging facility intended for small Linux projects. It logs
mesages from a single process to a file and/or a stream such as stderr.

It may be compiled directly with a target program, or installed as a library.

### Features:

- Output may be directed to a stream or a file.
- Messages are filtered by a log level.
- It produces output in a few different styles.
  - User defined formatters are possible.
- multi-thread support, with formats that print thread id and name
- logrotate support. Flushes, closes, and opens a log file on receipt of a
  signal from logrotate.
- tested on 64 and 32 bit Linux
  - RHEL 8.1 4.18.0-193.1.2.el8_2.x86_64 
  - Raspian Buster 4.19.97-v7l+

## Credits:
[Nominal Animal](https://stackoverflow.com/users/1475978/nominal-animal) made an excellent 
[post](https://stackoverflow.com/questions/53188731/logging-compatibly-with-logrotate#53201067)
on [StackOverflow](https://stackoverflow.com) illustrating logrotate support.

## Table of contents
1. [Dependancies](#dependancies)
2. [Installation](#installation)
3. [Usage](#usage)
   - [Hello world](#hello-world)
   - [Two streams](#two-streams)
4. [Output styles](#output-styles)
   - [basic](#basic)
   - [systemd](#systemd)
   - [standard](#standard)
   - [debug](#debug)
   - [XML](#xml)
5. [Example Output](#example-output)
   - [Two stream output](#two-stream-output)
   - [Custom formats](#custom-formats)
6. [Additional Information](guide/guide.md)

## Dependancies <a name="dependancies"></a>
- sd-daemon.h
  - RHEL8
```
$ sudo dnf install systemd-devel
```
  - Raspbian
```
$ sudo apt install libsystemd-dev
```
- Building the docs is optional. If you want to build the docs, There is a
	[guide](guide/doxygen.md) for installing doxygen on RHEL8 and Raspbian.
- I didn't keep a list of things I needed to install. Configure.ac needs work.
	docs/Makefile.am needs a lot of work.

## Installation <a name="installation"></a>
### Installation as a library is optional
This is an Autotools package. For installation into /usr/local/lib,
installation is very easy.

	$ autoreconf -i
    $ ./configure
	$ make
	$ sudo make install

libtinylogger static and dynamic librarys will be installed in /usr/local/lib
and the header file will be installed into /usr/local/include.

If you have doxygen, man pages can be made and installed into /usr/local/man.

## Usage <a name="usage"></a>
The following usage examples assume installation as a library.

The source files 
[tinylogger.c](src/tinylogger.c)
[tinylogger.h](src/tinylogger.h)
[formatters.c](src/formatters.c)
[logrotate.c](src/logrotate.c)
[private.h](src/private.h)
may alternatively be compiled and linked directly to your program.

tinylogger.c, tinylogger.h, formatters.c, logrotate.c and
private.h have been documented with doxygen flavored comments.

### The obligatory minimal program would be: <a name="hello-world"></a>

Source code: [hello_world.c](demo/hello_world.c)

~~~{.c}
    #include "tinylogger.h"

    int main(void) {
        log_info("hello, %s\n", "world");
    }
~~~

To compile:

    gcc -o hello_world hello_world.c -pthread -lpthread -ltinylogger

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
	LOG_CHANNEL ch1 = log_open_channel_s(stderr, LL_INFO, log_fmt_systemd);
	LOG_CHANNEL ch2 = log_open_channel_f(DEBUG_PATHNAME, LL_FINE, log_fmt_debug, true);
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

### More complete demo
A demo program [demo.c](demo/demo.c) is provided to demonstrate
the features available. To compile and run it:

	$ gcc -o test-logger test-logger.c -pthread -lpthread -ltinylogger
	$ ./test-logger

## Pre-configured output styles available <a name="output-styles"/>
Based on the output format selected by the user, a use of log_debug() will
produce a variety of output styles. Given the following line of code:

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
  <date>2020-06-14T15:19:50.437</date>
  <millis>1592162390437</millis>
  <nanos>522456</nanos>
  <sequence>0</sequence>
  <logger>tinylogger</logger>
  <level>INFO</level>
  <class>formats.c</class>
  <method>main</method>
  <thread>419645</thread>
  <message>this message uses the &quot;&lg;xml&gt;&quot; format &amp; it&apos;s easy!</message>
</record>


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
Examples that use threads and message formats that display the thread id are
[threads.c](demo/threads.c) and [beehive.c](demo/beehive.c).


### Demo that displays all pre-configured outputs and a custom format <a name="custom-formats"/>
A custom format may be created. This [demo program](demo/formats.c) shows
how to create a custom format and demonstrates it and all the predefined
ones.
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
This is the output from that program:
```
