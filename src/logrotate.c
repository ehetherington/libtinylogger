/*
 * (C) 2020 Edward Hetherington
 * This code is licensed under MIT license (see LICENSE in top dir for details)
 */

/** @file		logrotate.c
 *  @brief		logrotate support
 *  @details	Use of logrotate is optional
 *  @author		Edward Hetherington
 */

#include "config.h"

#define _GNU_SOURCE

#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/types.h>
#include <pthread.h>
#include <string.h>

#include "tinylogger.h"
#include "private.h"

extern pthread_mutex_t log_lock;

extern LOG_CHANNEL log_channels[];

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
	LOG_CHANNEL ch = (LOG_CHANNEL) log_channels;

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

		for (int n = 0; n < LOG_CH_COUNT; n++, ch++) {
			// just flush streams passed in by the user
			// opening/closing is under user control
			if ((ch->pathname == NULL) && (ch->stream != NULL)) {
				fflush(ch->stream);
				continue;
			}

			// make sure we have a stream to flush/close
			if (ch->stream) {
				fflush(ch->stream);
				fclose(ch->stream);
				ch->stream = NULL;
			}

			// re-open the stream
			if (ch->pathname) {
				ch->stream = fopen(ch->pathname, "a");
			}

			// set line buffering mode on successfully re-opend file
			if ((ch->stream != NULL) && ch->line_buffered) {
				setvbuf(ch->stream, NULL, _IOLBF, BUFSIZ);
			}
		}

		pthread_mutex_unlock(&log_lock);
	}

	// done
	pthread_mutex_lock(&log_lock);
	ch = (LOG_CHANNEL) log_channels;
	for (int n = 0; n < LOG_CH_COUNT; n++, ch++) {
		if ((ch->pathname != NULL) && (ch->stream != NULL)) {
			fflush(ch->stream);
			fclose(ch->stream);
			bzero((void *) ch, sizeof(*ch));
		}
	}
	pthread_mutex_unlock(&log_lock);

	return NULL;
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
	if ((signal == 0) && rotate_config.thread_running) {
		pthread_kill(rotate_config.log_thread, rotate_config.signal);
		pthread_join(rotate_config.log_thread, NULL);

		rotate_config.thread_running = false;
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
