/*
 * (C) 2020 Edward Hetherington
 * This code is licensed under MIT license (see LICENSE in top dir for details)
 */

/** @file       formatters.c
 *  @brief      Message formatting and output
 *  @author     Edward Hetherington
 */

#include "config.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#define _GNU_SOURCE

// from kernel/sched.h
#define TASK_COMM_LEN 16
#define NAME_LEN TASK_COMM_LEN  // includes null termination

#define FMT_STRING_STD "%04d-%02d-%02d %02d:%02d:%02d.%09ld"
#define FMT_STRING_ISO "%04d-%02d-%02dT%02d:%02d:%02d.%09ld"

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/syscall.h>

#if HAVE_SYSTEMD_SD_DAEMON_H
	#include <systemd/sd-daemon.h>
#else
	#include "alternatives/sd-daemon.h"
#endif

#include "tinylogger.h"
#include "private.h"

/**
 * systemd/sd-daemon.h macro names were used directly (minus the "SD_" prefix).
 * java.util.logging.Level names and values were merged in and mapped to the most
 * appropriate systemd value. Systemd macros were assigned values relative to the
 * java ones.
 */
//struct log_label {
//    char *english;  /**< label used for most formats */
//    char *systemd;  /**< label used for systemd format */
//    int java_level; /**< java equivalent level */
//};

struct log_label log_labels[]  = {
    {"EMERG",   SD_EMERG,  1300},
    {"ALERT",   SD_ALERT,  1200},
    {"CRIT",    SD_CRIT,   1100},
    {"SEVERE",  SD_ERR,    1000}, // masquerade as err for systemd (J)
    {"ERR",     SD_ERR,     950},
    {"WARNING", SD_WARNING, 900}, // (J)
    {"NOTICE",  SD_NOTICE,  850},
    {"INFO",    SD_INFO,    800}, // (J)
    {"CONFIG",  SD_INFO,    700}, // masquerade as info for systemd (J)
    {"DEBUG",   SD_DEBUG,   600},
    {"FINE",    SD_DEBUG,   500}, // masquerade as debug for systemd (J)
    {"FINER",   SD_DEBUG,   400}, // masquerade as debug for systemd (J)
    {"FINEST",  SD_DEBUG,   300}, // masquerade as debug for systemd (J)
};

/**
 * @fn LOG_LEVEL log_get_level(const char *label)
 * @brief Look up LOG_LEVEL for a certain string.
 *
 * Look up the numeric (enum) value using the label as a key. Useful
 * for command line argument processing to see if a user supplied option
 * is valid, and if so, set the requested LOG_LEVEL.
 * @param label the string in question
 * @return LL_INVALID if the string is not valid log level, otherwise the
 * corresponding LOG_LEVEL.
 */
LOG_LEVEL log_get_level(const char *label) {
    // alias for the least important level
    if (strcasecmp(label, "ALL") == 0) return LL_N_VALUES - 1;
    if (strcasecmp(label, "OFF") == 0) return LL_OFF;

    for (int n = 0; n < LL_N_VALUES; n++) {
        if (strcasecmp(label, log_labels[n].english) == 0) return n;
    }
    return LL_INVALID;
}

/**
 * @fn void do_offset(struct tm const *tm, char *buf, int len)
 * @brief Tack on the UTC offset to a data/time timestamp.
 * The buffer must be at least 10 characters long to hold the longest
 * offset.
 * @param tm the struct tm containing the offset
 * @param buf The buffer to write the offset to.
 * @param len The length of that buffer.
 */
static void do_offset(struct tm const *tm, char *buf, int len) {
static char *fmt_min  = "%+03ld:%02ld";
static char *fmt_sec  = "%+03ld:%02ld:%02ld";
	long hours, minutes, seconds;
	long off_remainder;

    hours = tm->tm_gmtoff / (60 *60);
    off_remainder = tm->tm_gmtoff % (60 * 60);
    minutes = off_remainder / 60;
    off_remainder = off_remainder % 60;
    seconds = off_remainder;

	if (seconds == 0) {
		snprintf(buf, len, fmt_min, hours, minutes);
	} else {
		snprintf(buf, len, fmt_sec, hours, minutes, seconds);
	}
	/*
	 * Consider adding the timezone.
	printf(" (%s)\n", tm->tm_zone);
	*/
}

