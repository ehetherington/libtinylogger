/*
 * (C) 2020 Edward Hetherington
 * This code is licensed under MIT license (see LICENSE in top dir for details)
 */

/** @file       xml_formatter.c
 *  @brief      XML Message formatting and output
 *  @author     Edward Hetherington
 */

#include "config.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#define _GNU_SOURCE	/**< for syscall SYS_gettid */
#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include <unistd.h>
#include <sys/syscall.h>

#include "tinylogger.h"
#include "private.h"

extern struct log_label log_labels[];

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#define XML_AMP		"&amp;"		/**< ampersand    */
#define XML_LT		"&lt;"		/**< less than    */
#define XML_GT		"&gt;"		/**< greater than */
#define XML_QUOT	"&quot;"	/**< quote        */
#define XML_APOS	"&apos;"	/**< apostrophe   */

#define LABEL_LEN 16	/**< quiet doxygen */

#define HEAD_1	"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>"
#define HEAD_2	"<!DOCTYPE log SYSTEM \"logger.dtd\">"
#define HEAD_3	"<log>"
#define TAIL_1	"</log>"

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

static char java_level[LL_N_VALUES][LABEL_LEN] = {0};
static char *get_level(int level) {
	if (java_level[0][0] == '\0') {
		for (int n = 0; n < LL_N_VALUES; n++) {
			switch (n) {
				case LL_SEVERE:
				case LL_WARNING:
				case LL_CONFIG:
				case LL_FINE:
				case LL_INFO:
				case LL_FINER:
				case LL_FINEST: {
					snprintf(java_level[n], LABEL_LEN,
						log_labels[n].english);
				} break;
				default:
					snprintf(java_level[n], LABEL_LEN,
						"%d", log_labels[n].java_level);
			}
		}
		/*
		for (int n = 0; n < LL_N_VALUES; n++) {
			fprintf(stderr, "%7s %s\n",
				log_labels[n].english, java_level[n]);
		}
		*/
	}
	return java_level[level];
}

static char *entity(int character) {
	switch (character) {
		case '&': return XML_AMP;
		case '<': return XML_LT;
		case '>': return XML_GT;
		case '"': return XML_QUOT;
		case '\'': return XML_APOS;
		default: return NULL;
	}
}

 /*
 * @brief Escape &apos;\b&apos;, &apos;\f&apos;, &apos;\n&apos;, &apos;\r&apos;, &apos;\t&apos;, &apos;\"&apos;, and &apos;\\&apos;
 */
static int sequence = 0;

#ifndef DOXYGEN_SHOULD_SKIP_THIS
/**
 * @fn char *escape_json(char const *, char *, int)
 * @brief Escape &apos;\\b&apos;, &apos;\\f&apos;,
 * &apos;\\n&apos;,  &apos;\\r&apos;,
 * &apos;\\t&apos;, &apos;\\"&apos; and
 * &apos;\\\\&apos;.
 *
 * No special treatment of non-ascii characters is performed.
 * @param input the string to escape
 * @param buf a buffer to place the escaped output
 * @param len the length of that buffer.
 */
char *escape_json(char const *input, char *buf, int len) {
	char const *ptr_in;
	char *ptr_out;
	int n_written;

	if ((input == NULL) || (buf == NULL) || (len == 0)) return NULL;

	for (ptr_in = input, ptr_out = buf;
		*ptr_in != '\0' && (ptr_out < buf + len);
		ptr_in++) {

		char *esc = NULL;
		switch (*ptr_in) {
			case '\b': esc = "\\\b"; break; 
			case '\f': esc = "\\\f"; break;
			case '\n': esc = "\\\n"; break;
			case '\r': esc = "\\\r"; break;
			case '\t': esc = "\\\t"; break;
			case '\"': esc = "\\\""; break;
			case '\\': esc = "\\\\"; break;
		}
		if (esc != NULL) {
			n_written =
				snprintf(ptr_out, len - (ptr_out - buf), "%s", esc);
			ptr_out += n_written;
		} else {
			*ptr_out++ = *ptr_in;
		}
	}

	*ptr_out = '\0';

	return buf;
}
#endif /* DOXYGEN_SHOULD_SKIP_THIS */

/**
 * @fn char *escape_xml(char const *, char *, int)
 * @brief Escape '&', '<', '>', '\"', '\\'
 * No special treatment of non-ascii characters is performed.
 * @param input the string to escape
 * @param buf a buffer to place the escaped output
 * @param len the length of that buffer.
 */
static char *escape_xml(char const *input, char *buf, int len) {
	char const *ptr_in;
	char *ptr_out;
	char *substitute;
	int n_written;

	if ((input == NULL) || (buf == NULL) || (len == 0)) return NULL;

	for (ptr_in = input, ptr_out = buf;
		*ptr_in != '\0' && (ptr_out < buf + len);
		ptr_in++) {
		if ((substitute = entity(*ptr_in)) != NULL) {
			/*
			printf("replacing %c with %s\n", *ptr_in, substitute);
			*/
			n_written =
				snprintf(ptr_out, len - (ptr_out - buf), "%s", substitute);
			ptr_out += n_written;
		} else {
			*ptr_out++ = *ptr_in;
		}
	}

	*ptr_out = '\0';

	return buf;
}

