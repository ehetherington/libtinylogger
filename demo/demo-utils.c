#define _GNU_SOURCE

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
 * @brief alert the user that an existing file will be appended to
 *
 * All formatters append to the destination file. The rationale is to avoid
 * losing valuable info in a pre-existing log by accidentally overwriting it.
 * Managment of log files is left to "external forces".
 * 
 * For the demos, it is often desireable to start with a clean slate. An
 * opportunity is provided to make that choice.
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
 * @fn char *get_proc_name(void)
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
