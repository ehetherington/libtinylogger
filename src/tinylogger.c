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
 * To configure with configure (autotools)
 * $ MAX_MSG_SIZE=xxxx ./configure
 * To configure with the quick-start setup edit the config.h in the quick-start
 * directory.
 * 0 = unlimited - uses asprintf
 */
#ifndef MAX_MSG_SIZE
#define MAX_MSG_SIZE BUFSIZ
#endif

/***************************************************/
/********************* private *********************/
/***************************************************/

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
 * The lock to support multithreaded access to the log_config data.
 */
static pthread_mutex_t log_lock = PTHREAD_MUTEX_INITIALIZER;

/**
 * @struct log_config
 * Parameters common to all (both) channels.
 */
static struct log_config {
	char *json_notes;	/**< notes to be used in future json format logs */
	clockid_t clock_id;	/**< The clock used for timestamps */
	struct timespec ts;	/**< The start time used for delta timestamp formats */
} log_config = {
	.json_notes = NULL,
	.clock_id = CLOCK_REALTIME,
	.ts = {0}
};

/**
 * Threads accessing log_channels[]
 *
 * Any thread calling the configuration functions. They modify it.
 *
 * Any thread calling the log_msg() function, They read the config.
 *
 * The logrotate thread. It reads the config.
 */
static struct _logChannel log_channels[] = {
	{LL_OFF,	NULL,	NULL, false, NULL, NULL, 0, NULL, NULL},
	{LL_OFF,	NULL,	NULL, false, NULL, NULL, 0, NULL, NULL},
};
#define LOG_CH_COUNT (sizeof(log_channels) / sizeof(log_channels[0]))

/**
 * Parameters used to support logrotate
 */
static struct rotate_config {
	int			signal;
	bool		thread_running;
	pthread_t	log_thread;
} rotate_config = {
	.signal = SIGUSR1,
	.thread_running = false,
	.log_thread = 0
};

/**
 * @fn LOG_CHANNEL *get_channel(void)
 * @brief get an available channel
 * @return a LOG_CHANNEL *, or NULL if none available
 */
static LOG_CHANNEL *get_channel(void) {
	LOG_CHANNEL *channel = (LOG_CHANNEL *) log_channels;
	for (size_t n = 0; n < LOG_CH_COUNT; n++, channel++) {
		if ((channel->pathname == NULL) &&
			(channel->stream == NULL)) {
			return channel;
		}
	}
	return NULL;
}

/**
 * @fn bool is_channel(LOG_CHANNEL *channel)
 * @brief verify that the given channel is valid
 * @param channel the channel to verify
 * @return true if the channel exists
 */
static inline bool is_channel(LOG_CHANNEL *channel) {
	// verify that it is actually a channel
	LOG_CHANNEL  *log_ch = (LOG_CHANNEL *) log_channels;
	for (size_t n = 0; n < LOG_CH_COUNT; n++, log_ch++) {
		if (channel == log_ch) {
			return true;
		}
	}

	return false;
}

/**
 * @fn bool is_open_channel(LOG_CHANNEL *channel)
 * @brief verify that the given channel is valid and open (in use)
 * @param channel the channel to verify
 * @return true if the channel exists
 */
static inline bool is_open_channel(LOG_CHANNEL *channel) {
	if (!is_channel(channel)) return false;
	return channel->stream != NULL ? true : false;
}

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
 * from <sys/time.h>
 * used timersub macro, changed timeval to timespec
 * kept the order of operands the same, that is a - b = result
#define timespec_diff_macro(a, b, result)             \
  do {                                                \
    (result)->tv_sec = (a)->tv_sec - (b)->tv_sec;     \
    (result)->tv_nsec = (a)->tv_nsec - (b)->tv_nsec;  \
    if ((result)->tv_nsec < 0) {                      \
      --(result)->tv_sec;                             \
      (result)->tv_nsec += 1000000000;                \
    }                                                 \
  } while (0)
 */

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
 * @fn void log_report_error(char const * const format, ...)
 * @brief log internal errors to stderr
 */
