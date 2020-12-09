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

#include <tinylogger.h>
#include "demo-utils.h"

#define LOG_FILENAME "testLogger.log"

/**
 * @fn void log_messages(void)
 * @brief log messages of all levels
 */
static void log_messages() {
	log_emerg("emerg %d",		log_get_level("emerg"));
	log_alert("alert %d",		log_get_level("alert"));
	log_crit("crit %d",			log_get_level("crit"));
	log_severe("severe %d",		log_get_level("severe"));
	log_err("err %d",			log_get_level("err"));
	log_warning("warning %d",	log_get_level("warning"));
	log_notice("notice %d",		log_get_level("notice"));
	log_info("info %d",			log_get_level("info"));
	log_config("config %d",		log_get_level("config"));
	log_debug("debug %d",		log_get_level("debug"));
	log_fine("fine %d",			log_get_level("fine"));
	log_finer("finer %d",		log_get_level("finer"));
	log_finest("finest %d",		log_get_level("finest"));
}

/*
 * Test that get_log_level() properly looks up the level labels.
 * The lookup is case insensitive.
 */
static char *test[] = {
	"ALL", "OFF",	// turn all messages ON/OFF
	"reject-me",	// unexpected string - returns LL_INVALID = -2
	"emerg",
	"alert",
	"crit",
	"severe",
	"err",
	"WARNing",
	"notice",
	"INFO",
	"coNfIg",
	"debug",
	"fine",
	"finer",
	"finest"
};

/**
 * @fn int main(int argc, char *argv[])
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
int main(int argc, char *argv[]) {
	bool json_examples = false;

	/*
	 * produce json examples if requested
	 */
	if (argc == 2) {
		int status = strcmp("--json-examples", argv[1]);
		if (status == 0) {
			json_examples = true;
		}
	}

	/*
	 * Test that get_log_level() properly looks up the level labels.
	 * The lookup is case insensitive.
	 */
	printf("==== checking log_get_level() (using printf())...\n");
	for (size_t n = 0; n < sizeof(test) / sizeof(test[0]); n++) {
		int level = log_get_level(test[n]);
 		printf("%s = %d\n", test[n], level);
	}
	printf("==== checking log_get_level() (using printf()) done\n\n");

	printf("==== showing systemd mapping...\n");
	for (size_t n = LL_EMERG; n <= LL_FINEST; n++) {
		printf("%2zu: %7s -> %s\n",
			n, log_labels[n].english, log_labels[n].systemd);
	}
	printf("==== showing systemd mapping done\n\n");

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

	if (!json_examples) exit(EXIT_SUCCESS);

	/*
	 * create a test file with a json log
	 */
	LOG_CHANNEL *ch;
	remove_or_exit("all-levels-log.json");		// start from scratch
	ch = log_open_channel_f("all-levels-log.json",
		LL_ALL, log_fmt_json, false);
	log_messages();
	log_close_channel(ch);

	/*
	 * create a test file with a series of json records
	 */
	remove_or_exit("all-levels-records.json");	// start from scratch
	ch = log_open_channel_f("all-levels-records.json",
		LL_ALL, log_fmt_json_records, false);
	log_messages();
	log_close_channel(ch);

	exit(EXIT_SUCCESS);
}
