/*
 * (C) 2020 Edward Hetherington
 * This code is licensed under MIT license (see LICENSE in top dir for details)
 */

/**	@file		tinylogger.c
 *	@brief		The tinylogger source file.
 *	@details	A small logging facility for small linux projects.
 *	@author		Edward Hetherington
 */


#include "config.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#define _GNU_SOURCE

// from kernel/sched.h
#define TASK_COMM_LEN 16
#define NAME_LEN TASK_COMM_LEN
#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <errno.h>
#include <linux/version.h>

#include "tinylogger.h"
#include "private.h"

/**
 * 0 = unlimited - uses asprintf
 */
#ifndef MAX_MSG_SIZE
#define MAX_MSG_SIZE BUFSIZ
#endif

/*
 * configured is false at startup, and set true when the user configures a
 * channel. It is never cleared. It is used to route messages that come by
 * before any configuration has taken place to the stderr.
 */
static bool configured = false;

/*
 * Messages that come by before configuration below LL_INFO are blocked.
 */
static LOG_LEVEL pre_init_level = LL_INFO;

/**
 * The lock to support multithreaded access to the log_config data. Must be
 * public for tinylogger.c and logrotate.c to access it.
 */
pthread_mutex_t log_lock = PTHREAD_MUTEX_INITIALIZER;

/**
 * @struct log_config
 * Parameters common to both channels.
 */
static struct log_config {
	bool wrap_records;	/**< Include head/tail in XML/Json formats */
	clockid_t clock_id;	/**< The clock used for timestamps */
	struct timespec ts;	/**< The start time used for delta timestamp formats */
} log_config = {
	.wrap_records = true,
	.clock_id = CLOCK_REALTIME,
	.ts = {0}
};

/**
 * @fn void log_set_pre_init_level(LOG_LEVEL log_level)
 * @brief Before any channel is configured, log messages are passed to the
 *        stderr.
 *
 * The minimum pre-init log level may be set using
 * log_set_pre_int_level(LOG_LEVEL). This is useful at startup time to debug
 * command line parsing before the final logging configuration is known.
 *
 * @param log_level the minimum level to output.
 */
void log_set_pre_init_level(LOG_LEVEL log_level) {
	pre_init_level = log_level;
}

/**
 * This may not need to be volatile, but better err on the
 * safe side.
 * Potentially different threads access it.
 *
 * Any thread calling the configuration functions. They modify it.
 *
 * Any thread calling the log_msg() function, They read the config.
 *
 * The logrotate thread. It reads the config.
 */
volatile struct _logChannel log_channels[LOG_CH_COUNT] = {
	{LL_OFF,	NULL,	NULL, false, NULL},
	{LL_OFF,	NULL,	NULL, false, NULL},
};

/**
 * @fn LOG_LEVEL log_constrain_level(LOG_LEVEL level)
 * @brief Constrain level to the valid range.
 * The user may give an invalid level to one of the channel init functions
 * or log_msg(), Adjust that level to a valid one. This should probably be
 * removed, and the functions involved should return an error.
 * @param level the level in question
 * @return the level constrained to the valid range.
 */
static LOG_LEVEL log_constrain_level(LOG_LEVEL level) {
    if (level >= LL_N_VALUES) return LL_N_VALUES - 1;
    if (level < 0) return 0;
    return level;
}

/**
 * @fn log_select_clock(clockid_t clock_id)
 * @brief Select the linux clock to use for timestamps.
 *
 * CLOCK_REALTIME, CLOCK_MONOTONIC, CLOCK_MONOTONIC_RAW, CLOCK_REALTIME_COARSE,
 * and CLOCK_BOOTTIME are available, depending on kernel vesion.
 *
 * The start timestamp used by the elapsed time formatter is recorded using the
 * selected clock. Both the timestamp and clock selection are common to both
 * channels in order to make them relative to the same "t0".
 *
 * @return true if the requested clock_id is supported, false otherwise.
 */