/**
 * @fn void log_format_timestamp(struct timespec *ts, SEC_PRECISION precision,
 * 	char *buf, int len)
 * @brief Format the (struct timespec) ts in tb to an ascii string.
 *
 * The fractional seconds appended is specified by the SEC_PRECISION precision.
 * - SP_NONE  no fraction is appended
 * - SP_MILLI .nnn is appended
 * - SP_MICRO .nnnnnn is appended
 * - SP_NANO  .nnnnnnnnn is appended
 *
 * @param ts the previously obtained struct timespec timestamp.
 * @param precision the precision of the fraction of second to display
 * @param buf the buffer to format the timestamp to
 * @param len the length of that buffer. Must be >= 30
 */
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat-truncation"
void log_format_timestamp(struct timespec *ts, SEC_PRECISION precision,
	char *buf, int len) {
	struct tm	tm;
	char const *fmt = FMT_STRING_STD;
	bool want_offset = false;

	if (precision >= LOG_FMT_DELTA) {
		log_format_delta(ts, precision, buf, len);
		return;
	}

	if (precision & FMT_ISO) {
		fmt = FMT_STRING_ISO;
		precision &= ~FMT_ISO;
	}

	if (precision & FMT_UTC_OFFSET) {
		want_offset = true;
		precision &= ~FMT_UTC_OFFSET;
	}

	if ((buf == NULL) || (len < TIMESTAMP_LEN)) {
		fprintf(stderr,
			"log_format_timestamp: internal error - provide %d char buf\n",
				TIMESTAMP_LEN);
		return;
	}

	if (localtime_r(&(ts->tv_sec), &tm) == &tm) {
		snprintf(buf, len, fmt,
			tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
			tm.tm_hour, tm.tm_min, tm.tm_sec,
			ts->tv_nsec);
		switch (precision) {
			case SP_NONE:  buf[19] = '\0'; break;
			case SP_MILLI: buf[23] = '\0'; break;
			case SP_MICRO: buf[26] = '\0'; break;
			case SP_NANO:  buf[29] = '\0'; break;
			default: break;	// quiet warning about no case for ISO_FMT
		}
	} else {
		snprintf(buf, len, "%s", "oops");
	}

	if (want_offset) {
		int end = strlen(buf);
		do_offset(&tm, buf + end, len - end);
	}
}

#pragma GCC diagnostic pop
/**
 * @fn int log_fmt_basic(FILE *, int sequence, struct timespec *, int,
 * const char *, const char *, int, char *)
 * @brief Output messages with just the user message.
 *
 * @param msg the actual user message
 * @param stream the output stream to write to
 * @param sequence the sequence number of the message
 * @param ts the struct timespec timestamp
 * @param level the log level to print
 * @param file the name of the file to print
 * @param function the name of the function to print
 * @param line the line number to print
 * @param msg the actual use message to print
 *
 * Example output:
```
eth0     AF_PACKET (17)
```
 */
int log_fmt_basic(FILE *stream, int sequence, struct timespec *ts, int level,
	const char *file, const char *function, int line, char *msg) {
	return fprintf(stderr, "%s\n", msg);
}

/**
 * @fn int log_fmt_systemd(FILE *, int, struct timespec *, int,
 * const char *, const char *, int, char *)
 * @brief Output messages in systemd compatible format.
 *
 * @param stream the output stream to write to
 * @param sequence the sequence number of the message
 * @param ts the struct timmespec timestamp
 * @param level the log level to print
 * @param file the name of the file to print
 * @param function the name of the function to print
 * @param line the line number to print
 * @param msg the actual use message to print
 * @return the number of characters written.
 *
 * Example output:
```
<7>eth0     AF_PACKET (17)
```
 */
int log_fmt_systemd(FILE *stream, int sequence, struct timespec *ts, int level,
	const char *file, const char *function, int line, char *msg) {
	return fprintf(stream, "%s%s\n", log_labels[level].systemd, msg);
}


/**
 * @fn int log_fmt_standard(FILE *, int sequence, struct timespec *, int,
 * const char *, const char *, int, char *)
 * @brief Output messages with timestamp, level and message.
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
 *
 * Example output:
```
2020-05-25 16:55:18 DEBUG   eth0     AF_PACKET (17)
```
 */
