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

#define LOG_FILE "/tmp/hive.log"
//#define LOG_FILE "/dev/null"

#define N_THREADS	250
#define N_LOOPS		4000
#define DO_CHECK	true

struct thread_info {
	char	name[NAME_LEN];
	pthread_t	thread_id;
	int			tid;
	int			count;
};
struct thread_info threads[N_THREADS];

//static char thread_names[N_THREADS][NAME_LEN];

#define errExitEN(en, msg) \
	do { errno = en; perror(msg); exit(EXIT_FAILURE); \
	} while (0)

static void *threadFunc(void *parm) {
	unsigned int seed;
	pthread_t thread = pthread_self();
	//pthread_id_np_t tid_np = pthread_gettreadid_np();
	//pid_t tid = gettid();
	//pid_t tid = syscall(__NR_gettid);
	pid_t tid = syscall(SYS_gettid);

	struct thread_info *thread_info;

	char thread_name[NAME_LEN];
	int rc;

	if (parm != NULL) {
		thread_info = (struct thread_info *) parm;
	} else {
		fprintf(stderr, "threadFunc didn't get a param pointer: tid = %d", tid);
		exit(EXIT_FAILURE);
	}

	thread_info->tid = tid;

	rc = pthread_setname_np(thread, parm);
	if (rc != 0)
		errExitEN(rc, "pthread getname");


	rc = pthread_getname_np(thread, thread_name, sizeof(thread_name));
	if (rc != 0)
		errExitEN(rc, "pthread getname");

	seed = tid;

	log_info("hello from %s (%d)", thread_name, tid);

	for (int n = 0; n < thread_info->count; n++) {
		int x = rand_r(&seed);
		double X = (double) x / (double) RAND_MAX;

		x = (int) ((double) 1000) * X;

		log_info("sleeping %d microseconds, s/n=%d, tid=%d",
			x, n, thread_info->tid);
		usleep(x);
	}

	return NULL;
}

bool check_hello(char *line) {
	char date[32];
	char time[32];
	char level[16];
	char thread[32];
	char thread_name[32];
	int tid;
	int n_converted;

	n_converted = sscanf(line, "%s %s %s %s hello from %s (%d)",
		date, time, level, thread,
		thread_name, &tid);
	
	return n_converted == 6;
}

bool check_thread(char *line) {
	char date[32];
	char time[32];
	char level[16];
	char thread[32];
	int sleep;
	int sn;
	int tid;
	int n_converted;

	n_converted = sscanf(line, "%s %s %s %s sleeping %d microseconds, s/n=%d, tid=%d",
		date, time, level, thread,
		&sleep, &sn, &tid);
	
	return n_converted == 7;
}

bool check_wait(char *line) {
	char date[32];
	char time[32];
	char level[16];
	char thread[32];
	char thread_name[32];
	int tid;
	int n_converted;

	n_converted = sscanf(line, "%s %s %s %s waiting for thread %s tid = %d",
		date, time, level, thread,
		thread_name, &tid);
	
	return n_converted == 6;
}

void scan_file(char *pathname) {
	int	lines = 0;
	int hellos = 0;
	int threads = 0;
	int waits = 0;
	char buf[BUFSIZ];
	FILE *file = fopen(pathname, "r");

	if (file == NULL) {
		fprintf(stderr, "can't open %s for reading\n", pathname);
		exit(EXIT_FAILURE);
	}

	while (fgets(buf, sizeof(buf), file) != NULL) {
		if (check_thread(buf)) {
			threads++;
		} else if (check_hello(buf)) {
			hellos++;
		} else if (check_wait(buf)) {
			waits++;
		} else {
			printf("don't recognize line %d\n%s\n", lines, buf);
		}
			
		lines++;
	}

	printf("%d lines read, %d hellos, %d threads, %d waits\n",
		lines, hellos, threads, waits);

	fclose(file);

	return;
}


int main(void) {
	int rc;

	LOG_CHANNEL ch2 = log_open_channel_f(LOG_FILE, LL_INFO, log_fmt_tall, true);
	if (ch2 == NULL) {
		fprintf(stderr, "problem opening %s for appending\n", LOG_FILE);
		exit(EXIT_FAILURE);
	}
	(void) ch2;

	if (ch2 == NULL) {
		fprintf(stderr, "error opening channel\n");
		exit(EXIT_FAILURE);
	}

	for (int n = 0; n < N_THREADS; n++) {
		threads[n].count = N_LOOPS;
		snprintf(threads[n].name, NAME_LEN, "thread_%d", n);

		rc = pthread_create(&threads[n].thread_id, NULL, threadFunc, threads[n].name);
		if (rc != 0)
			errExitEN(rc, "pthread create");
	}

	for (int n = 0; n < N_THREADS; n++) {
		log_info("waiting for thread %s tid = %d", threads[n].name, threads[n].tid);
		pthread_join(threads[n].thread_id, NULL);
	}

	log_done();

	printf("%d threads, %d records per thread = %d records\n",
		N_THREADS, N_LOOPS, N_THREADS * N_LOOPS);

	if (DO_CHECK) {
		scan_file(LOG_FILE);
	}

	return 0;
}