bool log_select_clock(clockid_t clock_id) {
	switch (clock_id) {
		case CLOCK_REALTIME:
		case CLOCK_MONOTONIC:
		#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,28)
		case CLOCK_MONOTONIC_RAW:
		#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,32)
		case CLOCK_REALTIME_COARSE:
		case CLOCK_MONOTONIC_COARSE:
		#if LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,39)
		case CLOCK_BOOTTIME: {
			log_config.clock_id = clock_id;
			// get a timestamp
			if (clock_gettime(log_config.clock_id, &log_config.ts) == 0) {
			}
		};
		#endif
		#endif
		#endif
	}
	return log_config.clock_id == clock_id;
}

/**
 * @fn void log_wrap_records(bool yes_no)
 * @brief Emit opening and closing sequences for Json and XML logging.
 * Both the Json and XML output can enclose the stream of message records
 * in a "log" definition. Output can be constructed as a single log entity
 * or object, or as a sequence of individual record entities or objects.
 * This setting affects all channels in use,
 * @param yes_no set to true to include the opening and closing sequences
 * to construct a single log object.
 */
void log_wrap_records(bool yes_no) {
	log_config.wrap_records = yes_no;
}

/**
 * from <sys/time.h>
 * used timersub macro, changed timeval to timespec
 * kept the order of operands the same, that is a - b = result
 */
#define timespec_diff_macro(a, b, result)             \
  do {                                                \
    (result)->tv_sec = (a)->tv_sec - (b)->tv_sec;     \
    (result)->tv_nsec = (a)->tv_nsec - (b)->tv_nsec;  \
    if ((result)->tv_nsec < 0) {                      \
      --(result)->tv_sec;                             \
      (result)->tv_nsec += 1000000000;                \
    }                                                 \
  } while (0)

/**
 * @fn timespec_diff(struct timespec *, struct timespec *, struct timespec *)
 * @brief Compute the diff of two timespecs, that is a - b = result.
 * @param a the minuend
 * @param b the subtrahend
 * @param result a - b
 */
static inline void timespec_diff(struct timespec *a, struct timespec *b,
	struct timespec *result) {
	result->tv_sec  = a->tv_sec  - b->tv_sec;
	result->tv_nsec = a->tv_nsec - b->tv_nsec;
	if (result->tv_nsec < 0) {
		if (result->tv_sec >= 0) --result->tv_sec;
		result->tv_nsec += 1000000000L;
	}
}

/**
 * @fn void log_format_delta(struct timespec *ts, SEC_PRECISION precision,
 *  char *buf, int len)
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
void log_format_delta(struct timespec *ts, SEC_PRECISION precision, char *buf, int len) {
	struct timespec delta;
	char seconds_buf[26];

    if ((buf == NULL) || (len < TIMESTAMP_LEN)) {
        fprintf(stderr,
            "log_format_timestamp: internal error - provide %d char buf\n",
                TIMESTAMP_LEN);
        return;
    }

	timespec_diff(ts, &log_config.ts, &delta);

	if (precision & LOG_FMT_HMS) {
		long hours;
		int minutes, seconds;
		long off_remainder;

		hours = delta.tv_sec / (60 *60);
		off_remainder = delta.tv_sec % (60 * 60);
		minutes = off_remainder / 60;
		off_remainder = off_remainder % 60;
		seconds = off_remainder;

		snprintf(seconds_buf, sizeof(seconds_buf), "%ld:%02d:%02d",
			hours, minutes, seconds);
	} else {
		snprintf(seconds_buf, sizeof(seconds_buf), "% 3ld", delta.tv_sec);
	}

	snprintf(buf, len, "%s.%09ld", seconds_buf, delta.tv_nsec);
}

/**
 * @fn int log_msg(int,
 *     const char *, const char *, const int,
 *     const char *, ...)
 *
 * @brief Log a message.
 *
 * This is the actual logging function. The convenience log_xxx() macros
 * should normally be used. See tinylogger.h for their definitions.
 *
 * @param level the log level desired
 * @param file the filename of the line of code (debug format)
 * @param function the function of the line of code (debug format)
 * @param line_number the line number of the line of code (debug format)
 * @param format the printf format string (required)
 * @param ... the arguments to the format string
 *
 * @return 0 on success, -1 if the format was NULL
 */