int log_fmt_standard(FILE *stream, int sequence, struct timespec *ts, int level,
	const char *file, const char *function, int line, char *msg) {
	char date[TIMESTAMP_LEN];
	// (-7 - let critical and emergency stick out, Use -9 to align them all)
	log_format_timestamp(ts, SP_NONE, date, sizeof(date));
	return fprintf(stream, "%s %-7s %s\n",
		date, log_labels[level].english, msg);
}

/**
 * @fn int log_fmt_tall(FILE *, int, struct timespec *, int,
 * const char *, const char *, int, char *)
 * @brief Debug format with thread id added.
 *
 * @param stream the output stream to write to
 * @param sequence the sequence number of the message
 * @param ts the struct timmespec timestamp
 * @param level the log level to print
 * @param file the name of the file to print
 * @param function the name of the function to print
 * @param line the line number to print
 * @param msg the actual use message to print
 * @return the number of characters written.
 *
 * Example output:
```
2020-05-25 17:28:17.011 DEBUG   65623:thread_2 eth0     AF_PACKET (17)
```
 */
int log_fmt_tall(FILE *stream, int sequence, struct timespec *ts, int level,
	const char *file, const char *function, int line, char *msg) {
	char date[TIMESTAMP_LEN];
	pthread_t thread = pthread_self();
	char thread_name[NAME_LEN];
	int rc;
	log_format_timestamp(ts, SP_MILLI, date, sizeof(date));
	rc = pthread_getname_np(thread, thread_name, sizeof(thread_name));
	if (rc != 0) {
		snprintf(thread_name, sizeof(thread_name), "unknown");
	}
	return fprintf(stream, "%s %-7s %6ld:%s %s\n",
		date, log_labels[level].english,
		syscall(__NR_gettid), thread_name, msg);
}

/**
 * @fn int log_fmt_debug(FILE *, int, struct timespec *, int,
 * const char *, const char *, int, char *)
 * @brief Output messages with timestamp, level, source code file, function,
 * and line number, and finally the message.
 *
 * @param stream the output stream to write to
 * @param sequence the sequence number of the message
 * @param ts the struct timmespec timestamp
 * @param level the log level to print
 * @param file the name of the file to print
 * @param function the name of the function to print
 * @param line the line number to print
 * @param msg the actual use message to print
 * @return the number of characters written.
 *
 * Example output:
```
2020-05-25 17:28:17.011 DEBUG   test-logger.c:main:110 eth0     AF_PACKET (17)
```
 */
int log_fmt_debug(FILE *stream, int sequence, struct timespec *ts, int level,
	const char *file, const char *function, int line, char *msg) {
	char date[TIMESTAMP_LEN];
	log_format_timestamp(ts, SP_MILLI, date, sizeof(date));
	// (-7 - let critical and emergency stick out, Use -9 to align them all)
	return fprintf(stream, "%s %-7s %s:%s:%d %s\n",
		date, log_labels[level].english, file, function, line, msg);
}

/**
 * @fn int log_fmt_debug_tid(FILE *, int, struct timespec *, int,
 * const char *, const char *, int, char *)
 * @brief Debug format with thread id added.
 *
 * @param stream the output stream to write to
 * @param sequence the sequence number of the message
 * @param ts the struct timmespec timestamp
 * @param level the log level to print
 * @param file the name of the file to print
 * @param function the name of the function to print
 * @param line the line number to print
 * @param msg the actual use message to print
 * @return the number of characters written.
 *
 * Example output:
```
2020-05-25 17:28:17.011 DEBUG   65623 test-logger.c:main:110 eth0     AF_PACKET (17)
```
 */
int log_fmt_debug_tid(FILE *stream, int sequence, struct timespec *ts, int level,
	const char *file, const char *function, int line, char *msg) {
	char date[TIMESTAMP_LEN];
	log_format_timestamp(ts, SP_MILLI, date, sizeof(date));
	return fprintf(stream, "%s %-7s %6ld %s:%s:%d %s\n",
		date, log_labels[level].english,
		 syscall(__NR_gettid), file, function, line, msg);	// or SYS_gettid
}