static void log_report_error(char const * const format, ...)
	__attribute__ ((format (printf, 1, 2)));
static void log_report_error(char const * const format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
}

/**
 * @fn void log_do_head(LOG_CHANNEL  *channel)
 * @brief Write the head for XML and Json output.
 * @param channel the channel to handle
 */
static void log_do_head(LOG_CHANNEL  *channel) {
	if (channel->formatter == log_fmt_xml) {
		log_do_xml_head(channel->stream);
	} else if (channel->formatter == log_fmt_json) {
		log_do_json_head(channel->stream, log_config.json_notes);
	}
}

/**
 * @fn void log_do_tail(LOG_CHANNEL  *channel)
 * @brief Write the tail for XML and Json output.
 * @param channel the channel to handle
 */
static void log_do_tail(LOG_CHANNEL  *channel) {
	if (channel->formatter == log_fmt_xml) {
		log_do_xml_tail(channel->stream);
	} else if (channel->formatter == log_fmt_json) {
		log_do_json_tail(channel->stream);
	}
}

/**
 * @fn bool _reopen_channel(LOG_CHANNEL *channel)
 * @brief used by log_sighandler() and log_reopen_channel().
 */
static bool _reopen_channel(LOG_CHANNEL *channel) {
	char buf[BUFSIZ];
	char *err_msg;

	// verify that it is actually an open channel
    if (!is_open_channel(channel)) return false;
	
	// for Json and XML
	log_do_tail(channel);

	// flush the output
	fflush(channel->stream);

	// if the channel is file based, close an reopen the channel
	if (channel->pathname != NULL) {
		fclose(channel->stream);

		// open the file in append mode
		FILE *file = fopen(channel->pathname, "a");

		// check for failure
		if (file == NULL) {
			err_msg = strerror_r(errno, buf, sizeof(buf));
			log_report_error("can't reopen file %s:%s\n", channel->pathname, err_msg);
			free(channel->pathname);
			bzero(channel, sizeof(*channel));
			return false;
		}

		// set line buffered output, if requested
		if (channel->line_buffered && setvbuf(file, NULL, _IOLBF, BUFSIZ)) {
			err_msg = strerror_r(errno, buf, sizeof(buf));
			log_report_error("can't set line buffering on %s: %s",
				channel->pathname, err_msg);
			return false;
		}
	}

	// reset the sequence number
	channel->sequence = 0;

	// for Json and XML
	log_do_head(channel);

	return true;
}

/**
 * @fn static void log_sighandler(void *config)
 * @brief The log rotate signal handler.
 * On catching the signal, log file(s) are flushed, closed, and re-opened.
 * Moving an output file to a new place, and sending the log rotate signal
 * saves the current state of the log file, and opens a new file to continue
 * logging to.
 *
 * The thread may be stopped by passing a value of 0 for the signal.
 * @param signal The number of the signal to use.
 * @return 0 on success
 */
static void *log_sighandler(void *config) {
	pthread_t me = pthread_self();
	siginfo_t	info;
	sigset_t	sigs;
	int			signum;
	int			rc;
	struct rotate_config *tc = config;
	LOG_CHANNEL *ch = (LOG_CHANNEL *) log_channels;

	// TODO: don't be so drastic on failure
	rc = pthread_setname_np(me, "log_sighandler");
	if (rc != 0) {
		perror("setting logrotate thread name");
		exit(EXIT_FAILURE);
	}

	sigemptyset(&sigs);
	sigaddset(&sigs, tc->signal);

	while (1) {
		signum = sigwaitinfo(&sigs, &info);
		if (signum != rotate_config.signal)
			continue;

		// Sent by this process itself, exit requested
		if (info.si_pid == getpid())
			break;

		pthread_mutex_lock(&log_lock);
		for (size_t n = 0; n < LOG_CH_COUNT; n++, ch++) {
			_reopen_channel(ch);
		}
		pthread_mutex_unlock(&log_lock);
	}

	return NULL;
}

