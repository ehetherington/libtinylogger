#include <stdlib.h>
#include <unistd.h>

#include "tinylogger.h"
#include "demo-utils.h"

#define LOG_FILE "tinylogger.json"	/**< the output file */
#define N_REPS 1					/**< increase for a long file */

/**
 * @fn int main(void)
 * @brief Short demo of the JSON fromatter.
 *
 * The only escaping done is for the below mentioned characters.
 *
 * @return 0
 */
int main(void) {
	/*
	 * check if the file already exists
	 */
	check_append(LOG_FILE);

	/*
	 * Print a series of JSON formatted messages
	 * Use the JSON formatter
	 * Turn off line buffered output. Json format would not typically be
	 * monitored in a line by line manner. It would usually be a "bulk" storage
	 * situation.
	 */
	LOG_CHANNEL *ch1 = log_open_channel_f(LOG_FILE, LL_INFO, log_fmt_json, false);
	(void) ch1;	// quiet the "unused variable" warning

	for (size_t n = 0; n < N_REPS; n++) {
		log_info("\b backspaces are escaped for JSON output", n);
		log_info("\r carriage returns are escaped for JSON output", n);
		log_info("\f formfeeds are escaped for JSON output", n);
		log_info("\n newlines are escaped for JSON output", n);
		log_info("\t tabs are escaped for JSON output", n);
		log_info("\" quotes are escaped for JSON output", n);
		log_info("\\ backslashes are escaped for JSON output", n);
	}

	/*
	 * Throw in a memory dump
	 */
	char buf[256];

	for (size_t n = 0; n < sizeof(buf); n++) {
		buf[n] = n;
	}
	log_memory(LL_INFO, buf, sizeof(buf), "hello, %s", "world");

	log_done();

	return 0;
}
