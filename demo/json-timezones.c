#include <stdlib.h>
#include <time.h>

#include <tinylogger.h>
#include "demo-utils.h"

/**
 * IMPORTANT
 * src/json-formatter.c MUST be compiled with -DTIMEZONE_TEST
 * see src/Makefile.am or quick-start/config.h
 */

/**
 * Support for testing the JSON-LogReader companion project
 */
int main(void) {
	LOG_CHANNEL *ch;
	
	ch = log_open_channel_s(stdout, LL_INFO, log_fmt_json);

	putenv("TZ=Europe/Paris");
	tzset();
	log_info("Europe/Paris");

	putenv("TZ=UTC");
	tzset();
	log_info("UTC");

	putenv("TZ=America/New_York");
	tzset();
	log_info("America/New_York");

	log_close_channel(ch);
}
