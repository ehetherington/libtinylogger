## Examples

### hello_world.c
A "one-liner" demonstrating that the library can be used with no setup or
initialization.

### formats.c
A sampler of the available formats.

Along with the expected single line formats, the Json and XML structured
formats are shown.

It also demonstrates the elapsed time format. The elapsed time may be reset.

The creation of custom formats is demonstrated.

### beehive.c
A massively multi-threaded example, intended as a stress test for threaded
support.

The message format defaults to one that displays the thread id and thread name.
Source file, function and line are also displayed.

With the default format, a command line option to verify that each thread
generated the correct number of messages in the correct order may be selected.

Json and XML formats may also be selected on the command line.


### clocks.c
Just selects each available clock to show the effect on the message timestamps.

CLOCK_REALTIME is the default clock and most appropriate one for normal
timestamping.

CLOCK_MONOTONIC and CLOCK_MONOTONIC_RAW are most useful if you are using an
elapsed time format, as they are guaranteed not to go backwards.

### json.c
Writes a json formatted log. Demonstrates the escaping of the special
characters.
A log_memory() call is added to show that the hex format message is in a single
json "message", not spread across multiple ones that would need to be
reassembled.

### levels.c
Demonstrate use of the log_get_level() utility function, and setting the active
logging level.

log_get_level() is useful if you want to specify the logging level as a command
line argument to your program. The user can specify a string, such as "debug",
and the appropriate LOG_LEVEL is returned.

Then the active log level is set, and filtering of unwanted levels is
demonstrated.

### log_mem.c
Fills a buffer with 256 + 8 bytes and then prints the "hex dump" with ascii
with a single log_memory() call.

### logrotate.c
The library has support for logrotate behaviour.

A background thread may be started that listens for an external signal to be
sent from logrotate to perform a flush/close and re-open of the log file.

This is simualted by the program itself sending the signal that logrotate would
normally send.

Or, the program could perform the operation by itself (without starting that
thread) by an appropriate sequence of operations.

The mode of operation may be selected from the command line.

It defaults to a debug message format, but Json or XML may be selected from the
command line.

TODO: doxygen links logrotate.c to the library source file. Figure out how to
stop that.

### second.c
_Two Output Streams_

Messages may be logged to two destinations at the same time. Sometimes it is
useful to have the intended logging to be performed as it would normally be,
but also have a debugging stream with more information at the same time.

Each stream may be set up with different logging levels and message formats.

This example sets up the "intended" stream to stderr with the systemd format at
LL_INFO, while the second stream to a file with a debug format at LL_FINE.

### threads.c
A simpler example demonstrating formats with thread info.

It is also a handy reminder for getting the thread id's using the ps command.

### xml.c
A short example using the xml message format.
The format is specified by Appendix A: DTD for XMLFormatter Output in
https://docs.oracle.com/javase/10/core/java-logging-overview.htm

XML special characters are replace by the appropriate XML entities.

### xml-levels.c
The levels used in the XML format are text strings for normal
java.util.logging.Level levels. Systemd levels are printed with numeric
values interpolated or extrapolated from the java ones.

See guide/levels.md for more information about how the levels are displayed in
different formats.

