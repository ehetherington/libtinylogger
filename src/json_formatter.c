/*
 * (C) 2020 Edward Hetherington
 * This code is licensed under MIT license (see LICENSE in top dir for details)
 */

/** @file       json_formatter.c
 *  @brief      Json Message formatting and output
 *  @details	The json output is an array of records. The record objects are:
 *
 *  - isoDateTime The message timestamp - includes UTC offset
 *  - timespec    The struct timespec timestamp (basis of isoDateTime)
 *    - sec
 *    - nsec
 *  - sequence    The sequence number of the message. Starts at 1.
 *  - logger      Always "tinylogger".
 *  - file        __FILE__ captured by the calling macro
 *  - function    __function__ captured by the calling macro
 *  - line        __LINE__ captured by the calling macro
 *  - threadId    The linux thread id of the caller.
 *  - threadName  The linux thread name of the caller.
 *  - message     The user message.
 *
 * Example output:
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
 *  @author     Edward Hetherington
 *
 */

#include "config.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#define _GNU_SOURCE	/**< for syscall SYS_gettid */
// from kernel/sched.h
#define TASK_COMM_LEN 16
#define NAME_LEN TASK_COMM_LEN  /**< includes null termination */

/* America/Argentina/ComodRivadavia is currently longest at 32 chars */
#define TIMEZONE_LEN 40	/**< TODO: find an actual max */
#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <sys/syscall.h>
#include <linux/limits.h>

#include "tinylogger.h"
#include "private.h"

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
 * @details Not really part of the API. Public so that tinylogger.c can use it.
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

// not re-entrant, but only called from mutex protected code
static inline char *get_timezone() {
	static char tz[PATH_MAX] = {0};
	static bool init_done = false;
	char *result;

	if (init_done) return tz;

	result = log_get_timezone(tz + 1, sizeof(tz) - 2);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-truncation"
#pragma GCC diagnostic ignored "-Wstringop-overflow"
	if (result == (tz + 1)) {
		tz[0] = '[';
		strncat(tz, "]", sizeof(tz));
	}
#pragma GCC diagnostic pop

// for generating a test file with json-timezones.c
//#define TIMEZONE_TEST
#ifndef TIMEZONE_TEST
	init_done = true;
#endif

	return tz;
}

static inline void json_format_timestamp(struct timespec *ts,
	char *buf, int buf_len) {
	log_format_timestamp(ts, FMT_UTC_OFFSET | FMT_ISO | SP_NANO, buf, buf_len);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-truncation"
#pragma GCC diagnostic ignored "-Wstringop-overflow"
	strncat(buf, get_timezone(), buf_len);
#pragma GCC diagnostic pop

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
 */
int log_fmt_json(FILE *stream, int sequence, struct timespec *ts, int level,
    const char *file, const char *function, int line, char *msg) {
    char date[TIMESTAMP_LEN + TIMEZONE_LEN];
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

	/*
	 * Save some clock cycles if use of timezone is not configured.
	 */
#if ENABLE_TIMEZONE
	json_format_timestamp(ts, date, sizeof(date));
#else
	log_format_timestamp(ts, FMT_UTC_OFFSET | FMT_ISO | SP_NANO,
		date, sizeof(date));
#endif

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

