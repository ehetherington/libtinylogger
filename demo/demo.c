// from:
// https://stackoverflow.com/questions/53188731/logging-compatibly-with-logrotate

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <systemd/sd-daemon.h>

// During library build, the local tinylogger.h will be found
// After installing properly in /usr/local/lib (or somewhere else
// with the appropriate attention to /etc/ld.so.conf), it will pick
// up the installed header in /usr/local/include (or elsewhere, as
// appropriate).
#include <tinylogger.h>

/*
 * Not ideal, but demonstrates a Ctl-C SIGINT handler flushing the buffered
 * output by calling log_done().
 */
static void inthandler(int sig) {
	printf("\nSIGINT caught\n");
	log_done();
	printf("logs closed\n");
	exit(EXIT_SUCCESS);
}

#define TEST_PATHNAME "/tmp/testLogger.log"

int main(int argc, char **argv) {
	char buf[FILENAME_MAX];
	pid_t my_pid = getpid();
	LOG_CHANNEL *ch1, *ch2;

	/*
	 * Set up for Ctl-c handling to clean up before the program is done.
	 */
	if (signal(SIGINT, inthandler) == SIG_ERR) {
		perror("setting INT handler\n");
		printf("error setting INT handler\n");
		exit(EXIT_FAILURE);
	}

	/*
	 * Test that get_log_level() properly looks up the level labels.
	 */
	char *test[] = {
		"ALL", "NONE",
		"finest", "finer", "fine",
		"debug", "coNfIg", "INFO", "notice", "WARNing",
		"err", "severe", "crit", "alert", "emerg",
		"junk"
	};
	for (int n = 0; n < sizeof(test) / sizeof(test[0]); n++) {
 		printf("%s = %d\n", test[n], log_get_level(test[n]));
	}

	log_emerg("emerg %d", 0);
	log_alert("alert %d", 1);
	log_crit("crit %d", 2);
	log_err("err %d", 3);
	log_warning("warning %d", 4);
	log_notice("notice %d", 5);
	log_info("info %d", 6);
	log_debug("debug %d", 7);
	log_fine("fine %d", 8);
	log_finer("finer %d", 8);
	log_finest("finest %d", 8);
	log_finest("LL_ALL = %d", LL_ALL);

	/*
	 * The default logging config is:
	 * Log to stderr.
	 * Use the "standard" format:
	 *     timestamp - level - msg
	 * Print log levels equal to or greater than INFO
	 */
	log_notice("default settings: this message will be printed");
	log_info("default settings: this message will be printed");
	log_debug("default settings: this message will not be printed");

	/*
	 * change the level to FINE, and all will be printed
	 */
	log_set_pre_init_level(LL_FINE);
	log_info("TRACE output LEVEL: this message will be printed");
	log_debug("TRACE output LEVEL: this message will be printed");
	log_fine("FINE output LEVEL: this message will be printed");

	/*
	 * change the level to OFF, and none will be printed
	 */
	log_set_pre_init_level(LL_OFF);
	log_info("NONE output LEVEL: this message will NOT be printed");
	log_debug("NONE output LEVEL: this message will NOT be printed");
	log_fine("FINE output LEVEL: this message will NOT be printed");
 
	/*
	 * Use the STREAM configure function
	 * set output to stderr (again), log level to INFO, and format to debug
	 */
	ch1 = log_open_channel_s(stderr, LL_INFO, log_fmt_debug);
	log_notice("INFO output LEVEL: this message will be printed");
	log_info("INFO output LEVEL: this message will be printed");
	log_debug("INFO output LEVEL: this message will NOT be printed");

	/*
	 * Add output to a file and change primary format to systemd
	 * set the output file to TEST_PATHNAME
	 * set log level to FINE
	 * set format to debug
	 * turn line buffering on (useful with tail -f, for example)
	 */
	ch2 = log_open_channel_f(TEST_PATHNAME, LL_FINE, log_fmt_debug, true);
	log_close_channel(ch1);	// TODO: implement re-open with change of params
	ch1 = log_open_channel_s(stderr, LL_INFO, log_fmt_systemd);
	log_notice("this message will be printed to both");
	log_info("this message will be printed to both");
	log_debug("this message will be printed to file only");


	/*
	 * The "raw" log_msg may also be used. The __FILE__, __function__, and
	 * __LINE__ must be manually supplied.
	 */
	log_msg(LL_INFO, "file", "function", 1234, "hello %d", 55555);

	/*
	 * enable log rotate signal
	 */
	log_enable_logrotate(SIGUSR1);
	log_info("enter the following command to see that the logrotate has started");
	log_info("ps H -C demo -o 'pid tid cmd comm'");

	/*
	 * Normal output buffering may be used. Output will be buffered, and only
	 * written when the output buffer (4096) gets filled. Using tail -f will
	 * show the output being written in 4k chunks, with no relation to newlines.
	 */
	snprintf(buf, sizeof(buf), "kill -USR1 %d", my_pid);
	log_close_channel(ch1);
	ch1 = log_open_channel_s(stderr, LL_INFO, log_fmt_standard);
	log_close_channel(ch2);
	ch2 = log_open_channel_f(TEST_PATHNAME, LL_FINE, log_fmt_debug, false);
	for (int n = 0; n < 60; n++) {
		for (int m = 0; m < 10; m++) {
			log_info(buf);
		}
		sleep(1);
	}

	/*
	 * disable log rotate signal
	 */
	log_enable_logrotate(0);
	log_info("rotate handler has been stopped");
	log_info("enter the following command to see that the logrotate has stopped");
	log_info("ps H -C demo -o 'pid tid cmd comm'");

	// Hang out to show that the logrotate thread has stopped.
	sleep(10);

	/*
	 * A "log rotate" may be performed under program control.
	 * a) Move the current file to a new place.
	 * b) re-init the channel using the same pathname.
	 * Initializing a open file output channel will flush and close it before
	 * opening the "new" file. The file is moved first, all previously output
	 * info will be retained.
	 * The "new" file will continue with the next output.
	 */
	// create new pathname
	snprintf(buf, sizeof(buf), "%s.%d", TEST_PATHNAME, my_pid);
	log_info("orignal log was renamed to %s", buf);
	rename(TEST_PATHNAME, buf);	// move the file to the new name
	// re-init the channel to the original pathname
	log_reopen_channel(ch2);
	log_info("logging has continued %s", TEST_PATHNAME);

	/*
	 * make sure files are closed and rotate thread has stopped
	 */
	log_done();

	exit(EXIT_SUCCESS);
}

