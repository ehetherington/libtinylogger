/*
 * (C) 2020 Edward Hetherington
 * This code is licensed under MIT license (see LICENSE in top dir for details)
 */

/** @file       tinylogger.h
 *  @brief      The tinylogger header file.
 *  @details    A tiny logger facility for small linux projects.
 *  @author     Edward Hetherington
 */

//#include "config.h"

#ifndef _TINYLOGGER_H
#define _TINYLOGGER_H 1

#include <stdio.h>
#include <stdbool.h>
#include <time.h>

/**
 * Use these macros for logging messages. They set the log_level parameter,
 * and capture \_\_FILE\_\_, \_\_func\_\_, and \_\_LINE\_\_ for use by the
 * log_formatter_debug() formatter.
 */
#define log_emerg(...)   log_msg(LL_EMERG, __FILE__, __func__, __LINE__, __VA_ARGS__) /**< emerg */
#define log_alert(...)   log_msg(LL_ALERT, __FILE__, __func__, __LINE__, __VA_ARGS__) /**< alert */
#define log_crit(...)    log_msg(LL_CRIT, __FILE__, __func__, __LINE__, __VA_ARGS__) /**< crit */
#define log_severe(...)  log_msg(LL_SEVERE, __FILE__, __func__, __LINE__, __VA_ARGS__) /**< severe */
#define log_err(...)     log_msg(LL_ERR, __FILE__, __func__, __LINE__, __VA_ARGS__) /**< err */
#define log_warning(...) log_msg(LL_WARNING, __FILE__, __func__, __LINE__, __VA_ARGS__) /**< warning */
#define log_notice(...)  log_msg(LL_NOTICE, __FILE__, __func__, __LINE__, __VA_ARGS__) /**< notice */
#define log_info(...)    log_msg(LL_INFO, __FILE__, __func__, __LINE__, __VA_ARGS__) /**< info */
#define log_config(...)  log_msg(LL_CONFIG, __FILE__, __func__, __LINE__, __VA_ARGS__) /**< config */
#define log_debug(...)   log_msg(LL_DEBUG, __FILE__, __func__, __LINE__, __VA_ARGS__) /**< debug */
#define log_fine(...)    log_msg(LL_FINE, __FILE__, __func__, __LINE__, __VA_ARGS__) /**< fine */
#define log_finer(...)   log_msg(LL_FINER, __FILE__, __func__, __LINE__, __VA_ARGS__) /**< finer */
#define log_finest(...)  log_msg(LL_FINEST, __FILE__, __func__, __LINE__, __VA_ARGS__) /**< finest */

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#if defined __cplusplus
# define TL_BEGIN_C_DECLS   extern "C" {
# define TL_END_C_DECLS     }
#else
# define TL_BEGIN_C_DECLS   /* empty */
# define TL_END_C_DECLS     /* empty */
#endif
#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#ifndef DOXYGEN_SHOULD_SKIP_THIS
TL_BEGIN_C_DECLS
#endif /* DOXYGEN_SHOULD_SKIP_THIS */

/**
 * Merge the definitions in <systemd/sd-daemon.h> and java.util.logging.
 * Add convenience values of LL_OFF and LL_ALL.
 */
typedef enum log_level {
	LL_INVALID = -2, LL_OFF = -1,
	LL_EMERG, LL_ALERT, LL_CRIT, LL_SEVERE,
	LL_ERR, LL_WARNING, LL_NOTICE, LL_INFO,
	LL_CONFIG, LL_DEBUG, LL_FINE, LL_FINER, LL_FINEST,
	LL_ALL = LL_FINEST, LL_N_VALUES} LOG_LEVEL;

/**
 * @struct log_label
 * systemd/sd-daemon.h macro names were used directly (minus the "SD_" prefix).
 * java.util.logging.level names were merged in and mapped to the most
 * appropriate systemd value.
 *
 * Public for use by custom formatter.
 */
struct log_label {
    char *english;  /**< label used for most formats */
    char *systemd;  /**< label used for systemd format */
	int	java_level;	/**< java equivalent level */
};
extern struct log_label log_labels[LL_N_VALUES];

/**
 * Formatters must have this signature.
 */
typedef int (*log_formatter_t)(FILE *, int, struct timespec *, int,
    const char *, const char *, int, char *);

/**
 * For use of log_format_timestamp()
 */
typedef enum {
	SP_NONE,				/**< no seconds fraction     */
	SP_MILLI,				/**< .nnn                    */
	SP_MICRO,				/**< .nnnnnn                 */
	SP_NANO,				/**< .nnnnnnnnn              */
	FMT_ISO = 16,			/**< 'T' instead of ' '      */
	FMT_UTC_OFFSET = 32,	/**< add UTC offset "+00:00" */
	LOG_FMT_DELTA = 64,	    /**< print an elapsed time   */
	LOG_FMT_HMS = 128	    /**< elapsed time in H:M:S   */
} SEC_PRECISION;

struct _logChannel;
/** Opaque */
typedef struct _logChannel LOG_CHANNEL;

void log_set_pre_init_level(LOG_LEVEL log_level);
LOG_LEVEL log_get_level(const char *label);

/* select clock to use for timestamps */
bool log_select_clock(clockid_t clock_id);

/* channel control */
LOG_CHANNEL *log_open_channel_s(FILE *, LOG_LEVEL, log_formatter_t);
LOG_CHANNEL *log_open_channel_f(char *, LOG_LEVEL, log_formatter_t, bool);
int log_msg(int level, const char *file, const char *function,
	const int line, const char *format, ...);
int log_change_params(LOG_CHANNEL *, LOG_LEVEL, log_formatter_t);
int log_reopen_channel(LOG_CHANNEL *);
int log_close_channel(LOG_CHANNEL *);
void log_done(void);

/* the formatters */
int log_fmt_basic(FILE *, int, struct timespec *, int, const char *, const char *, int, char *);
int log_fmt_systemd(FILE *, int, struct timespec *, int, const char *, const char *, int, char *);
int log_fmt_standard(FILE *, int, struct timespec *, int, const char *, const char *, int, char *);
int log_fmt_debug(FILE *, int, struct timespec *, int, const char *, const char *, int, char *);
int log_fmt_tall(FILE *, int, struct timespec *, int, const char *, const char *, int, char *);
int log_fmt_debug_tid(FILE *, int, struct timespec *, int, const char *, const char *, int, char *);
int log_fmt_debug_tname(FILE *, int, struct timespec *, int, const char *, const char *, int, char *);
int log_fmt_debug_tall(FILE *, int, struct timespec *, int, const char *, const char *, int, char *);
int log_fmt_elapsed_time(FILE *, int, struct timespec *, int, const char *, const char *, int, char *);
int log_fmt_xml(FILE *, int, struct timespec *, int, const char *, const char *, int, char *);
int log_fmt_json(FILE *, int, struct timespec *, int, const char *, const char *, int, char *);

/* timestamp formatters for the formatters that include a timestamp */
void log_format_timestamp(struct timespec *ts, SEC_PRECISION precision, char *buf, int len);
void log_format_delta(struct timespec *ts, SEC_PRECISION precision, char *buf, int len);

int log_enable_logrotate(int signal);

#ifndef DOXYGEN_SHOULD_SKIP_THIS
TL_END_C_DECLS
#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#endif /* _TINYLOGGER_H */
