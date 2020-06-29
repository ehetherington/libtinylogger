/*
 * (C) 2020 Edward Hetherington
 * This code is licensed under MIT license (see LICENSE in top dir for details)
 */

/**	@file		tinylogger.c
 *	@brief		The tinylogger source file.
 *	@details	A tiny logger facility for small linux projects.
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

#include "tinylogger.h"
#include "private.h"

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
 * public for logrotate.c
 */
pthread_mutex_t log_lock = PTHREAD_MUTEX_INITIALIZER;

/**
 * @fn void log_set_pre_init_level(LOG_LEVEL log_level)
 * @brief Before log_init() is called, log messages are passed to the stderr.
 *
 * The * minimum log level may be set using log_set_pre_int_level(LOG_LEVEL).
 * This is useful at startup time to debug command line parsing before the
 * final logging configuration is known.
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
 * @fn int log_msg(int, const char *, const char *, const int,
 * const char *, ...)
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
 * @return 0 on success, -1 if the format was NULL
 * there was output.
 */
int log_msg(int level,
	const char *file, const char *function, const int line_number,
	const char *format, ...) {
	va_list	args;
	char	msg[256];		// user message
	struct timespec ts;

	// make sure we have something to log
	if (!format)	return -1;	// error - require format string
	if (!*format)	return 0;	// silly format string, but let it go

	// lock the actual write(s) that may be through streams that may be
	// managed by SIG_ROTATE
	pthread_mutex_lock(&log_lock);

	// get a timestamp
	if (clock_gettime(CLOCK_REALTIME, &ts) == 0) {
		// TODO: report error
	}

	/* format the user message contents */
	va_start(args, format);
	vsnprintf(msg , sizeof(msg), format, args);
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

	return 0;	// success
}

/**
 * @fn void log_do_head(LOG_CHANNEL  *channel)
 * @brief Write the head for XML and Json output.
 * @param channel the channel to handle
 */
void log_do_head(LOG_CHANNEL  *channel) {
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
	// disable all log_channels
	// if a channel was file based, flush and close it
	LOG_CHANNEL  *log_ch = (LOG_CHANNEL *) log_channels;
	for (int n = 0; n < LOG_CH_COUNT; n++, log_ch++) {
		log_close_channel(log_ch);
	}

	// stop the logrotate support
	log_enable_logrotate(0);
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

	log_do_head(channel);

	// user has set up at least one channel
	configured = true;

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
	channel->stream = file;
	channel->level = level;
	channel->formatter = formatter;
	channel->line_buffered = line_buffered;

	// ejh xml
	log_do_head(channel);

	// user has set up at least one channel
	configured = true;

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
 * @brief Re-open a channel to support programatic logrotate.
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

	// ejh xml
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

	// ejh xml
	channel->sequence = 0;
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

	// ejh xml
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