int log_msg(int level,
	const char *file, const char *function, const int line_number,
	const char *format, ...) {
	va_list	args;
#if MAX_MSG_SIZE == 0
	char *msg;
#else
	char	msg[MAX_MSG_SIZE];		// user message
#endif
	struct timespec ts;

	// make sure we have something to log
	if (!format)	return -1;	// error - require format string
	if (!*format)	return 0;	// silly format string, but let it go

	// lock the actual write(s) that may be through streams that may be
	// managed by SIG_ROTATE
	pthread_mutex_lock(&log_lock);

	// get a timestamp
	if (clock_gettime(log_config.clock_id, &ts) == 0) {
		// TODO: report error
	}

	/* format the user message contents */
	va_start(args, format);
#if MAX_MSG_SIZE == 0
	vasprintf(&msg, format, args);
#else
	vsnprintf(msg, sizeof(msg), format, args);
#endif
	va_end(args);

	// if the log_channels have not been configured,
	// send the output to the stderr
	if (!configured) {
		if (level > pre_init_level) goto unlock; 	// discard
		// use sequence number of 0 - discarded by log_fmt_standard
		log_fmt_standard(stderr, 0,
				&ts, level, file, function, line_number, msg);
		goto unlock;
	}

	//
	// send the message to any active channels with the proper log level
	//
	LOG_CHANNEL *channel = (LOG_CHANNEL *) log_channels;
	for (int n = 0; n < LOG_CH_COUNT; n++, channel++) {
		if ((channel->stream != NULL) && (level <= channel->level)) {
			// pre-increment sequence - it is cleared to 0 on open
			channel->formatter(channel->stream, ++channel->sequence,
				&ts, level, file, function, line_number, msg);
		}
	}

	// unlock
unlock:
	pthread_mutex_unlock(&log_lock);
#if MAX_MSG_SIZE == 0
	free(msg);
#endif

	return 0;	// success
}


/**
 * @fn int log_mem(int const, void const * const, int const,
 *     char const * const, char const *, int const,
 *     char const * const, ...)
 *
 * @brief Format a region of memory to hex, and log it preceeded with a user
 *        message.
 *
 * This function is intended to be called with the log_memory() macro. See
 * tinylogger.h for its definitions.
 *
 * @param level the log level
 * @param buf address of the memory region
 * @param len length of the memory region
 * @param file the name of the file of the calling statement
 * @param function the name of the function of the calling statement
 * @param line_number the line number of the calling statement
 * @param format the printf format specifier
 *
 * @return 0 on success, -1 otherwise
 */
int log_mem(int const level, void const * const buf, int const len,
	char const *file, char const *function, int const line_number,
	char const *format, ...) {
	va_list	args;
#if MAX_MSG_SIZE == 0
	char *msg;
#else
	char	msg[MAX_MSG_SIZE];		// user message
#endif

	/* format the memory region */
	char *hex = log_hexformat(buf, len);
	if (hex == NULL) return -1;

	/* format the user message contents */
	va_start(args, format);
#if MAX_MSG_SIZE == 0
	vasprintf(&msg, format, args);
#else
	vsnprintf(msg, sizeof(msg), format, args);
#endif
	va_end(args);

	// separate the message from the dump with a newline
	int status =
		log_msg(level, file, function, line_number, "%s\n%s", msg, hex);

#if MAX_MSG_SIZE == 0
	free(msg);
#endif

	free(hex);

	return status;
}

/**
 * @fn void log_do_head(LOG_CHANNEL  *channel)
 * @brief Write the head for XML and Json output.
 * @param channel the channel to handle
 */
