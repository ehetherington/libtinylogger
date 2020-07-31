#include "tinylogger.h"

#define FILENAME "second.log"

/**
 * @fn int main(void)
 *
 * @brief Log to two streams using different formats
 *
 * Messages may be logged to two different streams using different formats.
 * This can be useful when you want to have the main output left as intended,
 * but want a second, more verbose output for debug purposes.
 *
 * This example uses one channel for systemd style format at LL_INFO level,
 * while logging a verbose log_fmt_debug format at LL_FINE level. This would
 * allow a detailed view of program progress for development and debug
 * purposes.
 */
int main(void) {
	/*
	 * Set the "main" channel to output to stderr at LL_INFO level.
	 */
	LOG_CHANNEL *ch1 = log_open_channel_s(stderr, LL_INFO, log_fmt_systemd);

	/**
	 * Set a second channel to log to a file at LL_FINE level and use the
	 * log_fmt_debug format.  Turn line buffering on (useful with tail -f, for
	 * example).
	 */
	LOG_CHANNEL *ch2 = log_open_channel_f(FILENAME, LL_FINE, log_fmt_debug, true);

	log_notice("this message will be printed to both");
	log_info("this message will be printed to both");
	log_debug("this message will be printed to file only");
	log_finer("this message will not be printed at all");

	log_close_channel(ch1);
	log_close_channel(ch2);

	return 0;
}
