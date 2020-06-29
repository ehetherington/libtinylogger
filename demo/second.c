#include "tinylogger.h"

#define DEBUG_PATHNAME "/tmp/second.log"
int main(void) {
	/*
	 * Change main stderr format to systemd and add output to a file.
	 * Set a second file output stream to DEBUG_PATHNAME.
	 *  set log level to FINE
	 *  set format to debug
	 *  turn line buffering on (useful with tail -f, for example)
	 */
	LOG_CHANNEL *ch1 = log_open_channel_s(stderr, LL_INFO, log_fmt_systemd);
	LOG_CHANNEL *ch2 = log_open_channel_f(DEBUG_PATHNAME, LL_FINE, log_fmt_debug, true);
	log_notice("this message will be printed to both");
	log_info("this message will be printed to both");
	log_debug("this message will be printed to file only");
	log_finer("this message will not be printed at all");
	log_close_channel(ch1);
	log_close_channel(ch2);

	return 0;
}
