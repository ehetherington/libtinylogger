#include <stdlib.h>
#include <unistd.h>

#include "tinylogger.h"

#define LOG_FILE "tinylogger.json"
#define N_REPS 1

int main(void) {
	/*
	 * Print a series of xml formatted messages
	 * Use the xml formatter
	 * Use line buffered output
	 */
	LOG_CHANNEL *ch1 = log_open_channel_f(LOG_FILE, LL_INFO, log_fmt_json, true);
	(void) ch1;	// quiet the "unused variable" warning

	for (int n = 0; n < N_REPS; n++) {
		log_info("\b backspaces are escaped for Json output", n);
		log_info("\r carriage returns are escaped for Json output", n);
		log_info("\f formfeeds are escaped for Json output", n);
		log_info("\n newlines are escaped for Json output", n);
		log_info("\t tabs are escaped for Json output", n);
		log_info("\" quotes are escaped for Json output", n);
		log_info("\\ backslashes are escaped for Json output", n);
	}

	log_done();

	return 0;
}