/**************************************************/
/********************* public *********************/
/**************************************************/

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
 * @fn log_select_clock(clockid_t clock_id)
 * @brief Select the linux clock to use for timestamps.
 *
 * CLOCK_REALTIME, CLOCK_MONOTONIC, CLOCK_MONOTONIC_RAW, CLOCK_REALTIME_COARSE,
 * and CLOCK_BOOTTIME are available, depending on kernel vesion.
 *
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
		case CLOCK_BOOTTIME:
		#endif
		#endif
		#endif
		log_config.clock_id = clock_id;
		// get a timestamp for the elapsed time format
		clock_gettime(log_config.clock_id, &log_config.ts);
	}
	return log_config.clock_id == clock_id;
}

/**
 * @fn void log_format_delta(struct timespec *ts, LOG_TS_FORMAT precision,
 *  char *buf, int len)
 * @brief Format an elapsed time timestamp.
 *
 * The starting timestamp is common to all open channels. It is set when the
 * first channel is opened, or when when a clock is selected with
 * `log_select_clock()`.
 *
 * The fractional seconds appended is specified by the LOG_TS_FORMAT precision.
 * - SP_NONE  no fraction is appended
 * - SP_MILLI .nnn is appended
 * - SP_MICRO .nnnnnn is appended
 * - SP_NANO  .nnnnnnnnn is appended
 *
 * Other bits in LOG_TS_FORMAT are ignored.
 *
 * @param ts the previously obtained struct timespec timestamp.
 * @param precision the precision of the fraction of second to display
 * @param buf the buffer to format the timestamp to
 * @param len the length of that buffer. Must be >= 30
 */