static int do_start(FILE *stream) {
	return fprintf(stream, "<record>\n");
}

static int do_date(FILE *stream, const char *date) { 
	return fprintf(stream, "  <date>%s</date>\n", date);
}

static int do_millis(FILE *stream, const long int millis) {
	return fprintf(stream, "  <millis>%ld</millis>\n", millis);
}

static int do_nanos(FILE *stream, const long int nanos) {
	return fprintf(stream, "  <nanos>%ld</nanos>\n", nanos);
}

static int do_sequence(FILE *stream, const int sequence) {
	return fprintf(stream, "  <sequence>%d</sequence>\n", sequence);
}

static int do_logger(FILE *stream, const char *logger) {
	return fprintf(stream, "  <logger>%s</logger>\n", logger);
}

static int do_level(FILE *stream, const char *level) {
	return fprintf(stream, "  <level>%s</level>\n", level);
}

static int do_class(FILE *stream, const char *class) {
	return fprintf(stream, "  <class>%s</class>\n", class);
}

static int do_method(FILE *stream, const char *method) {
	return fprintf(stream, "  <method>%s</method>\n", method);
}

static int do_thread(FILE *stream, const pid_t thread) {
	return fprintf(stream, "  <thread>%d</thread>\n", thread);
}

static int do_message(FILE *stream, const char *message) {
	return fprintf(stream, "  <message>%s</message>\n", message);
}

static int do_end(FILE *stream) {
	return fprintf(stream, "</record>\n");
}

/**
 * @fn int log_xml_do_head(FILE *stream)
 * @brief Write the xml prolog
 *
 * The prolog is identical to the default one produced by
 * java.util.logging.XMLFormatter.
 *
 * The opening &lt;log&gt; is written.
 *
 * @param stream the stream in use
 * @return the number of bytes written
 */
int log_xml_do_head(FILE *stream) {
	int n_written = 0;
	n_written += fprintf(stream, "%s\n", HEAD_1);
	n_written += fprintf(stream, "%s\n", HEAD_2);
	n_written += fprintf(stream, "%s\n", HEAD_3);
	return n_written;
}

/**
 * @fn int log_xml_do_tail(FILE *stream)
 * @brief write the closing &lt;/log&gt;.
 *
 * The closing &lt;/log&gt; is written.
 *
 * @param stream the stream in use
 * @return the number of bytes written
 */
int log_xml_do_tail(FILE *stream) {
	return fprintf(stream, "%s\n", TAIL_1);
}

/**
 * @fn int log_fmt_xml(FILE *stream, struct timespec *ts, int level,
 * const char *file, const char *function, int line, char *msg)
 * @brief Output messages with timestamp, level and message.
 * @param stream the output stream to write to
 * @param ts the struct timespec timestamp
 * @param level the log level to print
 * @param file the name of the file to print
 * @param function the name of the function to print
 * @param line the line number to print
 * @param msg the actual use message to print
 * @return the number of characters written.
 *
 * Example output:
```
<record>
  <date>2020-06-14T08:46:36.624</date>
  <millis>1592138796624</millis>
  <nanos>981722</nanos>
  <sequence>0</sequence>
  <logger>tinylogger</logger>
  <level>TODO</level>
  <class>formats.c</class>
  <method>main</method>
  <thread>393487</thread>
  <message>this message uses the &quot;&lg;xml&gt;&quot; format &amp; it&apos;s easy!</message>
</record>
```
 */
int log_fmt_xml(FILE *stream, struct timespec *ts, int level,
    const char *file, const char *function, int line, char *msg) {
    char date[TIMESTAMP_LEN];
	char buf[BUFSIZ] = {0};
	long int time_millis;
	long int time_nanos;

	/*
	fprintf(stderr, "sizeof time_t = %lu\n", sizeof(time_t));
	fprintf(stderr, "sizeof long int = %lu\n", sizeof(long int));
	fprintf(stderr, "sizeof pid_t = %lu\n", sizeof(pid_t));

	fprintf(stderr, "tv_sec was %ld\n", ts->tv_sec);
	fprintf(stderr, "tv_nsec was %ld\n", ts->tv_nsec);
	*/

	time_millis = ts->tv_sec * 1000 + ts->tv_nsec / 1000000;
	time_nanos = ts->tv_nsec % 1000000;

	int n_written = 0;

	// TODO: escape file and function also
	escape_xml(msg, buf, sizeof(buf));

    log_format_timestamp(ts, FMT_UTC_OFFSET | FMT_ISO | SP_MILLI,
		date, sizeof(date));
	n_written += do_start(stream);
	n_written += do_date(stream, date);
	n_written += do_millis(stream, time_millis);
	n_written += do_nanos(stream, time_nanos);
	n_written += do_sequence(stream, sequence++);
	n_written += do_logger(stream, "tinylogger");
	n_written += do_level(stream, get_level(level));
	n_written += do_class(stream, file);
	n_written += do_method(stream, function);
	n_written += do_thread(stream, syscall(SYS_gettid));
	n_written += do_message(stream, buf);
	n_written += do_end(stream);

	return n_written;
}

