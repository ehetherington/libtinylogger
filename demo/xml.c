#include "tinylogger.h"

#define LOG_FILE "tinylogger.xml"
#define N_MSGS 3

int main(void) {
	/*
	 * Print a series of xml formatted messages
	 * Use the xml formatter
	 * Use line buffered output
	 */
	LOG_CHANNEL *ch1 = log_open_channel_f(LOG_FILE, LL_INFO, log_fmt_xml, true);
	(void) ch1;	// quiet the "unused variable" warning

	for (int n = 0; n < N_MSGS; n++) {
		log_info("This message uses the \"<xml>\" format. It's msg #%d.", n);
	}

	log_done();

	return 0;
}
