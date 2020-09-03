## JSON reader
There is a companion project at https://github.com/ehetherington/JSON-LogReader
that has a Java data model for reading JSON log files produced by libtinylogger.

It uses FasterXML jackson and requires JDK8, and a POM file is supplied to
build it with Maven.

After building, it can be run like this (assuming you have a tinylogger.json
log file handy):
```
$ java -cp ./target/verify_log-1.0-SNAPSHOT-jar-with-dependencies.jar jsonlogreader.JsonLogReader tinylogger.json
```