void log_do_head(LOG_CHANNEL  *channel) {
	if (!log_config.wrap_records) return;
	if (channel->formatter == log_fmt_xml) {
		log_do_xml_head(channel->stream);
	} else if (channel->formatter == log_fmt_json) {
		log_do_json_head(channel->stream);
	}
}

/**
 * @fn void log_do_tail(LOG_CHANNEL  *channel)
 * @brief Write the tail for XML and Json output.
 * @param channel the channel to handle
 */
void log_do_tail(LOG_CHANNEL  *channel) {
	if (!log_config.wrap_records) return;
	if (channel->formatter == log_fmt_xml) {
		log_do_xml_tail(channel->stream);
	} else if (channel->formatter == log_fmt_json) {
		log_do_json_tail(channel->stream);
	}
}

/**
 * @fn void log_done(void)
 * @brief Close any open channels and stop the logrotate thread if it was
 * running, in that sequence.
 *
 * It calls log_close_ch() for each channel, then log_enable_logrotate(0).
 *
 * The software does not return to the pre-init state where messages are passed
 * to the stderr. All output is stopped. Channels must be opened again to resume
 * output. The fact that logging had been configured is remembered.
 */
void log_done(void) {
	// stop the logrotate support
	log_enable_logrotate(0);

	// disable all log_channels
	// if a channel was file based, flush and close it
	LOG_CHANNEL  *log_ch = (LOG_CHANNEL *) log_channels;
	for (int n = 0; n < LOG_CH_COUNT; n++, log_ch++) {
		log_close_channel(log_ch);
	}
}

/**
 * @fn LOG_CHANNEL *log_open_channel_s(FILE *stream, LOG_LEVEL level,
 * log_formatter_t formatter)
 * @brief Open a channel for stream output.
 *
 * Sets up the minimum log level and message format for a stream channel.
 * (For example, stderr).
 *
 * @param stream The output stream to use.
 * @param level The minimum log level to output.
 * @param formatter The message formatter to use.
 * @return NULL on error, else the LOG_CHANNEL
 */
LOG_CHANNEL *log_open_channel_s(FILE *stream, LOG_LEVEL level,
	log_formatter_t formatter) {
	LOG_CHANNEL *results = NULL;	// assume failure
	
	// check that we have a stream
	if (stream == NULL) return results;

	// give a default formatter in case none was specified
	if (formatter == NULL) formatter = log_fmt_standard;

	// make sure the level is a valid one
	level = log_constrain_level(level);

	// log_config is a global resource, LOCK it
	pthread_mutex_lock(&log_lock);

	// find an available channel
	LOG_CHANNEL *channel = (LOG_CHANNEL *) log_channels;
	int n;
	for (n = 0; n < LOG_CH_COUNT; n++, channel++) {
		if ((channel->pathname == NULL) &&
			(channel->stream == NULL)) {
			break;
		}
	}
	if (n == LOG_CH_COUNT) {
		goto unlock;
	}

	// clear unused params
	bzero(channel, sizeof(*channel));

	// record the stream, level and formatter
	channel->stream = stream;
	channel->level = level;
	channel->formatter = formatter;
	channel->wrap_records = log_config.wrap_records;	// latch state

	// for Json and XML
	log_do_head(channel);

	// initialize the start time for delta time formats
	if (!configured) {
		log_select_clock(log_config.clock_id);
		
		// user has set up at least one channel
		configured = true;
	}

	// success
	results = channel;

unlock:
	// log_config is a global resource, UNLOCK it
	pthread_mutex_unlock(&log_lock);

	return results;
}

/**
 * @fn LOG_CHANNEL *log_open_channel_f(char *pathname, LOG_LEVEL level,
 * log_formatter_t formatter, bool line_buffered)
 * @brief Open a channel for output to a file.
 *
 * Sets up the minimum log level and message format for a file channel.
 * Line buffering is useful for a debug log channel because the lines are
 * written immediately, instead of waiting for a 4k buffer to fill up. Tail
 * -f works well with this.
 *
 * @param pathname The pathname of the file to manage.
 * @param level The minimum log level to output.
 * @param formatter The message formatter to use.
 * @param line_buffered Use line buffering vs default
 * @return NULL on error, else the LOG_CHANNEL
 */
