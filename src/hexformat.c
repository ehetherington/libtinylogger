/*
 * (C) 2020 Edward Hetherington
 * This code is licensed under MIT license (see LICENSE in top dir for details)
 */

/** @file       hexformat.c
 *  @brief      Format a memory region to a hex + ascii representation.
 *  @details    The output includes a hex offset relative to the beginning of
 *              the input buffer, the bytes in hex, and the printable chars of
 *              the input.
 *
 * Example output:
 *
```
note the 2 leading spaces and the treatment of partial last lines:

  0000  00 01 02 03 04 05 06 07 08 09 0a 0b 0c 0d 0e 0f  ................
  0010  10 11 12 13 14 15 16 17 18 19 1a 1b 1c 1d 1e 1f  ................
  0020  20 21 22 23 24 25 26 27 28 29 2a 2b 2c 2d 2e 2f   !"#$%&'()*+,-./
  0030  30 31 32 33 34 35 36 37 38 39 3a 3b 3c 3d 3e 3f  0123456789:;<=>?
  0040  40 41 42 43 44 45 46 47 48 49 4a 4b 4c 4d 4e 4f  @ABCDEFGHIJKLMNO
  0050  50 51 52 53 54 55 56 57 58 59 5a 5b 5c 5d 5e 5f  PQRSTUVWXYZ[\]^_
  0060  60 61 62 63 64 65 66 67 68 69 6a 6b 6c 6d 6e 6f  `abcdefghijklmno
  0070  70 71 72 73 74 75 76 77 78 79 7a 7b 7c 7d 7e 7f  pqrstuvwxyz{|}~.
  0080  80 81 82 83 84 85 86 87 88 89 8a 8b 8c 8d 8e 8f  ................
  0090  90 91 92 93 94 95 96 97 98 99 9a 9b 9c 9d 9e 9f  ................
  00a0  a0 a1 a2 a3 a4 a5 a6 a7 a8 a9 aa ab ac ad ae af  ................
  00b0  b0 b1 b2 b3 b4 b5 b6 b7 b8 b9 ba bb bc bd be bf  ................
  00c0  c0 c1 c2 c3 c4 c5 c6 c7 c8 c9 ca cb cc cd ce cf  ................
  00d0  d0 d1 d2 d3 d4 d5 d6 d7 d8 d9 da db dc dd de df  ................
  00e0  e0 e1 e2 e3 e4 e5 e6 e7 e8 e9 ea eb ec ed ee ef  ................
  00f0  f0 f1 f2 f3 f4 f5 f6 f7 f8 f9 fa fb fc fd fe ff  ................
  0100  00 01 02 03 04 05 06 07                          ........
```
  @author Edward Hetherington
*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "private.h" /**< make sure declaration and definition match */

#ifndef DOXYGEN_SHOULD_SKIP_THIS

/*
 * Each line has a variable length "buffer address" field, followed by a
 * fixed length format. The hex plus ascii regions are constant length.
 */

/** Each line represents up to 16 bytes of the input buffer */
#define BYTES_PER_LINE 16

/* Each output line starts with two spaces, and there are two spaces between
 * the buffer address field and the hex fields, Then there are two spaces
 * separating the hex fields and the printable ascii input bytes.
 */

/**
 * Each byte of the input buffer is represented by a hex pattern = "XX " (three
 * chars).
 * ASCII_OFFSET is the offset from the variable length address offset field to
 * the printable representation of the input bytes.
 */
#define ASCII_OFFSET (BYTES_PER_LINE * 3 + 1)

/**
 * ascii field has one positon per BYTES_PER_LINE
 */
#define TERMINATING_NULL_INDEX (ASCII_OFFSET + BYTES_PER_LINE)

/**
 * The printf format specifier for the buffer offset field.
 * Must match MAX_ADDRESS_FIELD for proper results.
 */
#define ADDRESS_FORMAT "  %04zu  "

/**
 * Assume 64 bit addresses => 8 hex digits
 *
 * max address field = 2 spaces + 8 hex digits + 2 spaces
 *
 * The address offsets are printed with a minimum of four hex digits, but may
 * be up to 8 digits (64 bits). MAX_ADDRESS_FIELD includes two leading spaces,
 * the maximum of 8 digits, and the two trailing spaces.
 */
#define MAX_ADDRESS_FIELD (12)

/**
 * The fixed length portion of the line...
 * (3 * BYTES_PER_LINE + 1)                 (ASCII_OFFSET)
 * + BYTES_PER_LINE                         (16)
 * + separating newline or terminating null (1)
 */
#define FIXED_LINE_LENGTH (ASCII_OFFSET + BYTES_PER_LINE + 1)

