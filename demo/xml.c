#include <stdlib.h>
#include <ctype.h>
#include <sys/stat.h>

#include "tinylogger.h"
#include "demo-utils.h"

#define LOG_FILE "tinylogger.xml"
#define N_MSGS 2

/**
 * @fn int main(void)
 *
 * @brief Short program to demonstrate the xml formatter.
 *
 * XML entities handled are
 * &amp;
 * &lt;
 * &gt;
 * &apos;
 * &quot;
 *
 * No special treatment of non-ascii characters is done.
 * 
 */
int main(void) {

	// check if the file already exists
	check_append(LOG_FILE);

	/*
	 * Print a series of xml formatted messages
	 * Use the xml formatter
	 * Don't use line buffered output
	 */
	LOG_CHANNEL *ch = log_open_channel_f(LOG_FILE, LL_INFO, log_fmt_xml, false);
	(void) ch;	// quiet the "unused variable" warning

	for (int n = 0; n < N_MSGS; n++) {
		log_info("Special chars are escaped in the "
		"\"<xml>\" format. & it's msg #%d.", n);
	}

	// flush and close all channels
	log_done();

	return 0;
}
