
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

#include <tinylogger.h>

#define LOG_FILENAME "testLogger.log"

/**
 * @fn void log_messages(void)
 * @brief log messages of all levels
 */
static void log_messages() {
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
}

/**
 * @fn int main(int, char **)
 *
 * @various aspects of log levels
 *
 * log_get_level() does a reverse lookup of the LL_XXX associated with
 * a level string. It may be used when a program has an option of setting
 * the log level. The option may be specified using the level string, and
 * the associated LL_XXX value obtained.
 *
 * Messages are logged even before a LOG_CHANNEL is initialized. They are sent
 * to stderr in the log_fmt_standard format.
 *
 * log_set_pre_init_level() can be used to change the default pre-int log
 * threshold from LL_INFO.
 *
 */
int main(int argc, char **argv) {

	/*
	 * Test that get_log_level() properly looks up the level labels.
	 * The lookup is case insensitive.
	 */
	char *test[] = {
		"ALL", "OFF",	// turn all messages ON/OFF
		"junk",			// unexpected string - returns LL_INVALID = -2
		"emerg",
		"alert",
		"crit",
		"severe",
		"err",
		"INFO",
		"WARNing",
		"notice",
		"coNfIg",
		"debug",
		"fine",
		"finer",
		"finest"
	};
	printf("==== checking log_get_level() (using printf())...\n");
	for (int n = 0; n < sizeof(test) / sizeof(test[0]); n++) {
 		printf("%s = %d\n", test[n], log_get_level(test[n]));
	}
	printf("==== checking log_get_level() (using printf()) done\n\n");

	/*
	 * The default logging config BEFORE ANY CONFIGURATION is:
	 * Log to stderr.
	 * Use the "standard" format:
	 *     timestamp - level - msg
	 * Print log levels equal to or greater than INFO
	 */
	log_emerg("==== start default level of LL_INFO");
	log_messages();
	log_emerg("==== end default level of LL_INFO");

	/*
	 * You may change the pre-init log level without formally initializing
	 * a LOG_CHANNEL. Output continues to stderr in the log_fmt_standard
	 * format.
	 * change the level to FINE, and all but FINER will be printed
	 */
	log_set_pre_init_level(LL_FINE);	// log LL_FINE or above
	log_emerg("==== start level of LL_FINE");
	log_messages();
	log_emerg("==== end level of LL_FINE");

	log_done();

	exit(EXIT_SUCCESS);
}