LOG_CHANNEL *log_open_channel_f(char *pathname, LOG_LEVEL level,
	log_formatter_t formatter, bool line_buffered) {
	LOG_CHANNEL *results = NULL;	// assume failure
	
	// check that we have a pathname
	if (pathname == NULL) return results;

	// give a default formatter in case none was specified
	if (formatter == NULL) formatter = log_fmt_standard;

	// make sure the level is a valid one
	level = log_constrain_level(level);

	// log_config is a global resource, LOCK it
	pthread_mutex_lock(&log_lock);

	// find an available channel
	LOG_CHANNEL *channel = (LOG_CHANNEL *) log_channels;
	int n;
	for (n = 0; n < LOG_CH_COUNT; n++, channel++) {
		if ((channel->pathname == NULL) &&
			(channel->stream == NULL)) {
			break;
		}
	}
	if (n == LOG_CH_COUNT) {
		goto unlock;
	}

	// clear unused params
	bzero(channel, sizeof(*channel));

	// open the file in append mode
	FILE *file = fopen(pathname, "a");

	if (file == NULL) goto unlock;

	// set line buffered output, if requested
	if (line_buffered && setvbuf(file, NULL, _IOLBF, BUFSIZ)) {
		char buf[128];
		char *err_msg;
		err_msg = strerror_r(errno, buf, sizeof(buf));
		log_err("can't set line buffering on %s: %s", pathname, err_msg);
		goto unlock;
	}

	// duplicate the pathname,
	// record the stream, level, formatter, and line_bufferd status
	channel->pathname = strdup(pathname);
	channel->line_buffered = line_buffered;
	channel->stream = file;
	channel->level = level;
	channel->formatter = formatter;
	channel->wrap_records = log_config.wrap_records;	// latch state

	// for Json and XML
	log_do_head(channel);

	// initialize the start time for delta time formats
	if (!configured) {
		log_select_clock(log_config.clock_id);
		
		// user has set up at least one channel
		configured = true;
	}

	// success
	results = channel;

unlock:
	// log_config is a global resource, UNLOCK it
	pthread_mutex_unlock(&log_lock);

	return results;
}

/**
 * @fn int log_change_params(LOG_CHANNEL  *channel, LOG_LEVEL level, 
 * log_formatter_t formatter)
 * @brief Change the log level and/or formatter of a channel while it is open.
 *
 * The parameters will be changed in sync.
 *
 * @param channel The channel to modify parameters.
 * @param level The new log level.
 * @param formatter The new formatter.
 * @return 0 on success, -1 if the channel is not currently open or the
 *   formmater is NULL
 */
int log_change_params(LOG_CHANNEL  *channel, LOG_LEVEL level, log_formatter_t formatter) {
	int status = -1;	// assume failure

	// log_config is a global resource, LOCK it
	pthread_mutex_lock(&log_lock);

	// make sure we have a formatter
	if (formatter == NULL) goto unlock;

	// verify that it is actually a channel
	LOG_CHANNEL  *log_ch = (LOG_CHANNEL *) log_channels;
	int n;
	for (n = 0; n < LOG_CH_COUNT; n++, log_ch++) {
		if (channel == log_ch) {
			break;
		}
	}
	if (n == LOG_CH_COUNT) {
		goto unlock;
	}

	// change the params
	channel->level = log_constrain_level(level);
	channel->formatter = formatter;

	// success
	status = 0;

unlock:
	// log_config is a global resource, UNLOCK it
	pthread_mutex_unlock(&log_lock);

	return status;
}

