## JSON formatter
### JSON Message formatting and output
The format mostly mimics the java.util.logging.XMLFormatter output.

The JSON output is an array of records. The record objects are:

 - `isoDateTime` The message timestamp - includes UTC offset
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


Example output:
```
{
  "records" : [  {
    "isoDateTime" : "2020-08-11T17:40:31.109019932-04:00",
    "timespec" : {
      "sec" : 1597182031,
      "nsec" : 109019932
    },
    "sequence" : 1,
    "logger" : "tinylogger",
    "level" : "INFO",
    "file" : "json.c",
    "function" : "main",
    "line" : 35,
    "threadId" : 246970,
    "threadName" : "json",
    "message" : "\b backspaces are escaped for Json output"
  },  {
    "isoDateTime" : "2020-08-11T17:40:31.109213215-04:00",
    "timespec" : {
      "sec" : 1597182031,
      "nsec" : 109213215
    },
    "sequence" : 2,
    "logger" : "tinylogger",
    "level" : "INFO",
    "file" : "json.c",
    "function" : "main",
    "line" : 36,
    "threadId" : 246970,
    "threadName" : "json",
    "message" : "\r carriage returns are escaped for Json output"
  } ]
}
```
