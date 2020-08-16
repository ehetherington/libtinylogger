
## Message formats

In addition to the pre-configured formats, custom formats can be created. In the
spirit of this being a library, they are created _outside_ the library source.
See the formats.c example source file.

## Pre-configured output formats available

- [log_fmt_basic](#log_fmt_basic)
- [log_fmt_standard](#log_fmt_standard)
- [log_fmt_debug](#log_fmt_debug)
- [log_fmt_debug_tid](#log_fmt_debug_tid)
- [log_fmt_debug_tname](#log_fmt_debug_tname)
- [log_fmt_debug_tall](#log_fmt_debug_tall)
- [log_fmt_elapsed_time](#log_fmt_elapsed_time) Elapsed time from channel
		initialization. Starting time may be reset.
- [log_fmt_xml](#log_fmt_xml) Structured format.
- [log_fmt_json](#log_fmt_json) Structured format.


### log_fmt_basic <a name="log_fmt_basic"/>
Output messages with just the user message.

```
eth0     AF_PACKET (17)
```

### log_fmt_systemd <a name="log_fmt_systemd">
Output messages in systemd compatible format. Systemd log messages have their
log level inclosed in angle brackets prepended to the user message. When viewed
through journalctl, timestamps are added, so only the log level and user message
are needed.

```
<7>eth0     AF_PACKET (17)
```

### log_fmt_standard <a name="log_fmt_standard">
Output messages with timestamp, level and message.

```
2020-05-25 16:55:18.821 DEBUG   eth0     AF_PACKET (17)
```

### log_fmt_debug <a name="log_fmt_debug">
Output messages with timestamp, level, source code file, function, and line
number, and finally the message.

```
2020-05-25 17:28:17.011 DEBUG   test-logger.c:main:110 eth0     AF_PACKET (17)
```

### log_fmt_debug_tid <a name="log_fmt_debug_tid">
log_fmt_debug with thread id added
```
2020-05-25 17:28:17.011 DEBUG   65623 test-logger.c:main:110 eth0     AF_PACKET (17)
```

### log_fmt_debug_tname <a name="log_fmt_debug_tname">
log_fmt_debug with thread name added
```
2020-05-25 17:28:17.011 DEBUG   thread_2 test-logger.c:main:110 eth0     AF_PACKET (17)
```

### log_fmt_debug_tall <a name="log_fmt_debug_tall">
log_fmt_debug with thread id and thread name added
```
2020-05-25 17:28:17.011 DEBUG   65623:thread_2 test-logger.c:main:110 eth0     AF_PACKET (17)
```

### log_fmt_elapsed_time <a name="log_fmt_elapsed_time">
log_fmt_debug with elapsed time timestamp
```
0.000001665 INFO    formats.c:main:172 this message has elapsed time
0.000010344 INFO    formats.c:main:173 this message has elapsed time
0.000018740 INFO    formats.c:main:174 this message has elapsed time
```


### log_fmt_xml <a name="log_fmt_xml">
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
### log_fmt_json <a name="log_fmt_json">
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
