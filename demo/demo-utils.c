/**
 * @file demo-utils.c
 * @brief Some support utils for the examples.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <sys/stat.h>

#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/prctl.h>
#include <fcntl.h>

#include "demo-utils.h"

/**
 * @fn void check_append(char *)
 * @brief Alert the user that an existing file will be appended to.
 *
 * All formatters append to the destination file. The rationale is to avoid
 * losing valuable info in a pre-existing log by accidentally overwriting it.
 * Managment of log files is left to "external forces".
 * 
 * For the demos, it is often desireable to start with a clean slate. An
 * opportunity is provided to make that choice.
 *
 * The user is presented with 3 options
 * - a append to the current file - function returns
 * - o overwrite the current file - An attempt to remove the current file is
 *   made. exit(EXIT_FAILURE) on failure, return on success.
 * - q quit - exit(EXIT_SUCCESS)
 *
 * @param filename the filename to check for existance
 */
void check_append(char *filename) {
	struct stat statbuf;
	int status = stat(filename, &statbuf);

	if (status != 0) return;

	while (true) {
		char c;
		printf("%s exists, enter:\n", filename);
		printf("  a to append to it\n");
		printf("  o to overwrite it\n");
		printf("  q or Ctl-c to exit\n");
		scanf(" %c",&c); c = tolower(c);

		switch (c) {
			case 'a': printf("appending to %s\n", filename); return;
			case 'q': printf("quitting\n"); exit(EXIT_SUCCESS);
			case 'o': {
				printf("deleting current %s, starting a new one\n", filename);
				int status = remove(filename);
				if (status != 0) {
					printf("could not remove %s\n", filename);
					exit(EXIT_FAILURE);
				}
				return;
			}
		}
	}
}

/**
 * @fn timespec_diff(struct timespec *, struct timespec *, struct timespec *)
 * @brief Compute the diff of two timespecs, that is a - b = result.
 * @details copy of the static version in tinylogger.c
 * @param a the minuend
 * @param b the subtrahend
 * @param result a - b
 */
void timespec_diff(struct timespec *a, struct timespec *b,
    struct timespec *result) {
    result->tv_sec  = a->tv_sec  - b->tv_sec;
    result->tv_nsec = a->tv_nsec - b->tv_nsec;
    if (result->tv_nsec < 0) {
        if (result->tv_sec >= 0) --result->tv_sec;
        result->tv_nsec += 1000000000L;
    }
}

/**
 * @fn long long get_time_nanos(struct timespec *ts)
 * @brief convert timespec values to a long long
 * @details With 64 bit integers, it overflows at around 232 years.
 *          `long long` required to get 64 bits on 32 bit machines.
 *
 * @param ts the timespec to convert
 * @return the number of nanoseconds it represents
 */
long long get_time_nanos(struct timespec *ts) {
	long long nanos = 1000000000LL * ts->tv_sec + ts->tv_nsec;
	return nanos;
}

/**
 * @fn char *get_proc_comm(void)
 *
 * @brief get the command name needed to match ps H -C command
 *
 * The command name is limited to 15 bytes plus the trailing null. It is
 * available from /proc/self/comm, except that that has a newline appended.
 *
 * @return pointer to the process name in an allocated buffer. The buffer must
 * be freed.
 */
char *get_proc_comm(void) {
	char *comm = calloc(TASK_COMM_LEN, 1);
	prctl(PR_GET_NAME, comm, NULL, NULL, NULL, NULL);
	return comm;
}
