/** _GNU_SOURCE for pthread_setname_np */
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/syscall.h>

#include <tinylogger.h>
#include "demo-utils.h"

/** from kernel/sched.h */
#define TASK_COMM_LEN 16
/** includes null termination */
#define NAME_LEN TASK_COMM_LEN

#define N_THREADS		250		/**< number of threads to run */
#define N_LOOPS			1000	/**< number of loops for each thread to run */
#define SLEEP_MICROS	50		/**< maximum sleep duration for each loop */
#define FORMAT			log_fmt_tall	/**< message format to use */

/**
 * bookkeeping for the threads
 */
struct thread_info {
	char	name[NAME_LEN];			/**< the thread name */
	useconds_t	sleep_time;			/**< the maximum time to sleep (uS) */
	pthread_t	thread_id;			/**< the posix thread_id */
	int			tid;				/**< the linux thread id */
	int			count;				/**< the number of loops to do */
	int			sequence_number;	/**< for verifying the thread message
										 sequencing */
};
static struct thread_info threads[N_THREADS];

/**
 * @fn int get_index(int)
 * @brief search the thread list for a matching tid
 * @param tid the id of the thread to search for
 * @param n_threads the number of threads started
 * @return the index of the matching entry, -1 if no match
 */
static int get_index(int tid, int n_threads) {
	for (int n = 0; n < n_threads; n++) {
		if (threads[n].tid == tid) {
			return n;
		}
	}
	return -1;
}

/**
 * @fn threadFunc(void *)
 *
 * @brief A thread that emits a number of log messages
 *
 * The thread will emit the number of messages specified by thread_info->count.
 * It first logs a "hello" message containing the thread name and thread id.
 * Then it logs a message announcing how long it will sleep, and then sleeps.
 *
 * @param param the thread_info struct containing the actual parameters
 */
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

	rc = pthread_setname_np(thread, thread_info->name);
	if (rc != 0)
		errExitEN(rc, "pthread getname");


	rc = pthread_getname_np(thread, thread_name, sizeof(thread_name));
	if (rc != 0)
		errExitEN(rc, "pthread getname");

	seed = tid;

	log_info("hello from %s (%d)", thread_name, tid);

	for (int n = 0; n < thread_info->count; n++) {
		useconds_t x = rand_r(&seed);
		double X = (double) x / (double) RAND_MAX;

		x = (useconds_t) ((double) thread_info->sleep_time) * X;

		log_info("sleeping %d microseconds, s/n=%d, tid=%d",
			x, n, thread_info->tid);
		usleep(x);
	}

	return NULL;
}

/**
 * @fn bool check_hello(char *)
 *
 * @brief Check if the message is a valid hello message
 *
 * @return true if the message is parsed correctly
 */