/**
 * @fn int log_fmt_debug_tname(FILE *, int, struct timespec *, int,
 * const char *, const char *, int, char *)
 * @brief Debug format with thread name added.
 *
 * @param stream the output stream to write to
 * @param sequence the sequence number of the message
 * @param ts the struct timmespec timestamp
 * @param level the log level to print
 * @param file the name of the file to print
 * @param function the name of the function to print
 * @param line the line number to print
 * @param msg the actual use message to print
 * @return the number of characters written.
 *
 * Example output:
```
2020-05-25 17:28:17.011 DEBUG   thread_2 test-logger.c:main:110 eth0     AF_PACKET (17)
```
 */
int log_fmt_debug_tname(FILE *stream, int sequence, struct timespec *ts, int level,
	const char *file, const char *function, int line, char *msg) {
	char date[TIMESTAMP_LEN];
	pthread_t thread = pthread_self();
	char thread_name[NAME_LEN];
	int rc;
	log_format_timestamp(ts, SP_MILLI, date, sizeof(date));
	rc = pthread_getname_np(thread, thread_name, sizeof(thread_name));
	if (rc != 0) {
		snprintf(thread_name, sizeof(thread_name), "unknown");
	}
	return fprintf(stream, "%s %-7s %s %s:%s:%d %s\n",
		date, log_labels[level].english,
		thread_name, file, function, line, msg);
}


/**
 * @fn int log_fmt_debug_tall(FILE *, int, struct timespec *, int,
 * const char *, const char *, int, char *)
 * @brief Debug format with thread id and name added.
 *
 * @param stream the output stream to write to
 * @param sequence the sequence number of the message
 * @param ts the struct timmespec timestamp
 * @param level the log level to print
 * @param file the name of the file to print
 * @param function the name of the function to print
 * @param line the line number to print
 * @param msg the actual use message to print
 * @return the number of characters written.
 *
 * Example output:
```
2020-05-25 17:28:17.011 DEBUG   65623:thread_2 test-logger.c:main:110 eth0     AF_PACKET (17)
```
 */
int log_fmt_debug_tall(FILE *stream, int sequence, struct timespec *ts, int level,
	const char *file, const char *function, int line, char *msg) {
	char date[TIMESTAMP_LEN];
	pthread_t thread = pthread_self();
	char thread_name[NAME_LEN];
	int rc;
	log_format_timestamp(ts, SP_MILLI, date, sizeof(date));
	rc = pthread_getname_np(thread, thread_name, sizeof(thread_name));
	if (rc != 0) {
		snprintf(thread_name, sizeof(thread_name), "unknown");
	}
	return fprintf(stream, "%s %-7s %6ld:%s %s:%s:%d %s\n",
		date, log_labels[level].english,
		syscall(__NR_gettid), thread_name, file, function, line, msg);
}


/**
 * @fn int log_fmt_elapsed_time(FILE *, int, struct timespec *ts, int,
 * const char *, const char *, int line, char *)
 * @brief Use elapsed time as the timestamp.
 *
 * @param stream the output stream to write to
 * @param sequence the sequence number of the message
 * @param ts the struct timmespec timestamp
 * @param level the log level to print
 * @param file the name of the file to print
 * @param function the name of the function to print
 * @param line the line number to print
 * @param msg the actual use message to print
 * @return the number of characters written.
 *
 * The most appropriate clock to use for this purpose is CLOCK_MONOTONIC_RAW.
 * If both output channels are being used, and the other is not using an
 * elapsed time format, other clocks may be used. But the timestamp may
 * occasionally "hiccup" because they do not guarantee monotonic behavior.
 * ```{.c}
 * log_select_clock(CLOCK_MONOTONIC_RAW);
 * LOG_CHANNEL *ch = log_open_channel_s(stderr, LL_INFO, log_fmt_elapsed_time);
 * ```
 *
 * Example output:
 *```
 *  0.000001665 INFO    formats.c:main:172 this message has elapsed time
 *  0.000010344 INFO    formats.c:main:173 this message has elapsed time
 *  0.000018740 INFO    formats.c:main:174 this message has elapsed time
 *```
 */
int log_fmt_elapsed_time(FILE *stream, int sequence, struct timespec *ts, int level,
    const char *file, const char *function, int line, char *msg) {
    char date[TIMESTAMP_LEN];
    log_format_timestamp(ts, LOG_FMT_DELTA, date, sizeof(date));
    return fprintf(stream, "%s %-7s %s:%s:%d %s\n",
        date, log_labels[level].english,
        file, function, line, msg);
}

