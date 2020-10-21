#include <stdlib.h>
#include <stdbool.h>
#include <time.h>

#include <tinylogger.h>
#include "demo-utils.h"

/**
 * ===== IMPORTANT =====
 * src/json-formatter.c MUST be compiled with -DTIMEZONE_TEST
 * see src/Makefile.am or quick-start/config.h
 * Or, just change it in json-formatter.c
 * ===== IMPORTANT =====
 */

bool get_timestamp(char *buf, int buf_len) {
	struct timespec ts;

	// get a timestamp
	if (clock_gettime(CLOCK_REALTIME, &ts) == -1) {
		return false;
	}

	// emulate the json timestamp without timezone
	log_format_timestamp(&ts, FMT_UTC_OFFSET | SP_NANO, buf, buf_len);

	return true;
}

/**
 * Support for testing the JSON-LogReader companion project
 */
int main(void) {
	char timestamp[128] = {0};
	LOG_CHANNEL *ch;

	ch = log_open_channel_s(stdout, LL_INFO, log_fmt_json);

	putenv("TZ=Europe/Paris");
	tzset();
	get_timestamp(timestamp, sizeof(timestamp));
	log_info("Logged at %s in Europe/Paris", timestamp);

	putenv("TZ=UTC");
	tzset();
	get_timestamp(timestamp, sizeof(timestamp));
	log_info("Logged at %s in UTC (or thereabouts)", timestamp);

	putenv("TZ=America/New_York");
	tzset();
	get_timestamp(timestamp, sizeof(timestamp));
	log_info("Logged at %s in America/New_York", timestamp);

	log_close_channel(ch);
}
