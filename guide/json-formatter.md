# JSON formatter

Text must be encoded in UTF-8. (ASCII is a proper subset of that, and so it is
perfectly fine).

Two nice test files can be found at:
https://www.w3.org/2001/06/utf-8-test/UTF-8-demo.html and
UTF-8_Sampler.html from: http://kermitproject.org/utf8.html

The program utils/file-to-json.c reads a file and generates a message for each
line of text. That output log can then be read by the companion JSON-LogReader
project to produce an identical copy of the orignial input.

NOTE: messages are limited to BUFSIZ *including* any escaping required for
JSON.

## Two different formats available
There are two different formats available - `log_fmt_json` and
`log_fmt_json_records`

### `log_fmt_json`
`log_fmt_json` produces a JSON log object. It contains an optional header
followed by an array of records objects.

It is most easily parsed as an entire entity.

Example output produced by `log_fmt_json`:

```
{
  "header" : {
    "startDate" : "2020-11-28T00:26:41.205878884-05:00[America/New_York]",
    "hostname" : "sambashare",
    "notes" : "This log was produced using log_fmt_json"
  }, "records" : [  {
    "isoDateTime" : "2020-11-28T00:26:41.206316449-05:00[America/New_York]",
    "timespec" : {
      "sec" : 1606541201,
      "nsec" : 206316449
    },
    "sequence" : 1,
    "logger" : "tinylogger",
    "level" : "INFO",
    "file" : "json.c",
    "function" : "main",
    "line" : 111,
    "threadId" : 161060,
    "threadName" : "json",
    "message" : "This message is not very informative."
  },  {
    "isoDateTime" : "2020-11-28T00:26:41.206332617-05:00[America/New_York]",
    "timespec" : {
      "sec" : 1606541201,
      "nsec" : 206332617
    },
    "sequence" : 2,
    "logger" : "tinylogger",
    "level" : "INFO",
    "file" : "json.c",
    "function" : "main",
    "line" : 112,
    "threadId" : 161060,
    "threadName" : "json",
    "message" : "This message is not very informative either."
  } ]
}
```

### `log_fmt_json_records`
`log_fmt_json_records` produces a series of JSON record objects.

Since each record is a complete JSON object, this format makes stream based use
easier. It is also easy to make a reader to monitor a growing log file as each
record is immediately available for processing as it is produced.

Example output produced by `log_fmt_json_records`:

```
{
    "isoDateTime" : "2020-11-28T21:48:43.761094579-05:00[America/New_York]",
    "timespec" : {
      "sec" : 1606618123,
      "nsec" : 761094579
    },
    "sequence" : 1,
    "logger" : "tinylogger",
    "level" : "INFO",
    "file" : "json.c",
    "function" : "main",
    "line" : 111,
    "threadId" : 188373,
    "threadName" : "json",
    "message" : "sixteenth_note (♬), g_clef (𝄞)"
}
{
    "isoDateTime" : "2020-11-28T21:48:43.761337425-05:00[America/New_York]",
    "timespec" : {
      "sec" : 1606618123,
      "nsec" : 761337425
    },
    "sequence" : 2,
    "logger" : "tinylogger",
    "level" : "INFO",
    "file" : "json.c",
    "function" : "main",
    "line" : 112,
    "threadId" : 188373,
    "threadName" : "json",
    "message" : "\" quotes and \\ backslashes are escaped"
}
```

