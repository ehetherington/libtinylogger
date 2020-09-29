/*
 * (C) 2020 Edward Hetherington
 * This code is licensed under MIT license (see LICENSE in top dir for details)
 */

/** @file       private.h
 *  @brief      Non-public implementation definitions
 *  @author     Edward Hetherington
 */

/*
 * This file shouldn't be included by user programs or installed into
 * /usr/local/include
 * This file should only be included AFTER tinylogger.h (needs
 * TL_BEGIN_C_DECLS, among other things).
 */

#ifndef _PRIVATE_H
#define _PRIVATE_H 1

#include "config.h"

#include <stdio.h>
#include <stdbool.h>
#include <time.h>

#include "tinylogger.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS
// TODO: Check if is necessary. C++ programs shouldn't be calling the
// included functions directly but the declarations may pollute the
// symbol space.
TL_BEGIN_C_DECLS

#define LOG_CH_COUNT 2	/**< The number of channels supported. */
#define TIMESTAMP_LEN 40	/**< buffer size for formatting date/time */

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

/**
 * @struct _logChannel
 * @brief Parameters used to configure a logging channel.
 * This structure definition is not "user" visible. It is protected with mutex
 * so that multiple threads can access it in a consistant state.
 */
struct _logChannel {
	LOG_LEVEL	level;			/**< the minimum level to log */
	log_formatter_t	formatter;	/**< the formatter to use */
	char		*pathname;		/**< pathname of the file, if logging to file */
	bool		line_buffered;	/**< line buffered if true */
	FILE		*stream;		/**< the stream for output */
	bool		wrap_records;	/**< enclose Json and XML in begin/end log info */
	int			sequence;		/**< sequence number for structured streams (Json and XML) */
	void (*open_action)(void);	/**< open function for structured streams (Json and XML) */
	void (*close_action)(void);	/**< close function for structured streams (Json and XML) */
};

int log_do_xml_head(FILE *stream);
int log_do_xml_tail(FILE *stream);
int log_do_json_head(FILE *stream);
int log_do_json_tail(FILE *stream);
void log_do_head(LOG_CHANNEL *);
void log_do_tail(LOG_CHANNEL *);

char *log_hexformat (void const * const addr, size_t const len);
char *log_get_timezone(char *tz, size_t tz_len);

#ifndef DOXYGEN_SHOULD_SKIP_THIS
TL_END_C_DECLS
#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#endif /* _PRIVATE_H */