static bool check_hello(char *line) {
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

/**
 * @fn bool check_thread(char *)
 *
 * @brief Check if the message is a valid "sleeping" message from the threads.
 *
 * Each thread loops printing a message that it will sleep for a certain
 * period. If the date, time, level and thread name fields, and the sleep,
 * serial number, and thread id integer fields can be parsed, it is considered
 * a valid thread "sleeping" message.
 * 
 * Verify that the sequence numbers embedded in the message are valid. They
 * are checked at the end of the verification process.
 *
 * @param line the line to inspect
 * @param n_threads the number of threads started
 * @return true if all fields can be parsed
 */
bool check_thread(char *line, int n_threads) {
	char date[32];
	char time[32];
	char level[16];
	char thread[32];
	int sleep;
	int sn;
	int tid;
	int n_converted;

	n_converted = sscanf(line,
		"%s %s %s %s sleeping %d microseconds, s/n=%d, tid=%d",
		date, time, level, thread,
		&sleep, &sn, &tid);

	if (n_converted != 7) return false;
	
	int index = get_index(tid, n_threads);
	if (index >= 0) {
		// sequence_number is the number expected
		// if it is correct, increment it for the next expected number
		if (threads[index].sequence_number == sn) {
			threads[index].sequence_number++;
		}
	}
	
	return n_converted == 7;
}

/**
 * @fn bool check_wait(char *)
 *
 * @brief Check the final message from main waiting for each thread
 *
 * Check that the date, time, level, thread and thread_name strings and
 * the thread number can be parsed. If successful, it is assumed to be a
 * valid "wait message".
 *
 * @return true if all of those fields can be parsed.
 */
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

/**
 * @fn void scan_file(char *, int, int)

 * @brief Check that the file produced is correct.
 *
 * Scan the output file and count:
 * hellos: each thread produces an hello message on startup
 * threads: each thread will produce the requested number of messages
 * waits: after starting the threads, main waits for each one to exit
 *
 * @param pathname the name of the file to scan
 * @param n_threads the number of threads expected
 * @param n_loops the number of loops each thread executed
 *
 * @return true on success
 */
bool scan_file(char *pathname, int n_threads, int n_loops) {
	int	total_lines = 0;
	int n_hellos = 0;
	int n_sleeping = 0;
	int n_waits = 0;
	char buf[BUFSIZ];
	bool success = true;

	FILE *file = fopen(pathname, "r");

	if (file == NULL) {
		fprintf(stderr, "can't open %s for reading\n", pathname);
		exit(EXIT_FAILURE);
	}

	// examine each message, and count the number of each type
	while (fgets(buf, sizeof(buf), file) != NULL) {
		if (check_thread(buf, n_threads)) {
			n_sleeping++;
		} else if (check_hello(buf)) {
			n_hellos++;
		} else if (check_wait(buf)) {
			n_waits++;
		} else {
			printf("don't recognize line %d\n%s\n", total_lines, buf);
		}
			
		total_lines++;
	}

	fclose(file);

	// make sure the sequence numbers for each thread's messages are correct
	for (int n = 0; n < n_threads; n++) {
		if (threads[n].sequence_number != n_loops) {
			printf("sequence error on thread %d: found %d\n",
				threads[n].tid, threads[n].sequence_number);
			success = false;
		}
	}

	if (n_hellos != n_threads) success = false;
	if (n_waits != n_threads) success = false;

	// print the totals so that they can be compared to the expected numbers
	printf("%d records read, %d hellos, %d sleeping messages, %d waits\n",
		total_lines, n_hellos, n_sleeping, n_waits);
	

	return success;
}


/**
 * @fn int main(int argc, char *argv[])
 *
 * @brief Generate a log file in log_fmt_tall, log_fmt_json or log_fmt_xml
 * format.
 *
 * Multiple threads are started, and each produces the same number of messages.
 * The resulting file may optionally be verified if it is in the log_fmt_tall
 * format.
 *
 * @return 0 on success
 */
int main(int argc, char *argv[]) {
	bool do_verify = false;
	log_formatter_t formatter = log_fmt_tall;
	int rc;
	char *filename = "beehive.log";
	int n_threads = N_THREADS;
	int n_loops = N_LOOPS;
	useconds_t sleep_time = SLEEP_MICROS;

	// test if user requested verification
	for (int n = 1; n < argc; n++) {
		if ((strcmp(argv[n], "-v") == 0)) {
			do_verify = true;
		} else if (strcmp(argv[n], "-q") == 0) {
			n_threads /= 10;
			n_loops /= 10;
			sleep_time /= 10;
		} else if (strcmp(argv[n], "-z") == 0) {
			sleep_time = 0;
		} else if (strcmp(argv[n], "-j") == 0) {
			formatter = log_fmt_json;
			filename = "beehive.json";
		} else if (strcmp(argv[n], "-x") == 0) {
			formatter = log_fmt_xml;
			filename = "beehive.xml";
		} else {
			fprintf(stderr, "usage: %s [-v] [-q] [-j] [-x]\n", argv[0]);
			fprintf(stderr, "  -v selects verification (not available "
				"for json or xml)\n");
			fprintf(stderr, "  -q selects quick mode "
				"(1/10 the normal threads and messages per thread)\n");
			fprintf(stderr, "  -z sets 0 intermessage sleep time\n");
			fprintf(stderr, "  -j selects json format\n");
			fprintf(stderr, "  -x selects xml format\n");
			exit(EXIT_FAILURE);
		}
	}

	// verify requires log_fmt_tall and a fresh file (no append)
	// start with a fresh file for all output formats
	unlink(filename);

	// don't use line buffering for bulk output
	// speeds thing up a little
	LOG_CHANNEL *ch = log_open_channel_f(filename, LL_INFO, formatter, false);
	if (ch == NULL) {
		fprintf(stderr, "problem opening %s for appending\n", filename);
		exit(EXIT_FAILURE);
	}

	// start the threads
	for (int n = 0; n < n_threads; n++) {
		threads[n].count = n_loops;
		threads[n].sleep_time = sleep_time;
		snprintf(threads[n].name, NAME_LEN, "thread_%d", n);

		// initialize sequence number for verification phase
		threads[n].sequence_number = 0;

		rc = pthread_create(&threads[n].thread_id, NULL,
			threadFunc, threads[n].name);
		if (rc != 0)
			errExitEN(rc, "pthread create");
	}

	// wait for the threads to complete
	for (int n = 0; n < n_threads; n++) {
		log_info("waiting for thread %s tid = %d",
			threads[n].name, threads[n].tid);
		pthread_join(threads[n].thread_id, NULL);
	}

	// flush and close the output file
	log_done();

	// scanners are expecting log_fmt_tall
	// xml and json are verified by external programs
	if ((formatter == log_fmt_tall) && do_verify) {
		printf("expecting %d threads, %d + 2 "
			"records per thread = %d total records\n",
		n_threads, n_loops, n_threads * (n_loops + 2));
		bool success = scan_file(filename, n_threads, n_loops);
		printf("Verify %s\n", success ? "succeeded" : "failed");
		if (!success) {
			exit(EXIT_FAILURE);
		}
	}

	return EXIT_SUCCESS;
}
