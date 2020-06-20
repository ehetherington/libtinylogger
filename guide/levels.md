## Logging levels

The levels available include all the systemd levels, which are top heavy,
merged with java.util.logging ones, which fill out the bottom end.

Logging is normally by log_XXX() functions, which are actually macros that
expand to log_msg() function calls.


macro | LL_XXX value | java level | java value | systemd macro | systemd output | text output
------|--------------|------------|------------|---------------|----------------|-------------
log_emerg()   |  0 |         | 1300 | SD_EMERG   | <0> | EMERG 
log_alert()   |  1 |         | 1200 | SD_ALERT   | <1> | ALERT 
log_crit()    |  2 |         | 1100 | SD_CRIT    | <2> | CRIT 
log_severe()  |  3 | SEVERE  | 1000 |            | <3> | ERR 
log_err()     |  4 |         |  950 | SD_ERR     | <3> | ERR 
log_warning() |  5 | WARNING |  900 | SD_WARNING | <4> | WARNING 
log_notice()  |  6 |         |  850 | SD_NOTICE  | <5> | NOTICE 
log_info()    |  7 | INFO    |  800 | SD_INFO    | <6> | INFO 
log_config()  |  8 | CONFIG  |  700 |            | <6> | INFO 
log_debug()   |  9 |         |  600 | SD_DEBUG   | <7> | DEBUG 
log_fine()    | 10 | FINE    |  500 |            | <7> | FINE
log_finer()   | 11 | FINER   |  400 |            | <7> | FINER
log_finest()  | 12 | FINEST  |  300 |            | <7> | FINEST

### XML levels

XML formatting conforms to the java.util.logging.XMLFormatter formatting.
As such, it only prints standard java.util.logging.Level labels. If the
level is not one of the java ones, the numeric value is used.

This program runs through each level (demo/xml-levels.c):
```{.c}
#include "tinylogger.h"

#define LOG_FILE "xml-levels.xml"

int main(void) {
	/*
	 * Print a series of xml formatted messages
	 * Use the xml formatter
	 * Use line buffered output
	 */
	LOG_CHANNEL ch1 = log_open_channel_f(LOG_FILE, LL_ALL, log_fmt_xml, true);
	(void) ch1;	// quiet the "unused variable" warning

	log_emerg("emerg level");
	log_alert("alert level");
	log_crit("crit level");
	log_severe("severe level");
	log_err("err level");
	log_warning("warning level");
	log_notice("notice level");
	log_info("info level");
	log_config("config level");
	log_debug("debug level");
	log_fine("fine level");
	log_finer("finer level");
	log_finest("finest level");

	log_done();

	return 0;
}
```
LL_SEVERE, LL_WARNING, LL_INFO, LL_CONFIG, LL_FINE, LL_FINER and LL_FINEST
will get a label, while
LL_EMERG, LL_ALERT, LL_CRIT, LL_ERR, LL_NOTICE an LL_DEBUG will get numeric
values.

Work is required in the log reader to handle a non-standard label, and the
same work can give a translation from numeric value to a text label. So
this seems like the best solution.

And any other implementation in any other language can handle the translation.
It isn't like XML is the ideal human readable format. It is basically a
pre-parsed format for use by an analysis tool.

The output from that program:

