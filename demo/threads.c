// https://stackoverflow.com/questions/21091000/how-to-get-thread-id-of-a-pthread-in-linux-c-program
// man pthread_setname_np
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/syscall.h>

#include <tinylogger.h>

// from kernel/sched.h
#define TASK_COMM_LEN 16
#define NAME_LEN TASK_COMM_LEN	// includes null termination

#define N_THREADS 5

static char thread_names[N_THREADS][NAME_LEN];

#define errExitEN(en, msg) \
	do { errno = en; perror(msg); exit(EXIT_FAILURE); \
	} while (0)

static void *threadFunc(void *parm) {
	pthread_t thread = pthread_self();
	//pthread_id_np_t tid_np = pthread_gettreadid_np();
	//pid_t tid = gettid();
	pid_t tid = syscall(__NR_gettid);

	char thread_name[NAME_LEN];
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

	rc = pthread_getname_np(thread, thread_name, sizeof(thread_name));
	if (rc != 0)
		errExitEN(rc, "pthread getname");

	for (int n = 0; n < 10; n++) {
		log_info("hello from %s (%d)", thread_name, tid);
		sleep(1);
	}
	return NULL;
}

int main(void) {
	pthread_t thread;
	int rc;
	char thread_name[NAME_LEN];

	LOG_CHANNEL *ch1 = log_open_channel_s(stderr, LL_INFO, log_fmt_debug_tall);

	if (ch1 == NULL) {
		fprintf(stderr, "error opening channel\n");
		exit(EXIT_FAILURE);
	}

	for (int n = 0; n < N_THREADS; n++) {
		snprintf(thread_names[n], NAME_LEN, "thread_%d", n);

		rc = pthread_create(&thread, NULL, threadFunc, thread_names[n]);
		if (rc != 0)
			errExitEN(rc, "pthread create");

		rc = pthread_getname_np(thread, thread_name, sizeof(thread_name));
		if (rc != 0)
			errExitEN(rc, "pthread getname");
	}

	log_info("use the following command: %s", "ps H -C threads -o 'pid tid cmd comm'");

	sleep(10);

	// TODO: wait for the threads to finish! (see beehive.c)
	log_done();

	return 0;
}
