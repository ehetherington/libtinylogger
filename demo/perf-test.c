#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <tinylogger.h>
#include "demo-utils.h"

#define MSGS_PER_TEST 1000

#define OUTPUT_FILE "/dev/null"

/**
 * @struct format_test
 */
struct format_test {
	char *label; /**< the name of the log formatter */
	log_formatter_t formatter; /**< the address of the log formatter */
} tests[] = {
	{"log_fmt_basic", log_fmt_basic},
	{"log_fmt_systemd", log_fmt_systemd},
	{"log_fmt_standard", log_fmt_standard},
	{"log_fmt_debug", log_fmt_debug},
	{"log_fmt_debug_tid", log_fmt_debug_tid},
	{"log_fmt_debug_tname", log_fmt_debug_tname},
	{"log_fmt_debug_tall", log_fmt_debug_tall},
	{"log_fmt_xml", log_fmt_xml},
	{"log_fmt_json", log_fmt_json},
};
#define N_FORMATS (sizeof(tests) / sizeof(tests[0]))

/**
 * @struct results
 */
struct result {
	long long first;
	long long min;
	long long median;
	long long mean;
	long long max;
	long long data[MSGS_PER_TEST];
} results[N_FORMATS] = {0};

void dump_ts(char *label, struct timespec *ts) {
	fprintf(stderr, "%s: tv_sec=%ld tv_nsec=%09ld\n", label, ts->tv_sec, ts->tv_nsec);
}

/**
 * @fn void run_test(struct format_test *test, int n_msgs)
 *
 * @brief Run an elapsed time test using the specified message formatter.
 *
 * @detail This is a simple elapsed time test. A large number of messages are
 *         written to /dev/null. An time-per-message average is then computed.
 *         This is a simple elapsed time test, so any other running processes
 *         will inflate the value.
 */
void run_test(struct format_test *test, int n_msgs) {
	struct timespec ts_start;
	struct timespec ts_end;
	long long elapsed_time;
	int n;
	LOG_CHANNEL *ch;

	ch = log_open_channel_f(OUTPUT_FILE, LL_INFO, test->formatter, false);

	clock_gettime(CLOCK_REALTIME, &ts_start);

	// generate many log messages
	for (n = 0; n < n_msgs; n++) {
		log_info(test->label);
	}

	clock_gettime(CLOCK_REALTIME, &ts_end);

	long long start_time = get_time_nanos(&ts_start);
	long long   end_time = get_time_nanos(&ts_end );
	elapsed_time = get_time_nanos(&ts_end) - get_time_nanos(&ts_start);

	if (elapsed_time < 0) {
		dump_ts("start", &ts_start);
		dump_ts("end", &ts_end);
		fprintf(stderr, "  start_time = %lld\n", start_time);
		fprintf(stderr, "    end_time = %lld\n", end_time);
		fprintf(stderr, "elapsed_time = %lld\n", elapsed_time);
		exit(EXIT_FAILURE);
	}

	printf("%s: %lld\n", test->label, elapsed_time / (n_msgs));

	log_close_channel(ch);
}

int cmp_ll(const void *p1, const void *p2) {
	long long const *pll1 = p1;
	long long const *pll2 = p2;
	if (*pll1 == *pll2) return 0;
	return (*pll1 > *pll2) ? 1 : -1;
	return 0;
}

long long time_msg(char *fmt, char *arg) {
	struct timespec ts_start;
	struct timespec ts_end;

	clock_gettime(CLOCK_REALTIME, &ts_start);

// try to eliminate clock_gettime() bias with BATCH_MSGS
#define BATCH_MSGS
#ifndef BATCH_MSGS
	log_info(fmt, arg);
#else
	log_info(fmt, arg);
	log_info(fmt, arg);
	log_info(fmt, arg);
	log_info(fmt, arg);
	log_info(fmt, arg);

	log_info(fmt, arg);
	log_info(fmt, arg);
	log_info(fmt, arg);
	log_info(fmt, arg);
	log_info(fmt, arg);

	log_info(fmt, arg);
	log_info(fmt, arg);
	log_info(fmt, arg);
	log_info(fmt, arg);
	log_info(fmt, arg);

	log_info(fmt, arg);
	log_info(fmt, arg);
	log_info(fmt, arg);
	log_info(fmt, arg);
	log_info(fmt, arg);
#endif

	clock_gettime(CLOCK_REALTIME, &ts_end);

#ifndef BATCH_MSGS
	return get_time_nanos(&ts_end) - get_time_nanos(&ts_start);
#else
	return (get_time_nanos(&ts_end) - get_time_nanos(&ts_start)) / 20;
#endif
}