```
<?xml version="1.0" encoding="UTF-8" standalone="no"?>
<!DOCTYPE log SYSTEM "logger.dtd">
<log>
<record>
  <date>2020-06-15T17:01:15.955</date>
  <millis>1592254875955</millis>
  <nanos>567600</nanos>
  <sequence>0</sequence>
  <logger>tinylogger</logger>
  <level>1300</level>
  <class>xml-levels.c</class>
  <method>main</method>
  <thread>515081</thread>
  <message>emerg level</message>
</record>
<record>
  <date>2020-06-15T17:01:15.955</date>
  <millis>1592254875955</millis>
  <nanos>827357</nanos>
  <sequence>1</sequence>
  <logger>tinylogger</logger>
  <level>1200</level>
  <class>xml-levels.c</class>
  <method>main</method>
  <thread>515081</thread>
  <message>alert level</message>
</record>
<record>
  <date>2020-06-15T17:01:15.955</date>
  <millis>1592254875955</millis>
  <nanos>877076</nanos>
  <sequence>2</sequence>
  <logger>tinylogger</logger>
  <level>1100</level>
  <class>xml-levels.c</class>
  <method>main</method>
  <thread>515081</thread>
  <message>crit level</message>
</record>
<record>
  <date>2020-06-15T17:01:15.955</date>
  <millis>1592254875955</millis>
  <nanos>923919</nanos>
  <sequence>3</sequence>
  <logger>tinylogger</logger>
  <level>SEVERE</level>
  <class>xml-levels.c</class>
  <method>main</method>
  <thread>515081</thread>
  <message>severe level</message>
</record>
<record>
  <date>2020-06-15T17:01:15.955</date>
  <millis>1592254875955</millis>
  <nanos>961647</nanos>
  <sequence>4</sequence>
  <logger>tinylogger</logger>
  <level>950</level>
  <class>xml-levels.c</class>
  <method>main</method>
  <thread>515081</thread>
  <message>err level</message>
</record>
<record>
  <date>2020-06-15T17:01:15.955</date>
  <millis>1592254875955</millis>
  <nanos>999516</nanos>
  <sequence>5</sequence>
  <logger>tinylogger</logger>
  <level>WARNING</level>
  <class>xml-levels.c</class>
  <method>main</method>
  <thread>515081</thread>
  <message>warning level</message>
</record>
<record>
  <date>2020-06-15T17:01:15.956</date>
  <millis>1592254875956</millis>
  <nanos>37653</nanos>
  <sequence>6</sequence>
  <logger>tinylogger</logger>
  <level>850</level>
  <class>xml-levels.c</class>
  <method>main</method>
  <thread>515081</thread>
  <message>notice level</message>
</record>
<record>
  <date>2020-06-15T17:01:15.956</date>
  <millis>1592254875956</millis>
  <nanos>75432</nanos>
  <sequence>7</sequence>
  <logger>tinylogger</logger>
  <level>INFO</level>
  <class>xml-levels.c</class>
  <method>main</method>
  <thread>515081</thread>
  <message>info level</message>
</record>
<record>
  <date>2020-06-15T17:01:15.956</date>
  <millis>1592254875956</millis>
  <nanos>113148</nanos>
  <sequence>8</sequence>
  <logger>tinylogger</logger>
  <level>CONFIG</level>
  <class>xml-levels.c</class>
  <method>main</method>
  <thread>515081</thread>
  <message>config level</message>
</record>
<record>
  <date>2020-06-15T17:01:15.956</date>
  <millis>1592254875956</millis>
  <nanos>150706</nanos>
  <sequence>9</sequence>
  <logger>tinylogger</logger>
  <level>600</level>
  <class>xml-levels.c</class>
  <method>main</method>
  <thread>515081</thread>
  <message>debug level</message>
</record>
<record>
  <date>2020-06-15T17:01:15.956</date>
  <millis>1592254875956</millis>
  <nanos>188710</nanos>
  <sequence>10</sequence>
  <logger>tinylogger</logger>
  <level>FINE</level>
  <class>xml-levels.c</class>
  <method>main</method>
  <thread>515081</thread>
  <message>fine level</message>
</record>
<record>
  <date>2020-06-15T17:01:15.956</date>
  <millis>1592254875956</millis>
  <nanos>227077</nanos>
  <sequence>11</sequence>
  <logger>tinylogger</logger>
  <level>FINER</level>
  <class>xml-levels.c</class>
  <method>main</method>
  <thread>515081</thread>
  <message>finer level</message>
</record>
<record>
  <date>2020-06-15T17:01:15.956</date>
  <millis>1592254875956</millis>
  <nanos>265354</nanos>
  <sequence>12</sequence>
  <logger>tinylogger</logger>
  <level>FINEST</level>
  <class>xml-levels.c</class>
  <method>main</method>
  <thread>515081</thread>
  <message>finest level</message>
</record>
</log>

```
Re-read by a log reader:

```
Jun 15, 2020 5:01:15 PM xml-levels.c main
1300: emerg level
Jun 15, 2020 5:01:15 PM xml-levels.c main
1200: alert level
Jun 15, 2020 5:01:15 PM xml-levels.c main
1100: crit level
Jun 15, 2020 5:01:15 PM xml-levels.c main
SEVERE: severe level
Jun 15, 2020 5:01:15 PM xml-levels.c main
950: err level
Jun 15, 2020 5:01:15 PM xml-levels.c main
WARNING: warning level
Jun 15, 2020 5:01:15 PM xml-levels.c main
850: notice level
Jun 15, 2020 5:01:15 PM xml-levels.c main
INFO: info level
Jun 15, 2020 5:01:15 PM xml-levels.c main
CONFIG: config level
Jun 15, 2020 5:01:15 PM xml-levels.c main
600: debug level
Jun 15, 2020 5:01:15 PM xml-levels.c main
FINE: fine level
Jun 15, 2020 5:01:15 PM xml-levels.c main
FINER: finer level
Jun 15, 2020 5:01:15 PM xml-levels.c main
FINEST: finest level
```

[guide](./guide.md)

