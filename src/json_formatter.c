/*
 * (C) 2020 Edward Hetherington
 * This code is licensed under MIT license (see LICENSE in top dir for details)
 */

/** @file       json_formatter.c
 *  @brief      Json Message formatting and output
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

#define LABEL_LEN 16	/**< quiet doxygen */

/**
 * @fn char *escape_json(char const *, char *, int)
 * @brief Escape &apos;\\b&apos;, &apos;\\f&apos;,
 *     &apos;\\n&apos;,  &apos;\\r&apos;,
 *     &apos;\\t&apos;, &apos;\\"&apos; and
 *     &apos;\\\\&apos;.
 *
 * No special treatment of non-ascii characters is performed.
 *
 * @param input the string to escape
 * @param buf a buffer to place the escaped output
 * @param len the length of that buffer.
 */
static char *escape_json(char const *input, char *buf, int len) {
	char const *ptr_in;
	char *ptr_out;
	int n_written;

	if ((input == NULL) || (buf == NULL) || (len == 0)) return NULL;

	for (ptr_in = input, ptr_out = buf;
		*ptr_in != '\0' && (ptr_out < buf + len);
		ptr_in++) {

		char *esc = NULL;
		switch (*ptr_in) {
			case '\b': esc = "\\b"; break; 
			case '\f': esc = "\\f"; break;
			case '\n': esc = "\\n"; break;
			case '\r': esc = "\\r"; break;
			case '\t': esc = "\\t"; break;
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

/**
 * @fn int log_do_json_head(FILE *stream)
 * @brief Write the json prolog
 *
 * The opening
 * ```
 * {
 *   "records" : [ 
 * ```
 * is written.
 * @param stream the stream in use
 * @return the number of bytes written
 */
int log_do_json_head(FILE *stream) {
	return fprintf(stream, "{\n  \"records\" : [");
}

/**
 * @fn int log_do_json_tail(FILE *stream)
 * @brief Write the json epilog
 *
 * The closing
 * ```
 *  ]
 * }
 * ```
 * is written.
 * @param stream the stream in use
 * @return the number of bytes written
 */
int log_do_json_tail(FILE *stream) {
	return fprintf(stream, " ]\n}\n");
}

static int do_json_start(FILE *stream, int sequence) {
	return fprintf(stream, "%s  {\n", sequence > 1 ? "," : "");
}

static int do_json_timespec(FILE *stream, struct timespec *timespec) {
	int n_written = 0;
	n_written += fprintf(stream, "    \"timespec\" : {\n"
								 "      \"sec\" : %ld,\n",
		timespec->tv_sec);
	n_written += fprintf(stream, "      \"nsec\" : %ld\n    },\n",
		timespec->tv_nsec);
	return n_written;
}

static int do_json_text(FILE *stream,
	char const *label, char const *value, bool do_comma) {
	return fprintf(stream, "    \"%s\" : \"%s\"%s\n",
		label, value, do_comma ? "," : "");
}

static int do_json_int(FILE *stream,
	char const *label, long const value, bool do_comma) {
	return fprintf(stream, "    \"%s\" : %ld%s\n",
		label, value, do_comma ? "," : "");
}

static int do_json_end(FILE *stream) {
	return fprintf(stream, "  }");
}

/**
 * @fn int log_fmt_json(FILE *stream, int sequence, struct timespec *ts, int level,
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
```
 */
int log_fmt_json(FILE *stream, int sequence, struct timespec *ts, int level,
    const char *file, const char *function, int line, char *msg) {
    char date[TIMESTAMP_LEN];
	char buf[BUFSIZ] = {0};

	pthread_t thread = pthread_self();
	char thread_name[NAME_LEN];
	int rc;
	rc = pthread_getname_np(thread, thread_name, sizeof(thread_name));
	if (rc != 0) {
		snprintf(thread_name, sizeof(thread_name), "unknown");
	}

	int n_written = 0;

	// TODO: escape file and function also
	escape_json(msg, buf, sizeof(buf));

    log_format_timestamp(ts, FMT_UTC_OFFSET | FMT_ISO | SP_NANO,
		date, sizeof(date));
	n_written += do_json_start(stream, sequence);
	n_written += do_json_text(stream, "isoDateTime", date, true);
	n_written += do_json_timespec(stream, ts);
	n_written += do_json_int(stream, "sequence", sequence, true);
	n_written += do_json_text(stream, "logger", "tinylogger", true);
	n_written += do_json_text(stream, "level", log_labels[level].english, true);
	n_written += do_json_text(stream, "file", file, true);
	n_written += do_json_text(stream, "function", function, true);
	n_written += do_json_int(stream, "line", line, true);
	n_written += do_json_int(stream, "threadId", syscall(SYS_gettid), true);
	n_written += do_json_text(stream, "threadName", thread_name, true);
	n_written += do_json_text(stream, "message", buf, false);
	n_written += do_json_end(stream);

	return n_written;
}

