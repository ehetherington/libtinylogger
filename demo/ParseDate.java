import java.io.PrintWriter;
import java.io.StringWriter;
import java.time.format.DateTimeFormatter;
import static java.time.temporal.ChronoField.NANO_OF_SECOND;
import static java.time.temporal.ChronoField.INSTANT_SECONDS;
import java.time.temporal.TemporalAccessor;
import java.util.Date;
import java.util.Iterator;
import java.util.List;

/**
 * Short tips for reading the date and time elements in the XML format logs.
 *
 * The elemeents available in the record element related to the date are:
 *
 * - date: the date string, such as "2020-06-19T11:56:35.938955247-04:00".
 *
 * - millis: the time in milliseconds since the EPOC.
 *
 * - nanos: this element was added in JDK9, so logs produced by Java runtimes
 *   before that will not have this element. Reqardless, you can use previous
 *   runtimes and JAXB with the current DTD to produce a binding to read such
 *   files. It is used to extend the precision of the timestamp. It is appended
 *   to the millis value. Its range is 0 to 999999.
 */

public class ParseDate {
	public static void main(String[] args) {
		/*
		 * values copied from an xml record
		 */
		String dateWithOffset = "2020-06-19T11:56:35.938955247-04:00";
		String dateNoOffset = "2020-06-15T12:59:55.289";
		long millis = Long.parseLong("1592582195938");
		long nanos = Long.parseLong("955247");


		/*
		 * Print what we start with.
		 * NOTE: the nanos element of the record is the number of nanoseconds
		 * to add to the milliseconds. It's range is from 0.000000000 to
		 * 0.000999999 seconds.
		 */
		System.out.println("DATE:   " + dateWithOffset);
		System.out.println("MILLIS: " + millis);
		System.out.println("NANOS:  " + nanos);

		/*
		 * Parse the date string using DateTimeFormatter.
		 * We get the offset in seconds from UTC correctly.
		 * (-4 * 60 * 60 = -14400)
		 */
		TemporalAccessor isoOffsetDateTime =
			DateTimeFormatter.ISO_DATE_TIME.parse(dateWithOffset);
		System.out.println("Fully parsed: " + isoOffsetDateTime);

		/*
		 * print it out in different styles...
		 */
		DateTimeFormatter formatter;

		formatter = DateTimeFormatter.ISO_LOCAL_DATE_TIME;
		System.out.println(formatter.format(isoOffsetDateTime));

		formatter = DateTimeFormatter.RFC_1123_DATE_TIME;
		System.out.println(formatter.format(isoOffsetDateTime));

		/*
		 * We can pick of a variety of fields:
		 */
		long secondsSinceEpoch = isoOffsetDateTime.getLong(INSTANT_SECONDS);
		System.out.println("Seconds: " + secondsSinceEpoch);
		long nanosAgain = isoOffsetDateTime.getLong(NANO_OF_SECOND);
		System.out.println("Nanos: " + nanosAgain);

		/*
		 * If we don't care about the nanosecond precision and the UTC offset,
		 * we can us a plain old date, we can construct it directly with
		 * millis.
		 */
		Date date = new Date(millis);
		System.out.println("Plain old Date: " + date);

		/*
		 * Parse the date string with NO OFFSET using DateTimeFormatter.
		 * The formatter MUST MATCH.
		 * We get the offset in seconds from UTC correctly.
		 * (-4 * 60 * 60 = -14400)
		 */
		System.out.println("\nDate/Time with no offset");
		TemporalAccessor isoDateTime =
			DateTimeFormatter.ISO_DATE_TIME.parse(dateNoOffset);
		System.out.println("Fully parsed: " + isoDateTime);

		/*
		 * The output formatter needs to match. RFC_1123_DATE_TIME won't work.
		 */
		formatter = DateTimeFormatter.ISO_LOCAL_DATE_TIME;
		System.out.println(formatter.format(isoDateTime));
	}
}
