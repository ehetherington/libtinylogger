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
#include <ctype.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
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
 * @param input the string to escape
 * @param buf a buffer to place the escaped output
 * @param len the length of that buffer - termination null will be added
 */
static char *escape_json(char const *input, char *buf, int len) {
	char const *ptr_in;
	char *ptr_out;

	// len must be at least 1 for the terminating null
	if ((input == NULL) || (buf == NULL) || (len < 1)) return NULL;

	// stop when there isn't even room for an un-escaped char
	for (ptr_in = input, ptr_out = buf;
		*ptr_in != '\0' && (ptr_out < (buf + len) - 1);
		ptr_in++) {

		char *esc = NULL;

		switch (*ptr_in) {
			case 0x00: esc = "\\u0000"; break; // hmmm...
			case 0x01: esc = "\\u0001"; break;
			case 0x02: esc = "\\u0002"; break;
			case 0x03: esc = "\\u0003"; break;
			case 0x04: esc = "\\u0004"; break;
			case 0x05: esc = "\\u0005"; break;
			case 0x06: esc = "\\u0006"; break;
			case 0x07: esc = "\\u0007"; break;
			case '\b': esc = "\\b";     break; // 0x08
			case '\t': esc = "\\t";     break; // 0x09
			case '\n': esc = "\\n";     break; // 0x0A
			case 0x0B: esc = "\\u000B"; break;
			case '\f': esc = "\\f";     break; // 0x0C
			case '\r': esc = "\\r";     break; // 0x0D
			case 0x0E: esc = "\\u000E"; break;
			case 0x0F: esc = "\\u000F"; break;

			case 0x10: esc = "\\u0010"; break;
			case 0x11: esc = "\\u0011"; break;
			case 0x12: esc = "\\u0012"; break;
			case 0x13: esc = "\\u0013"; break;
			case 0x14: esc = "\\u0014"; break;
			case 0x15: esc = "\\u0015"; break;
			case 0x16: esc = "\\u0016"; break;
			case 0x17: esc = "\\u0017"; break;
			case 0x18: esc = "\\u0018"; break;
			case 0x19: esc = "\\u0019"; break;
			case 0x1A: esc = "\\u001A"; break;
			case 0x1B: esc = "\\u001B"; break;
			case 0x1C: esc = "\\u001C"; break;
			case 0x1D: esc = "\\u001D"; break;
			case 0x1E: esc = "\\u001E"; break;
			case 0x1F: esc = "\\u001F"; break;

			case '\"': esc = "\\\""; break;  // 0x22 ('"')
			case '\\': esc = "\\\\"; break;  // 0x5C ('\")
		}

		if (esc != NULL) {
			// Make sure we can fit the whole escape sequence.
			// Truncate output at the first character that doesn't fit.
			// Leave room for terminating null.
			size_t escape_len = strlen(esc);
			size_t n_available = len - (ptr_out - buf) - 1;

			if (escape_len > n_available) break;

			memcpy(ptr_out, esc, escape_len);
			ptr_out += escape_len;
		} else {
			*ptr_out++ = *ptr_in;
		}
	}

	*ptr_out = '\0';

	return buf;
}

#if ENABLE_JSON_HEADER
#define HOSTNAME_FILE "/proc/sys/kernel/hostname"
static inline char *get_hostname() {
	static char hostname[64] = {0};
	static bool init_done = false;
	int fd;
	char *newline;

	if (init_done) return hostname;

	fd = open(HOSTNAME_FILE, O_RDONLY);
	if (fd != -1) {
		read(fd, hostname, sizeof(hostname) - 1);
		newline = index(hostname, '\n');
		if (newline != NULL) *newline = '\0';
	}
	init_done = true;

	return hostname;
}
#endif /* ENABLE_JSON_HEADER */

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

#if ENABLE_JSON_HEADER
static inline int do_header(FILE *stream, char *notes) {
	char notes_buf[BUFSIZ] = {0};
    char date[TIMESTAMP_LEN + TIMEZONE_LEN];
	struct timespec ts;

	// get an timestamp
	clock_gettime(CLOCK_REALTIME, &ts);

#if ENABLE_TIMEZONE
	json_format_timestamp(&ts, date, sizeof(date));
#else
	log_format_timestamp(&ts, FMT_UTC_OFFSET | FMT_ISO | SP_NANO,
		date, sizeof(date));
#endif /* ENABLE_TIMEZONE */

	if (notes == NULL) {
		snprintf(notes_buf, sizeof(notes_buf), "null");
	} else {
		// leave room for enclosing quotes
		escape_json(notes, notes_buf + 1, sizeof(notes_buf) - 2);
		notes_buf[0] = '"';
		strcat(notes_buf, "\"");
	}

	return fprintf(stream,	"  \"header\" : {\n"
							"    \"startDate\" : \"%s\",\n"
							"    \"hostname\" : \"%s\",\n"
							"    \"notes\" : %s\n  },",
					date, get_hostname(), notes_buf);
}
#endif /* ENABLE_JSON_HEADER */

