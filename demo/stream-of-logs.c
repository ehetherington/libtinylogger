#include <tinylogger.h>

/**
 * @fn int main(void)
 * @brief short example of logging json to stdout
 * @detail Demonstrates streaming multiple complete json logs (with a root
 * element of `log`).
 *
 * (Generates test log for the companion JSON-Logreader).
 *
 */
int main(void) {
	LOG_CHANNEL *ch;

	// log_set_json_notes() affects the subsequent logs to be opened
	log_set_json_notes("this is log 1 - it's sequence starts at 1");
	ch = log_open_channel_s(stdout, LL_INFO, log_fmt_json);
	log_info("one");
	log_info("two");

	// re-opening a log channel will also latch the current notes
	log_set_json_notes("this is log 2 - it's sequence also starts at 1");
	log_reopen_channel(ch);
	log_info("three");
	log_info("four");

	// re-opening a log channel will also latch the current notes
	log_set_json_notes("this is log 3 - it's sequence also starts at 1");
	log_reopen_channel(ch);
	log_info("five");
	log_info("six");

	log_close_channel(ch); // log_close_channel() emits the closing sequence
}
