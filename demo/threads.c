// https://stackoverflow.com/questions/21091000/how-to-get-thread-id-of-a-pthread-in-linux-c-program
// man pthread_setname_np

/** _GNU_SOURCE for pthread_setname_np() */
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <sys/prctl.h>
#include <fcntl.h>

#include <tinylogger.h>
#include "demo-utils.h"

#define N_THREADS 5

#define NAME_LEN TASK_COMM_LEN
static char thread_names[N_THREADS][NAME_LEN];

/**
 * @fn void *threadFunc(void *parm)
 * @brief A thread that periodically emits log messages
 * @param parm a buffer to share parameters between the main thread and the
 * child
 * @return NULL
 */
static void *threadFunc(void *parm) {
	pthread_t thread = pthread_self();
	//pthread_id_np_t tid_np = pthread_gettreadid_np();
	//pid_t tid = gettid();
	pid_t tid = syscall(__NR_gettid);	// or SYS_gettid

	int rc;

	char *name;

	if (parm != NULL) {
		name = parm;
	} else {
		name = "oops";
	}
	log_info("setting thread name to %s (%d) (%ld)", name, tid, thread);
	rc = pthread_setname_np(thread, parm);
	if (rc != 0)
		errExitEN(rc, "pthread getname");

	usleep(1000);

	for (int n = 0; n < 5; n++) {
		log_info("hello from %s (%d)", name, tid);
		sleep(2);
	}
	sleep(2);
	return NULL;
}

/**
 * @fn int main(void)
 *
 * @brief There are a few log formats that display thread id and/or name.
 *
 * Thread id formats use the linux thread id, not the posix pthread_t.
 *
 * The linux thread id is used instead of the posix pthread_t because it is
 * more visible from outside the process. The ps command referred to below may
 * be used to get thread status. The command expected is the command basename,
 * possibly truncated to 15 bytes.
 *
 * get_proc_comm() is an example routine in demo-utils.c that gets the proper name.
 *
 * Try renaming the thread binary to thread-verylongcommandname and running
 * that. The proper command to use will be displayed.
 *
 * This demo is best run in two command windows.
 *
 * Observe that the thread names and ids in the messages match those added by
 * the message formatter and those produced by the ps command.
 *
 * Thread info for child threads is also available through the /proc/$(PID)/task
 * subdirectory. 
 * 
 * @return 0 on success
 */
int main(void) {
	pthread_t threads[N_THREADS];
	int rc;
	char command[128];

	// get the comm name to match "ps H -C comm ..."
	// remember to free it...
	char *proc_comm = get_proc_comm();

	LOG_CHANNEL *ch1 = log_open_channel_s(stderr, LL_INFO, log_fmt_debug_tall);

	if (ch1 == NULL) {
		fprintf(stderr, "error opening channel\n");
		exit(EXIT_FAILURE);
	}

	snprintf(command, sizeof(command), "ps H -C %s -o 'pid tid cmd comm'" , proc_comm);

	for (int n = 0; n < N_THREADS; n++) {
		snprintf(thread_names[n], NAME_LEN, "thread_%d", n);

		rc = pthread_create(&threads[n], NULL, threadFunc, thread_names[n]);
		if (rc != 0)
			errExitEN(rc, "pthread create");
	}

	// the threads run for 10 seconds and sleep for 2 seconds
	// catch them while they are still running
	sleep(10);
	printf("output of %s\n", command);
	system(command);

	// join all threads to reclaim memory
	for (int n = 0; n < N_THREADS; n++) {
		pthread_join(threads[n], NULL);
	}

	// flush and close all channels and reclaim any memory
	log_done();

	free(proc_comm);

	return EXIT_SUCCESS;
}