/**
 * @fn int log_reopen_channel(LOG_CHANNEL *channel)
 * @brief Re-open a channel to support *programatic* logrotate.
 *
 * If the channel is a file based channel, the file is flushed and closed.
 * The file is then opened again with the same log level, formatter, and
 * selected line buffering.
 *
 * If the channel is a stream based channel, it is just flushed.
 *
 * The procedure would be:
 *
 * - rename the current file to, say, currentfile.save
 * - call log_reopen_channel(). The next things happen:
 *   - logging is locked
 *   - the file is flushed
 *   - the file is closed
 *   - a new file with the original name is opened
 *   - logging is unlocked
 * - logging then resumes without any messages being lost
 *
 * ```
 *    LOG_CHANNEL *ch1 = log_open_channel_f(LIVE_LOG_NAME, ...);
 *    log_info(...);
 *    log_info("this is the last message in the "archived file");
 *    rename(LIVE_LOG_NAME, ARCHIVE_LOG_NAME);
 *    // re-init the channel to the original pathname
 *    log_reopen_channel(ch1);
 *    log_info("this is the first message in the new live log file");
 *
 * ```
 *
 * @param channel The channel to re-open.
 * @return 0 on success
 */
int log_reopen_channel(LOG_CHANNEL *channel) {
	char buf[128];
	char *err_msg;
	int status = -1;

	// log_config is a global resource, LOCK it
	pthread_mutex_lock(&log_lock);

	// verify that it is actually a channel
	LOG_CHANNEL *log_ch = (LOG_CHANNEL *) log_channels;
	int n;
	for (n = 0; n < LOG_CH_COUNT; n++, log_ch++) {
		if (channel == log_ch) {
			break;
		}
	}
	if (n == LOG_CH_COUNT) {
		goto unlock;
	}
	
	// make sure we have a file based channel
	if (channel->pathname == NULL) {
		status = 0;	// no harm done
		goto unlock;
	}

	// for Json and XML
	log_do_tail(channel);

	// flush and close the current file
	fflush(channel->stream);
	fclose(channel->stream);

	// open the file in append mode
	FILE *file = fopen(channel->pathname, "a");

	if (file == NULL) {
		free(channel->pathname);
		bzero(channel, sizeof(*channel));
		goto unlock;
	}

	channel->sequence = 0;

	// for Json and XML
	log_do_head(channel);

	// set line buffered output, if requested
	if (channel->line_buffered && setvbuf(file, NULL, _IOLBF, BUFSIZ)) {
		err_msg = strerror_r(errno, buf, sizeof(buf));
		log_err("can't set line buffering on %s: %s", channel->pathname, err_msg);
		goto unlock;
	}

	// success
	status = 0;

unlock:
	// log_config is a global resource, UNLOCK it
	pthread_mutex_unlock(&log_lock);

	return status;
}

/**
 * @fn int log_close_channel(LOG_CHANNEL *channel)
 * @brief Flush and close the channel, and mark it not in use.
 *
 * @param channel The channel to close
 * @return 0 on success. If channel is not an actual channel, -1 is returned.
 * If the channel wasn't actually open, -2 is returned.
 */
int log_close_channel(LOG_CHANNEL *channel) {
	int status = 0;	// assume success

	// log_config is a global resource, lock it
	pthread_mutex_lock(&log_lock);

	// verify that it is actually a channel
	LOG_CHANNEL *log_ch = (LOG_CHANNEL *) log_channels;
	int n;
	for (n = 0; n < LOG_CH_COUNT; n++, log_ch++) {
		if (channel == log_ch) {
			break;
		}
	}
	if (n == LOG_CH_COUNT) {
		status = -1;
		goto unlock;
	}

	// see if it was actually open
	if (channel->stream == NULL) {
		status = -2;
		goto unlock;
	}

	// for Json and XML
	log_do_tail(channel);

	// If we are closing an existing file based config, that means we need to
	// flush and close the file.
	if (channel->pathname != NULL) {
		fflush(channel->stream);
		fclose(channel->stream);
		free(channel->pathname);	// remember to free the stdrup()'ed pathname
	}

	// clear the target channel to indicate it is no longer active
	bzero(channel, sizeof(*channel));

unlock:
	// log_config is a global resource, unlock it
	pthread_mutex_unlock(&log_lock);

	return status;
}