## Fields in the Record objects

 - `isoDateTime` The message timestamp - it includes UTC offset, and may
                 optionally include Olson timezone. See [enabling timezone]
				 (#enabling-timezone).
 - `timespec`    The struct timespec timestamp (basis of isoDateTime)
   - `sec`
   - `nsec`
 - `sequence`    The sequence number of the message. Starts at 1.
 - `logger`      Always "tinylogger".
 - `file`        `__FILE__` captured by the calling macro
 - `function`    `__function__` captured by the calling macro
 - `line`        `__LINE__` captured by the calling macro
 - `threadId`    The linux thread id of the caller.
 - `threadName`  The linux thread name of the caller.
 - `message`     The user message.

`timespec` is the actual timestamp used to produce `isoDateTime`. A call to
`localtime_r(&(ts->tv_sec), &tm) == &tm)` is then made, and the UTC offset is
obtained from that.

`sequence` starts at one, and may be used for log consistancy checks.

`logger` is always set to "tinylogger".

`file`, `function` and `line` are obtained from the corresponding pre-processor
macros.

`threadId` is the linux thread id (not the Posix one).

`threadName` is the linux thread name.

`message` is the actual user message.


## There are two compile time options that may be disabled.
The header and Olson timezones are now _enabled_ by default. They may be disabled
at compile time.

- [Olson timezones](#enabling-timezone)
- [log header](#disabling-log-header)

## Enabling Olson Timezone in the timestamp <a name="enabling-timezone"/>
This feature appends the Olson timezone, as in "Europe/London" or
"America/New_York", not timezone IDs like "EST" or "BST".

Using the timezone allows access to time change history and rough geographic
region that the UTC offset doesn't provide.

Finding the timezone varies depending on Linux distro. And each particular
system may have its configuration modified. And the user can override the
system default by setting the TZ variable in the environment. For these reasons,
supporting timezone may be disabled at compile time.

If this feature is enabled, the timezone will be appended only if a matching
zoneinfo file in the zoneinfo database directory exists following the rules
used by tzset(3). If it fails to find one, the normal isoDateTime without a
timezone appended will be used. This is done to ensure the timezone used
actually matches the timestamp._

### Verify that this software finds the correct timezone
The program "check-timezone.c" in the demo directory has been provided to
check to see if the timezone can be found by this software. Run it:

```
$ demo/check-timezone
timezone found is America/New_York.
2020-10-16T18:43:47.454282598-04:00
2020-10-16T18:43:47.454282598-04:00[America/New_York]
Fri Oct 16 18:43:47 2020
```
If successfull, the first line displays the timezone found. The second line
displays the normal isoDateTime field (UTC offset, but no timezone). The third
line displays the timestamp with the timezone appended. The fourth line is
formatted by asctime(3).

If it reports a default timezone that meets your expectations, then use of it
can be enabled in the library. Make sure it works on all configurations of your
targets.

### Quick-start configuration
Edit the quick-start/config.h file to enable/disable timezones. There are
instructions in that file.

### autoconf configuration
There is an option to the configure script to enable/disable use of timezones
in JSON output. In the top directory:

```
$ autoreconf -i
$ ./configure --disable-timezone
$ make
```
### JSON isoDateTime field with and without Olson timezone

With:
```
    "isoDateTime" : "2020-08-11T17:40:31.109019932-04:00[America/New_York],
```
Without:
```
    "isoDateTime" : "2020-08-11T17:40:31.109019932-04:00",
```

An example of a complete log file with Olson timezones enabled:
```
{
  "records" : [  {
    "isoDateTime" : "2020-10-23T13:00:35.509017997-04:00[America/New_York]",
    "timespec" : {
      "sec" : 1603472435,
      "nsec" : 509017997
    },
    "sequence" : 1,
    "logger" : "tinylogger",
    "level" : "INFO",
    "file" : "json.c",
    "function" : "main",
    "line" : 35,
    "threadId" : 13581,
    "threadName" : "json",
    "message" : "\b backspaces are escaped for JSON output"
  },  {
    "isoDateTime" : "2020-10-23T13:00:35.509343545-04:00[America/New_York]",
    "timespec" : {
      "sec" : 1603472435,
      "nsec" : 509343545
    },
    "sequence" : 2,
    "logger" : "tinylogger",
    "level" : "INFO",
    "file" : "json.c",
    "function" : "main",
    "line" : 36,
    "threadId" : 13581,
    "threadName" : "json",
    "message" : "\r carriage returns are escaped for JSON output"
  } ]
}
```
## Disabling header object <a name="disabling-log-header"/>

The header for the log file can be disabled at compile time.

The header is a `logHeader` object that consists of:

 - `logHeader`
     - `startDate` A timestamp of when the channel was configured
     - `hostname`  The hostname obtained from /proc/sys/kernel/hostname
     - `notes`     Notes added by `log_set_json_notes()` BEFORE the channel is
                   opened

A sample log with both the header and Olson timezone options enabled:

```

{
  "logHeader" : {
    "startDate" : "2020-11-06T23:45:18.321777407-05:00[America/New_York]",
    "hostname" : "sambashare",
    "notes" : "this is log 3 - it's sequence also starts at 1"
  }, "records" : [  {
    "isoDateTime" : "2020-11-06T23:45:18.321799100-05:00[America/New_York]",
    "timespec" : {
      "sec" : 1604724318,
      "nsec" : 321799100
    },
    "sequence" : 1,
    "logger" : "tinylogger",
    "level" : "INFO",
    "file" : "stream-of-logs.c",
    "function" : "main",
    "line" : 30,
    "threadId" : 50759,
    "threadName" : "stream-of-logs",
    "message" : "five"
  },  {
    "isoDateTime" : "2020-11-06T23:45:18.321870267-05:00[America/New_York]",
    "timespec" : {
      "sec" : 1604724318,
      "nsec" : 321870267
    },
    "sequence" : 2,
    "logger" : "tinylogger",
    "level" : "INFO",
    "file" : "stream-of-logs.c",
    "function" : "main",
    "line" : 31,
    "threadId" : 50759,
    "threadName" : "stream-of-logs",
    "message" : "six"
  } ]
}

```

To disable header:
```
$ autoreconf -i
$ ./configure --disable-json-header
$ make
```
To disable both options:
```
$ autoreconf -i
$ ./configure --disable-timezone --disable-json-header
$ make
```

For the quick-start configuration, edit the quick-start/config.h file.


json.c, logrotate.c, and stream-of-logs.c are example programs in the demo directory,