void log_format_delta(struct timespec *ts, LOG_TS_FORMAT precision, char *buf, int len) {
	struct timespec delta;
	char seconds_buf[26];

    if ((buf == NULL) || (len < TIMESTAMP_LEN)) {
        fprintf(stderr,
            "log_format_delta: internal error - provide %d char buf\n",
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
 * @param line the line number of the line of code (debug format)
 * @param format the printf format string (required)
 * @param ... the arguments to the format string
 *
 * @return 0 on success, -1 if the format was NULL, -2 if clock_gettime() error
 */
int log_msg(int const level,
	char const * const file, char const * const function, int const line,
	char const * const format, ...) {
	va_list	args;
	struct timespec ts;
	int status = 0;	// assume success
#if MAX_MSG_SIZE == 0
	char *msg;
#else
	char	msg[MAX_MSG_SIZE];		// user message
#endif

	// make sure we have something to log
	if (!format)	return -1;	// error - require format string
	if (!*format)	return 0;	// no msg - silly format string, but let it go

	// lock the actual write(s) that may be through streams that may be
	// managed by SIG_ROTATE
	pthread_mutex_lock(&log_lock);

	// get a timestamp
	if (clock_gettime(log_config.clock_id, &ts) == -1) {
		// drop the message, but return error status
		status = -2;
		goto unlock;
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
		// use a dummy sequence number of 0 - discarded by log_fmt_standard
		log_fmt_standard(stderr, 0,
				&ts, level, file, function, line, msg);
		goto unlock;
	}

	//
	// send the message to any active channels with the proper log level
	//
	LOG_CHANNEL *channel = (LOG_CHANNEL *) log_channels;
	for (size_t n = 0; n < LOG_CH_COUNT; n++, channel++) {
		if ((channel->stream != NULL) && (level <= channel->level)) {
			// pre-increment sequence - it is cleared to 0 on open
			channel->formatter(channel->stream, ++channel->sequence,
				&ts, level, file, function, line, msg);
		}
	}

	// unlock
unlock:
	pthread_mutex_unlock(&log_lock);
#if MAX_MSG_SIZE == 0
	free(msg);
#endif

	return status;	// 0 on success
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
 * @param line the line number of the calling statement
 * @param format the printf format specifier
 *
 * @return 0 on success, -1 otherwise
 */
int log_mem(int const level, void const * const buf, int const len,
	char const * const file, char const * const function, int const line,
	char const * const format, ...) {
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
		log_msg(level, file, function, line, "%s\n%s", msg, hex);

#if MAX_MSG_SIZE == 0
	free(msg);
#endif

	free(hex);

	return status;
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
	for (size_t n = 0; n < LOG_CH_COUNT; n++, log_ch++) {
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
	LOG_CHANNEL *channel = NULL;	// assume failure
	
	// check that we have a stream
	if (stream == NULL) return NULL;

	// give a default formatter in case none was specified
	if (formatter == NULL) formatter = log_fmt_standard;

	// make sure the level is a valid one
	level = log_constrain_level(level);

	// LOCK global resources
	pthread_mutex_lock(&log_lock);

	// find an available channel
	channel = get_channel();
	if (channel == NULL) goto unlock;

	// clear all params
	bzero(channel, sizeof(*channel));

	// record the stream, level and formatter
	channel->stream = stream;
	channel->level = level;
	channel->formatter = formatter;

	// for Json and XML
	log_do_head(channel);

	// initialize the start time for delta time formats
	if (!configured) {
		log_select_clock(log_config.clock_id);
		
		// user has set up at least one channel
		configured = true;
	}

unlock:
	// UNLOCK global resources
	pthread_mutex_unlock(&log_lock);

	return channel;
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
	LOG_CHANNEL *channel = NULL;
	
	// check that we have a pathname
	if (pathname == NULL) return NULL;

	// give a default formatter in case none was specified
	if (formatter == NULL) formatter = log_fmt_standard;

	// make sure the level is a valid one
	level = log_constrain_level(level);

	// LOCK global resources
	pthread_mutex_lock(&log_lock);

	// find an available channel
	channel = get_channel();
	if (channel == NULL) goto unlock;

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

	// for Json and XML
	log_do_head(channel);

	// initialize the start time for delta time formats
	if (!configured) {
		log_select_clock(log_config.clock_id);
		
		// user has set up at least one channel
		configured = true;
	}

unlock:
	// UNLOCK global resources
	pthread_mutex_unlock(&log_lock);

	return channel;
}

/**
 * @fn void log_set_json_notes(char *notes)
 * @brief Set the notes to use in future logs opened using the json formatter.
 *
 * @note ENABLE_JSON_HEADER must be set for the notes field to be present. See
 * guide/json_formatter.md
 *
 * @param notes the notes to use
 * 
 */
void log_set_json_notes(char *notes) {
	static char _notes[BUFSIZ] = {0};

	// LOCK global resources
	pthread_mutex_lock(&log_lock);

	if (notes == NULL) {
		log_config.json_notes = NULL;
	} else {
		// stash a "private" copy
		strncpy(_notes, notes, sizeof(_notes) - 1);
		log_config.json_notes = _notes;
	}

	// UNLOCK global resources
	pthread_mutex_unlock(&log_lock);
}

/**
 * @fn int log_change_params(LOG_CHANNEL  *channel, LOG_LEVEL level, 
 * log_formatter_t formatter)
 * @brief Change the log level and/or formatter of a channel while it is open.
 *
 * The parameters will be changed in sync.
 *
 * @deprecated Changing formats on an open channel will not be supported in
 * the future. See #log_set_level(LOG_CHANNEL *ch, LOG_LEVEL level)
 *
 * @param channel The channel to modify parameters.
 * @param level The new log level.
 * @param formatter The new formatter.
 * @return 0 on success, -1 if the channel is not currently open or the
 *   formmater is NULL
 */
int log_change_params(LOG_CHANNEL  *channel, LOG_LEVEL level, log_formatter_t formatter) {
	int status = -1;	// assume failure

	// LOCK global resources
	pthread_mutex_lock(&log_lock);

	// make sure we have a formatter
	if (formatter == NULL) goto unlock;

	// verify that it is actually a channel
    if (!is_channel(channel)) {
        goto unlock;
    }

	// change the params
	channel->level = log_constrain_level(level);
	channel->formatter = formatter;

	// success
	status = 0;

unlock:
	// UNLOCK global resources
	pthread_mutex_unlock(&log_lock);

	return status;
}

/**
 * @fn int log_set_level(LOG_CHANNEL  *channel, LOG_LEVEL level)
 * @brief Change the log level of a channel while it is open.
 *
 * @param channel The channel to modify parameters.
 * @param level The new log level.
 * @return 0 on success, -1 if the channel is not valid
 */
int log_set_level(LOG_CHANNEL  *channel, LOG_LEVEL level) {
	int status = -1;	// assume failure

	// UNLOCK global resources
	pthread_mutex_lock(&log_lock);

	if (!is_channel(channel)) {
		goto unlock;
	}

	// change the level
	channel->level = log_constrain_level(level);

	// success
	status = 0;

unlock:
	// UNLOCK global resources
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
 * In both cases, if the formatter is log_format_json or log_format_xml, the
 * proper closing and opening sequences are produced.
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
 *```
 *    LOG_CHANNEL *ch1 = log_open_channel_f(LIVE_LOG_NAME, ...);
 *    log_info(...);
 *    log_info("this is the last message in the "archived file");
 *    rename(LIVE_LOG_NAME, ARCHIVE_LOG_NAME);
 *    // re-init the channel to the original pathname
 *    log_reopen_channel(ch1);
 *    log_info("this is the first message in the new live log file");
 *```
 *
 * @param channel The channel to re-open.
 * @return true on success
 */
int log_reopen_channel(LOG_CHANNEL *channel) {

	// LOCK global resources
	pthread_mutex_lock(&log_lock);

	int status = _reopen_channel(channel);

	// UNLOCK global resources
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

	// LOCK global resources
	pthread_mutex_lock(&log_lock);

	// verify that it is actually a channel
    if (!is_channel(channel)) {
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
	// UNLOCK global resources
	pthread_mutex_unlock(&log_lock);

	return status;
}

/**
 * @fn void log_enable_logrotate(int signal)
 * @brief Start a thread to catch a signal for log rotation.
 * On catching the signal, log file(s) are flushed, closed, and re-opened.
 * Moving an output file to a new place, and sending the log rotate signal
 * saves the current state of the log file, and opens a new file to continue
 * logging to.
 *
 * The thread may be stopped by passing a value of 0 for the signal.
 * @param signal The number of the signal to use.
 * @return 0 on success
 */
int log_enable_logrotate(int signal) {
	sigset_t		sigs;
	pthread_attr_t	attrs;
	int				retval;

	if ((signal < 0) || (signal > SIGRTMAX)) {
		return -1;
	}

	// TODO: check correct way to see if a thread actually exists and is
	// still running. Set thread to 0 after successfull wait?
	if (signal == 0) {
		if (rotate_config.thread_running) {
			pthread_kill(rotate_config.log_thread, rotate_config.signal);
			pthread_join(rotate_config.log_thread, NULL);

			rotate_config.thread_running = false;
		}
		return 0;
	}

	// create a set with only the signal of interest
	sigemptyset(&sigs);
	sigaddset(&sigs, signal);
	pthread_sigmask(SIG_BLOCK, &sigs, NULL);

	// Create a thread to handle the rotate signal
	pthread_attr_init(&attrs);
	pthread_attr_setstacksize(&attrs, 65536);
	retval = pthread_create((pthread_t *) &rotate_config.log_thread,
		&attrs, log_sighandler, (void *) &rotate_config);
	pthread_attr_destroy(&attrs);
	if (retval) return retval;

	rotate_config.thread_running = true;
	rotate_config.signal = signal;

	return 0;
}
