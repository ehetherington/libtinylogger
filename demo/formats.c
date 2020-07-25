#define _GNU_SOURCE // for pthread_getname_np and other reasons

#include <time.h>
#include <locale.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>

#include "tinylogger.h"

// from kernel/sched.h
#define TASK_COMM_LEN 16
#define NAME_LEN TASK_COMM_LEN  // includes null termination

/**
 * @fn int log_fmt_custom_1(FILE *, struct timespec *, int,
 * const char *, const char *, int, char *)
 * @brief Output messages with a CUSTOM format.
 *
 * Use another familiar date/time format. Has thread id and thread name. Note
 * that the thread name of the main thread defaults to the command name.
 *
 * @param msg the actual user message
 * @param stream the output stream to write to
 * @param sequence ignored
 * @param ts the struct timespec timestamp
 * @param level the log level to print
 * @param file the name of the file to print
 * @param function the name of the function to print
 * @param line the line number to print
 * @param msg the actual use message to print
 * @return the number of characters written
 */
int log_fmt_custom_1(FILE *stream, int sequence, struct timespec *ts, int level,
	const char *file, const char *function, int line, char *msg) {
	char date[32];
	pthread_t thread = pthread_self();
	char thread_name[NAME_LEN];
	int rc;
	struct tm tm;

	// date
	if (localtime_r(&(ts->tv_sec), &tm) == &tm) {
		strftime(date, sizeof(date), "%B %d, %Y, %H:%M:%S", &tm);
	} else {
		snprintf(date, sizeof(date), "oops");
	}

	// thread name
	rc = pthread_getname_np(thread, thread_name, sizeof(thread_name));
	if (rc != 0) {
	    snprintf(thread_name, sizeof(thread_name), "unknown");
	}

	return fprintf(stream, "%s %-7s %ld:%s %s\n",
		date, log_labels[level].english,
		syscall(SYS_gettid), thread_name, msg);
}

/**
 * @fn int log_fmt_custom_2(FILE *, struct timespec *, int,
 * const char *, const char *, int, char *)
 * @brief Output messages with a CUSTOM format.
 *
 * Use another familiar date/time format. The debug info of file/function/line
 * is printed instead of thread id:thread name.
 *
 * @param msg the actual user message
 * @param stream the output stream to write to
 * @param sequence ignored
 * @param ts the struct timespec timestamp
 * @param level the log level to print
 * @param file the name of the file to print
 * @param function the name of the function to print
 * @param line the line number to print
 * @param msg the actual use message to print
 */
int log_fmt_custom_2(FILE *stream, int sequence, struct timespec *ts, int level,
	const char *file, const char *function, int line, char *msg) {
	char date[32];
	struct tm tm;

	// date
	if (localtime_r(&(ts->tv_sec), &tm) == &tm) {
		strftime(date, sizeof(date), "%B %d, %Y, %H:%M:%S", &tm);
	} else {
		snprintf(date, sizeof(date), "oops");
	}

	return fprintf(stream, "%s %-7s %s:%s:%d %s\n",
		date, log_labels[level].english,
		file, function, line, msg);
}

/**
 * @fn int main(void)
 * @brief Demonstrate available formats.
 *
 * Also demonstrate creating custom formats, and use of elapsed time formats.
 */
int main(void) {
	/*
	 * Change main stderr format to systemd and add output to a file.
	 * set the output file to TEST_PATHNAME
	 * set log level to FINE
	 * set format to debug
	 * turn line buffering on (useful with tail -f, for example)
	 */
	LOG_CHANNEL *ch1;

	ch1 = log_open_channel_s(stderr, LL_INFO, log_fmt_basic);
	log_info("this message uses the basic format");

	log_change_params(ch1, LL_INFO, log_fmt_systemd);
	log_info("this message uses the systemd format");

	log_change_params(ch1, LL_INFO, log_fmt_standard);
	log_info("this message uses the standard format");

	log_change_params(ch1, LL_INFO, log_fmt_debug);
	log_info("this message uses the debug format");

	log_change_params(ch1, LL_INFO, log_fmt_debug_tid);
	log_info("this message uses the debug_tid format");

	log_change_params(ch1, LL_INFO, log_fmt_debug_tname);
	log_info("this message uses the debug_tname format");

	log_change_params(ch1, LL_INFO, log_fmt_debug_tall);
	log_info("this message uses the debug_tall format");

	// Set the locale for the date to respond to the locale set in the
	// environment.
	// (requires the proper locales to be installed, of course)
	// To see it in action, try -
	// $ LC_ALL=es_ES ./formats
	setlocale(LC_ALL, "");

	log_change_params(ch1, LL_INFO, log_fmt_custom_1);
	log_info("this message uses a CUSTOM format");

	// explicitely set the locale
	setlocale(LC_ALL, "es_ES");

	log_change_params(ch1, LL_INFO, log_fmt_custom_2);
	log_info("this message uses a another CUSTOM format");

	log_change_params(ch1, LL_INFO, log_fmt_xml);
	log_info("this message has excaped \"<xml>\", apostrophe also '");
	log_info("this message has no escaped xml");

	/*
	 * Select CLOCK_MONOTONIC_RAW instead of CLOCK_REALTIME.
	 * CLOCK_MONOTONIC_RAW is the most appropriate for this use.
	 * Selecting a clock also resets the elapsed time.
	 */
	// The following reports 1 ns on my HP xw6600. I'm not sure what that
	// actually means.
	struct timespec resolution = {0};
	clock_getres(CLOCK_MONOTONIC_RAW, &resolution);
	log_change_params(ch1, LL_INFO, log_fmt_standard);
	log_info("CLOCK_MONOTONIC_RAW resolution = %ld ns", resolution.tv_nsec);

	// crude measurement of the time to write a message.
	log_change_params(ch1, LL_INFO, log_fmt_elapsed_time);
	log_select_clock(CLOCK_MONOTONIC_RAW);
	log_info("this message has elapsed time");
	log_info("this message has elapsed time");
	log_info("this message has elapsed time");
	log_info("this message has elapsed time");
	log_info("this message has elapsed time");

	// reset t0 - the elapsed time starts back at 0
	log_info("reset t0");
	log_select_clock(CLOCK_MONOTONIC_RAW);
	log_info("this message has elapsed time");
	log_info("this message has elapsed time");
	log_info("this message has elapsed time");
	log_info("this message has elapsed time");
	log_info("this message has elapsed time");

	log_done();

	return 0;
}
