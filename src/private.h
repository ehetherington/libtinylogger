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
 */

#ifndef _PRIVATE_H
#define _PRIVATE_H 1

#include "config.h"

#include <stdio.h>
#include <stdbool.h>
#include <time.h>

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#define LOG_CH_COUNT 2	/**< The number of channels supported. */
#define TIMESTAMP_LEN 40	/**< buffer size for formatting date/time */
#endif /* DOXYGEN_SHOULD_SKIP_THIS */

/**
 * @struct _logChannel
 * Parameters used to configure a logging channel.
 * If pathname is supplied, it is opened in append mode. Then, if a rotate
 * signal is caught, the file is closed and reopened to support log rotation.
 *
 * Otherwise the supplied output * stream is used.
 */
struct _logChannel {
	LOG_LEVEL		level;		/**< the minimum level to log */
	log_formatter_t	formatter;	/**< the formatter to use */
	char			*pathname;	/**< pathname of the file, if logging to file */
	bool			line_buffered;	/**< line bufferd if true */
	FILE			*stream;	/**< the stream for output */
};

int log_xml_do_head(FILE *stream);
int log_xml_do_tail(FILE *stream);

#endif /* _PRIVATE_H */
