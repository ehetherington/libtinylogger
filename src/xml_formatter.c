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
// from kernel/sched.h
#define TASK_COMM_LEN 16
#define NAME_LEN TASK_COMM_LEN  // includes null termination
#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
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

static int do_xml_start(FILE *stream) {
	return fprintf(stream, "<record>\n");
}

static int do_xml_text(FILE *stream, const char *label, const char *value) {
	return fprintf(stream, "  <%s>%s</%s>\n", label, value, label);
}

static int do_xml_long(FILE *stream, const char *label, const long value) {
	return fprintf(stream, "  <%s>%ld</%s>\n", label, value, label);
}

static int do_xml_end(FILE *stream) {
	return fprintf(stream, "</record>\n");
}

/**
 * @fn int log_do_xml_head(FILE *stream)
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
int log_do_xml_head(FILE *stream) {
	int n_written = 0;
	n_written += fprintf(stream, "%s\n", HEAD_1);
	n_written += fprintf(stream, "%s\n", HEAD_2);
	n_written += fprintf(stream, "%s\n", HEAD_3);
	return n_written;
}

/**
 * @fn int log_do_xml_tail(FILE *stream)
 * @brief write the closing &lt;/log&gt;.
 *
 * The closing &lt;/log&gt; is written.
 *
 * @param stream the stream in use
 * @return the number of bytes written
 */
int log_do_xml_tail(FILE *stream) {
	return fprintf(stream, "%s\n", TAIL_1);
}

/**
 * @fn int log_fmt_xml(FILE *stream, int sequence, struct timespec *ts, int level,
 * const char *file, const char *function, int line, char *msg)
 * @brief Output messages with timestamp, level and message.
 * @param stream the output stream to write to
 * @param sequence the sequence number of the message
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
int log_fmt_xml(FILE *stream, int sequence, struct timespec *ts, int level,
    const char *file, const char *function, int line, char *msg) {
    char date[TIMESTAMP_LEN];
	char buf[BUFSIZ] = {0};
	long int time_millis;
	long int time_nanos;

	time_millis = ts->tv_sec * 1000 + ts->tv_nsec / 1000000;
	time_nanos = ts->tv_nsec % 1000000;

	int n_written = 0;

	// TODO: escape file and function also
	escape_xml(msg, buf, sizeof(buf));

    log_format_timestamp(ts, FMT_UTC_OFFSET | FMT_ISO | SP_MILLI,
		date, sizeof(date));
	n_written += do_xml_start(stream);
	n_written += do_xml_text(stream, "date", date);
	n_written += do_xml_long(stream, "millis", time_millis);
	n_written += do_xml_long(stream, "nanos", time_nanos);
	n_written += do_xml_long(stream, "sequence", sequence);
	n_written += do_xml_text(stream, "logger", "tinylogger");
	n_written += do_xml_text(stream, "level", get_level(level));
	n_written += do_xml_text(stream, "class", file);
	n_written += do_xml_text(stream, "method", function);
	n_written += do_xml_long(stream, "thread", syscall(SYS_gettid));
	n_written += do_xml_text(stream, "message", buf);
	n_written += do_xml_end(stream);

	return n_written;
}