/**
 * @fn int log_do_json_head(FILE *stream, char *notes)
 * @brief Write the json prolog
 *
 * The opening
 * ```
 * {
 *   "records" : [ 
 * ```
 * is written.
 * @param stream the stream in use
 * @param notes the notes to be included in the header (may be NULL)
 * @return the number of bytes written
 * @note This is not part of the API, and should not be public. This is an
 * artifact of it being needed by the main tinylogger.c file.
 */
int log_do_json_head(FILE *stream, char *notes) {
	int n_written = 0;
#if ENABLE_JSON_HEADER
	n_written = fprintf(stream, "{\n");
	n_written += do_header(stream, notes);
	n_written += fprintf(stream, " \"records\" : [");
#else
	(void) notes;
	n_written = fprintf(stream, "{\n  \"records\" : [");
#endif
	return n_written;
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
 * @note This is not part of the API, and should not be public. This is an
 * artifact of it being needed by the main tinylogger.c file.
 */
int log_do_json_tail(FILE *stream) {
	return fprintf(stream, " ]\n}\n");
}

static int do_json_start(FILE *stream, int sequence, bool records) {
	return records ?
				fprintf(stream, "{\n") :
				fprintf(stream, "%s  {\n", sequence > 1 ? "," : "");
}

static int do_json_timespec(FILE *stream, struct timespec *timespec) {
	return fprintf(stream,	"    \"timespec\" : {\n"
							"      \"sec\" : %ld,\n"
							"      \"nsec\" : %ld\n    },\n",
					timespec->tv_sec, timespec->tv_nsec);
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

/**
 * End-of-Record
 * If we are producing a Log, the next record will insert a comma to separate
 * the list of record elements, or the End-of-Log will be appended at stream
 * end.
 * If we are producing a list of records, no comma will be necessary.
 */
static int do_json_end(FILE *stream, bool records) {
	return fprintf(stream, records ? "}\n" :"  }");
}

/**
 * @fn int _log_fmt_json(FILE *stream, int sequence, struct timespec *ts, int level,
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
 * @param records select a log with an array of records, or a stream of records
 * @return the number of characters written.
 */
static int _log_fmt_json(FILE *stream, int sequence, struct timespec *ts, int level,
    const char *file, const char *function, int line, char *msg, bool records) {
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

	// The message must be properly escaped for the JSON output
	// TODO: escape file also ???
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

	n_written += do_json_start(stream, sequence, records);
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
	n_written += do_json_end(stream, records);

	return n_written;
}

/**
 * @fn int log_fmt_json(FILE *stream, int sequence,
 *           struct timespec *ts, int level,
 *           const char *file, const char *function, int line, char *msg)
 * @brief Output messages with timestamp, level and message.
 *
 * @details Use log_fmt_json for logs that are a log object with an array
 *          of records. While the head and tail of the Log are separately
 *          generated, the records in a log must be separated by commas,
 *          as they are elements of an array.
 *
 * @param stream the output stream to write to
 * @param sequence the sequence number of the message
 * @param ts the struct timespec timestamp
 * @param level the log level to print
 * @param file the name of the file to print
 * @param function the name of the function to print
 * @param line the line number to print
 * @param msg the actual use message to print
 * @return the number of characters written.
 */
int log_fmt_json(FILE *stream,
        int sequence, struct timespec *ts, int level,
        const char *file, const char *function, int line, char *msg) {
	return _log_fmt_json(stream, sequence, ts, level,
		file, function, line, msg, false);
}

/**
 * @fn int log_fmt_json_records(FILE *stream, int sequence,
 *           struct timespec *ts, int level,
 *           const char *file, const char *function, int line, char *msg)
 * @brief Output messages with timestamp, level and message.
 *
 * @details Use log_fmt_json_records for logs that are a series of record
 *          objects. Each record is a separate object - no separating commas.
 *
 * @param stream the output stream to write to
 * @param sequence the sequence number of the message
 * @param ts the struct timespec timestamp
 * @param level the log level to print
 * @param file the name of the file to print
 * @param function the name of the function to print
 * @param line the line number to print
 * @param msg the actual use message to print
 * @return the number of characters written.
 */
int log_fmt_json_records(FILE *stream,
        int sequence, struct timespec *ts, int level,
        const char *file, const char *function, int line, char *msg) {
	return _log_fmt_json(stream, sequence, ts, level,
		file, function, line, msg, true);
}
