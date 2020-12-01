#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <tinylogger.h>

/**
 *
 * The purpose of this program is to run UTF-8 test data through the logger
 * and the JSON-LogReader. The input and output files can then be compared to
 * verify proper operation of the JSON encoding and decoding processes.
 *
 * Two test files were used.
 *
 * UTF-8_test_file.html from:
 *     https://www.w3.org/2001/06/utf-8-test/UTF-8-demo.html
 *
 * UTF-8_Sampler.html from:
 *     http://kermitproject.org/utf8.html
 *
 * After removing the "header" lines added by the JSON tinylogger header,
 * adjusting the last lines which are not line-feed terminated, and using
 * dos2unix where appropriate, the files compare identically.
 *
 * The JSON "encoding" must be done on a linux machine, as libtinylogger is
 * linux specific, but the "decoding" may also be run on a windows machine.
 * 
 * This program reads the sample file and logs a message for each line of text
 * in the file. The complementary program JsonLogReader re-assembles the
 * original.
 *
 */

int main(int argc, char *argv[]) {
	FILE *fp;
    char line[BUFSIZ] = {0};

	if (argc != 2) {
		fprintf(stderr, "Usage: %s <input-file>\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	fp = fopen(argv[1], "r");
	if (fp == NULL) {
		fprintf(stderr, "can't open %s\n", argv[1]);
		exit(EXIT_FAILURE);
	}

	// set the sample filename BEFORE opening the channel
	log_set_json_notes(argv[1]);

	// open a log channel using log_fmt_json
	LOG_CHANNEL *ch = log_open_channel_s(stdout, LL_INFO, log_fmt_json);

	// log a message for each line of input after stripping the newline
	while (fgets(line, sizeof(line), fp) != NULL) {
		int len = strlen(line);
		if ((len > 0) && (line[len - 1] == '\n')) line[len - 1] = '\0';
        log_info("%s", line);
    }

	log_close_channel(ch);
	fclose(fp);
	exit(EXIT_SUCCESS);
}
