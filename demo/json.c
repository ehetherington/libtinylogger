#include <stdlib.h>
#include <strings.h>
#include <unistd.h>

#include "tinylogger.h"
#include "demo-utils.h"

#define LOG_FILE "log.json"	/**< the output file */
#define N_REPS 1			/**< increase for a long file */

/*
 * a little russian taken from:
 * https://www.w3.org/2001/06/utf-8-test/UTF-8-demo.html
 */
unsigned char utf8_sample[] = {
	0xd0, 0x97, 0xd0, 0xb0, 0xd1, 0x80, 0xd0, 0xb5,
	0xd0, 0xb3, 0xd0, 0xb8, 0xd1, 0x81, 0xd1, 0x82,

	0xd1, 0x80, 0xd0, 0xb8, 0xd1, 0x80, 0xd1, 0x83,
	0xd0, 0xb9, 0xd1, 0x82, 0xd0, 0xb5, 0xd1, 0x81,

	0xd1, 0x8c, 0x20, 0xd1, 0x81, 0xd0, 0xb5, 0xd0,
	0xb9, 0xd1, 0x87, 0xd0, 0xb0, 0xd1, 0x81, 0x20,

	0xd0, 0xbd, 0xd0, 0xb0, 0x20, 0xd0, 0x94, 0xd0,
	0xb5, 0xd1, 0x81, 0xd1, 0x8f, 0xd1, 0x82, 0xd1,

	0x83, 0xd1, 0x8e, 0x20, 0xd0, 0x9c, 0xd0, 0xb5,
	0xd0, 0xb6, 0xd0, 0xb4, 0xd1, 0x83, 0xd0, 0xbd,

	0xd0, 0xb0, 0xd1, 0x80, 0xd0, 0xbe, 0xd0, 0xb4,
	0xd0, 0xbd, 0xd1, 0x83, 0xd1, 0x8e, 0x20, 0xd0,

	0x9a, 0xd0, 0xbe, 0xd0, 0xbd, 0xd1, 0x84, 0xd0,
	0xb5, 0xd1, 0x80, 0xd0, 0xb5, 0xd0, 0xbd, 0xd1,

	0x86, 0xd0, 0xb8, 0xd1, 0x8e, 0x20, 0xd0, 0xbf,
	0xd0, 0xbe,

	0x00
};


/*
 * sixteenth note (U+266C)
 */
unsigned char sixteenth_note[] = {
	0xe2, 0x99, 0xac,
	0x00
};

/*
 * G-clef (U+1D11E)
 */
unsigned char g_clef[] = {
	0xf0, 0x9d, 0x84, 0x9e,
	0x00
};

/**
 * @fn int main(void)
 * @brief Short demo of the JSON fromatter.
 *
 * @detail Can output a single `log` object or a series of `record` objects.
 * Default output is a log, -r option selects a series of records.
 *
 * The only escaping done is for the below mentioned characters.
 *
 * @return 0
 */
int main(int argc, char **argv) {
	log_formatter_t format = log_fmt_json;

	/*
	 * check if the file already exists
	 */
	check_append(LOG_FILE);

	/*
	 * see if just a stream of records was requested
	 */
	for (int n = 1; n < argc; n++) {
		if (strcasecmp("-r", argv[n]) == 0) {
			format = log_fmt_json_records;
		}
	}

	/*
	 * log_set_json_notes() only has an effect if the library was compiled with
	 * ENABLE_JSON_HEADER. ("notes" is a field in the header)
	 */
	if (format != log_fmt_json_records) {
		//log_set_json_notes("hello\nworld");
		log_set_json_notes((char *) utf8_sample);
	}

	/*
	 * Print a series of JSON formatted messages
	 * Use the JSON formatter
	 * Turn off line buffered output. Json format would not typically be
	 * monitored in a line by line manner. It would usually be a "bulk" storage
	 * situation.
	 */
	LOG_CHANNEL *ch = log_open_channel_f(LOG_FILE, LL_INFO, format, false);
	(void) ch;

	for (size_t n = 0; n < N_REPS; n++) {
		// On linux, both the sixteen not and g-clef are displayed correctly.
		// On cygwin, the sixteen note is displayed correctly, but the g-clef
		// is displayed as a single character wide outline of a box.
		log_info("sixteenth_note (%s), g_clef (%s)", sixteenth_note, g_clef);
		log_info("\" quotes and \\ backslashes are escaped");
		log_info("some russian in UTF-8: %s", utf8_sample);
	}

	/*
	 * Throw in a memory dump
	 */
	char buf[256];

	for (size_t n = 0; n < sizeof(buf); n++) {
		buf[n] = n;
	}
	log_memory(LL_INFO, buf, sizeof(buf), "hello, %s", "world");


	/*
	 * control characters 0x01 - 0x1f (skip 0)
	 */
	char control[32] = {0};
	for (int n = 0; n <31; n++) {
		control[n] = n + 1;
	}
	log_info("control characters: %s", control);


	log_done();

	return 0;
}