// markdown left justifies everything - bummer
// markdown aware editors get confused with the '_' characters - oh well
#define HEADER_0     " Format             |  min   | median |  mean  |   max   |  first "
#define HEADER_1     "--------------------|--------|--------|--------|---------|--------"
#define CONTENT_FMT  "%-19s | % 6lld | % 6lld | % 6lld | % 7lld | % 7lld"
void print_results(struct result *result, int n_results) {
	printf("%s\n", HEADER_0);
	printf("%s\n", HEADER_1);
	for (int n = 0; n < n_results; n++) {
		printf(CONTENT_FMT "\n",
			tests[n].label,
			result[n].min,
			result[n].median,
			result[n].mean,
			result[n].max,
			result[n].first);
	}
}

void try_one_at_a_time(struct format_test *test, struct result *result, int n_msgs) {
	int n;
	double sum;
	long long time[n_msgs];
	LOG_CHANNEL *ch;

	ch = log_open_channel_f(OUTPUT_FILE, LL_INFO, test->formatter, false);

	for (n = 0; n < n_msgs; n++) {
		time[n] = time_msg("%s", test->label);
	}

	result->first = time[0];

	for (sum = 0.0f, n = 0; n < n_msgs; n++) {
		sum += time[n];
	}
	result->mean = 0.5f + (sum / n_msgs);

	qsort(time, n_msgs, sizeof(long long), cmp_ll);

	result->min =  time[0];
	result->median = time[n_msgs / 2];
	result->max =  time[n_msgs - 1];

	log_close_channel(ch);
}

/**
 * @fn int main(void)
 * Run an elapsed time test using each message formatter
 */
int main() {
	struct timespec ts_start;
	struct timespec ts_end;
	struct tm   tm;
	long long elapsed_time;

	// check clock_gettime()
	clock_gettime(CLOCK_REALTIME, &ts_start);
	clock_gettime(CLOCK_REALTIME, &ts_end);
	elapsed_time = get_time_nanos(&ts_end) - get_time_nanos(&ts_start);
	printf("clock_gettime() = %lld\n", elapsed_time);

	// check clock_gettime() again
	clock_gettime(CLOCK_REALTIME, &ts_start);
	clock_gettime(CLOCK_REALTIME, &ts_end);
	elapsed_time = get_time_nanos(&ts_end) - get_time_nanos(&ts_start);
	printf("clock_gettime() = %lld\n", elapsed_time);

	// check localtime_r()
	clock_gettime(CLOCK_REALTIME, &ts_start);
	localtime_r(&(ts_start.tv_sec), &tm);
	clock_gettime(CLOCK_REALTIME, &ts_end);
	elapsed_time = get_time_nanos(&ts_end) - get_time_nanos(&ts_start);
	printf("clock_gettime() + localtime_r() = %lld\n", elapsed_time);

	// check localtime_r() again
	clock_gettime(CLOCK_REALTIME, &ts_start);
	localtime_r(&(ts_start.tv_sec), &tm);
	clock_gettime(CLOCK_REALTIME, &ts_end);
	elapsed_time = get_time_nanos(&ts_end) - get_time_nanos(&ts_start);
	printf("clock_gettime() + localtime_r() = %lld\n", elapsed_time);

	for (size_t test = 0; test < N_FORMATS; test++) {
		run_test(&tests[test], MSGS_PER_TEST);
	}

	// try to reduce outliers
	for (size_t n = 0; n < N_FORMATS; n++) {
		try_one_at_a_time(&tests[n], &results[n], MSGS_PER_TEST);
	}
	print_results(results, N_FORMATS);

	exit(EXIT_SUCCESS);
}
