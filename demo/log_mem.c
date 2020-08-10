#include <stdio.h>

#include "tinylogger.h"

/**
 * @fn int main(void)
 *
 * @brief Demonstrate use of log_memory()
 *
 * All byte values are used, and the corresponding printable characters are
 * displayed.
 *
 * The hex dump lines always start with two spaces, regardless of the length of
 * the buffer offset. This "marks" them as continuation lines of the starting
 * message. The date/time stamp starts with numbers, and hex dump does not.
 * This makes construction of a parser that recognizes this pattern
 * straightforward.
 *
 * Note the treatment of the partial final line;
 */
int main(void) {
	char buf[256 + 8];

	for (int n = 0; n < sizeof(buf); n++) {
		buf[n] = n;
	}

	log_memory(LL_INFO, buf, sizeof(buf), "hello, %s", "world");
}
