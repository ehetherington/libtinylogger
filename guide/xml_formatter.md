## XML output formatter

Rather than re-invent the wheel, the java.util.logging.XMLFormatter format was
used. It has an official DTD. See Appendix A of Oracle's
[java-logging-overview] (https://docs.oracle.com/javase/10/core/java-logging-overview.htm).

Or, [here](./logger.dtd)


The millis element is the number of milliseconds since the epoch.
The nanos element is number of nanoseconds to add to that.

So, to create them from a timespec:

```{.c}
	struct timespec *ts;
    long int time_millis;
    long int time_nanos;

    time_millis = ts->tv_sec * 1000 + ts->tv_nsec / 1000000;
    time_nanos = ts->tv_nsec % 1000000;
```

To create the values for the millis and nanos element

### Tinylogger to XMLFormatter xml element mapping

The date element is in the same ISO 8601 format, except the millisecond fraction is added
```
  <date>2020-06-14T10:48:35.634</date>
```

The millis element is the same.
```
  <millis>1592146115634</millis>
```

The nanos element (added in JDK 9) is the same.
```
  <nanos>587985</nanos>
```

The sequence element is the same.
```
  <sequence>0</sequence>
```

The logger element, which is normally the name of the logger class, is filled with tinylogger.
```
  <logger>tinylogger</logger>
```

The level element is filled with the standard Java labels when a log level from
Java is used. When there is no corresponding Java pre-defined level, an integer
representing its interpolated level is used.
```
  <level>INFO</level>
```

The class element is filled with the source code file name instead of the class;
```
  <class>formats.c</class>
```

The method element is filled with the function name.
```
  <method>main</method>
```

The thread element is filled with the thread id returned by gettid(). This is
different than the pthread_t thread id.
```
  <thread>407953</thread>
```

And finally, the message element is filled with the actual message.
```
  <message>this message uses the &quot;&lg;xml&gt;&quot; format &amp; it&apos;s easy!</message>
```

### Special Character "escaping"
The following characters are replace with the corresponding xml entities:

character |  entity  | description
----------|----------|------------
'<'       | "&lt;"   | less than
'>'       | "&gt;"   | greater than
'&'       | "&amp;"  | ampersand
'\''      | "&apos;" | apostrophe
'\"'      | "&quot;" | quote
(using c style notation for "char" and "string" - probably not appropriate)

### example

The following program:

```{.c}
#include "tinylogger.h"

#define LOG_FILE "tinylogger.xml"
#define N_MSGS 3

int main(void) {
	/*
	 * Print a series of xml formatted messages
	 * Use the xml formatter
	 * Use line buffered output
	 */
	LOG_CHANNEL *ch1 = log_open_channel_f(LOG_FILE, LL_INFO, log_fmt_xml, true);
	(void) ch1;	// quiet the "unused variable" warning

	for (int n = 0; n < N_MSGS; n++) {
		log_info("this message uses the \"<xml>\" format it's #%d", n);
	}

	log_done();

	return 0;
}
```

produces:
```
<?xml version="1.0" encoding="UTF-8" standalone="no"?>
<!DOCTYPE log SYSTEM "logger.dtd">
<log>
<record>
  <date>2020-06-15T12:59:55.288</date>
  <millis>1592240395288</millis>
  <nanos>895371</nanos>
  <sequence>0</sequence>
  <logger>tinylogger</logger>
  <level>INFO</level>
  <class>xml.c</class>
  <method>main</method>
  <thread>488739</thread>
  <message>this message uses the &quot;&lt;xml&gt;&quot; format it&apos;s #0</message>
</record>
<record>
  <date>2020-06-15T12:59:55.289</date>
  <millis>1592240395289</millis>
  <nanos>103486</nanos>
  <sequence>1</sequence>
  <logger>tinylogger</logger>
  <level>INFO</level>
  <class>xml.c</class>
  <method>main</method>
  <thread>488739</thread>
  <message>this message uses the &quot;&lt;xml&gt;&quot; format it&apos;s #1</message>
</record>
<record>
  <date>2020-06-15T12:59:55.289</date>
  <millis>1592240395289</millis>
  <nanos>143152</nanos>
  <sequence>2</sequence>
  <logger>tinylogger</logger>
  <level>INFO</level>
  <class>xml.c</class>
  <method>main</method>
  <thread>488739</thread>
  <message>this message uses the &quot;&lt;xml&gt;&quot; format it&apos;s #2</message>
</record>
</log>
```

When read by a java program that reads files produced with the
java.util.logging.XMLForatter formatter:

```
Jun 15, 2020 12:59:55 PM xml.c main
INFO: this message uses the "<xml>" format it's #0
Jun 15, 2020 12:59:55 PM xml.c main
INFO: this message uses the "<xml>" format it's #1
Jun 15, 2020 12:59:55 PM xml.c main
INFO: this message uses the "<xml>" format it's #2
```