/**
 * max line =
 *   The variable length offset field + the FIXED_LINE_LENGTH
 */
#define MAX_LINE_LENGTH (MAX_ADDRESS_FIELD + FIXED_LINE_LENGTH)

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

static char const digits[16] = {
	'0', '1', '2', '3', '4', '5', '6', '7',
	'8', '9', 'a', 'b', 'c', 'd', 'e', 'f'
};

/**
 * @fn void fmt_line(char const * const buf, size_t const len, char *line_buf)
 *
 * @brief format the data portion of an output line.
 *
 * This function produces the fixed length output after the address offset
 * field
 */
static void fmt_line(unsigned char const *buf, size_t const len, char *line_buf) {
	size_t n;

	/* actual data */
	for (n = 0; n < len; n++) {
		/* hex */
		line_buf[n * 3    ] = digits[(buf[n] >> 4)];
		line_buf[n * 3 + 1] = digits[(buf[n] & 0x0f)];
		line_buf[n * 3 + 2] = ' ';

		/* ascii */
		line_buf[ASCII_OFFSET + n] = isprint(buf[n]) ? buf[n] : '.';
	}

	/* pad a short last line with spaces */
	for (; n < BYTES_PER_LINE; n++) {
		/* hex */
		line_buf[n * 3    ] = ' ';
		line_buf[n * 3 + 1] = ' ';
		line_buf[n * 3 + 2] = ' ';

		/* ascii */
		line_buf[ASCII_OFFSET + n] = ' ';
	}

	/* put the extra space between the hex and ascii fields */
	line_buf[ASCII_OFFSET - 1] = ' ';

	/* Terminate the buffer. If followed by more lines, the main routine
	 * replaces the '\0' with a '\n'.
	 * May be omitted when not debugging this function.
	 */
	line_buf[TERMINATING_NULL_INDEX] = '\0';
}

/**
 * If the address of the buffer is NULL, return a pointer to a message instead
 * of a hex format result.
 */
#define NULL_POINTER_MSG \
	"  0000           <null pointer - check your code >                       \n"

/**
 * If the length of the buffer is less than 1, return a pointer to message
 * instead of a hex format result.
 */
#define NO_CONTENT_MSG \
	"  0000           <no content - zero length buffer>                       \n"

/**
 * @fn char *log_hexformat(void const * const mem, size_t const len)
 *
 * @brief Format a memory region to hex + ascii representation.
 *
 * @param mem accept a pointer to anything
 * @param len the length of the memory region
 *
 * @return A malloc(3)'ed buffer with the results. If mem is NULL or len is
 * zero, a "result" with an appropriate message is returned. If the malloc(3)
 * fails, NULL is returned. All non-null results must be freed with free(3).
 */
char *log_hexformat(void const * const mem, size_t const len) {
	unsigned char const * const u_mem = mem;	/* avoid a cast */
	char *hex_buf;
	char *current_ptr;
	size_t num2do = len;
	size_t num_done = 0;
	size_t total_lines;

	/* Sanity check buffer address. Use strdup(3) so that it can be free(3)'d.
	 */
	if (u_mem == NULL) {
		return strdup(NULL_POINTER_MSG);
	}

	/* Sanity check buffer length. Use strdup(3) so that it can be free(3)'d.
	 */
	if (len < 1) {
		return strdup(NO_CONTENT_MSG);
	}

	/* calculate number of lines */
	total_lines = len / BYTES_PER_LINE;
	total_lines += (len % BYTES_PER_LINE) ? 1 : 0;

	/* allocate buffer to construct those lines */
	hex_buf = malloc(total_lines * MAX_LINE_LENGTH);

	/* return NULL on failure */
	if (hex_buf == NULL) return NULL;

	/* start at the beginning */
	current_ptr = hex_buf;

	while (num2do > 0) {
		int num_this_line = num2do < BYTES_PER_LINE ? num2do : BYTES_PER_LINE;

		/* produce the variable length buffer offset, and remember the length */
		int header_len =
			snprintf(current_ptr, MAX_ADDRESS_FIELD + 1, ADDRESS_FORMAT, num_done);

		/* produce the fixed length hex/ascii regions */
		fmt_line(u_mem + num_done, num_this_line, current_ptr + header_len);

		/* keep track of progress */
		num_done += num_this_line;
		num2do -= num_this_line;

		/* update the "next" pointer */
		current_ptr += header_len + FIXED_LINE_LENGTH;

		/* if not done yet, replace the '\0' with a '\n' */
		if (num2do > 0) {
			*(current_ptr - 1) = '\n';
		} else {
			*(current_ptr - 1) = '\0';
		}
	}

	return hex_buf;
}
