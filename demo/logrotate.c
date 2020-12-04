/** _GNU_SOURCE for asprintf() */
#define _GNU_SOURCE

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/syscall.h>

#include <tinylogger.h>
#include "demo-utils.h"

/*
 * resources to free on Ctl-C
 */
static char *pscmd = NULL;
static char *lscmd = NULL;
static char *killcmd = NULL;

/**
 * @fn static void free_bufs(void)
 * @brief free buffers
 */
static void free_bufs(void) {
	if (lscmd != NULL) free(lscmd);
	if (pscmd != NULL) free(pscmd);
	if (killcmd != NULL) free(killcmd);
}

/**
 * @fn void inthandler(int sig)
 *
 * @brief Ctl-C handler to cleanly close log
 *
 * Not ideal, but demonstrates a Ctl-C SIGINT handler flushing the buffered
 * output by calling log_done().
 */
static void inthandler(int sig) {
	(void) sig;	// suppress "unused" warning
	printf("\nSIGINT caught\n");
	log_done();
	printf("logs closed\n");
	free_bufs();
	exit(EXIT_SUCCESS);
}

/**
 * @fn void *threadFunc(void *parm)
 * @brief A thread that periodically emits log messages
 * @param parm a buffer to share parameters between the main thread and the
 * child
 * @return NULL
 */
// These constants and the loops in the main program were adjusted to
// illustrate certain aspects of the example.
#define N_MSGS			(500)
#define SLEEP_MICROS	(100 * 1000)
#define ONE_SECOND		(1000 * 1000)

struct thread_params {
	char *name;
	int sleep_micros;
} thread_params;

static void *threadFunc(void *param) {
	pthread_t thread = pthread_self();
	pid_t tid = syscall(__NR_gettid);	// or SYS_gettid
	int	msg_sn = 1;

	struct thread_params *thread_params;
	if (param == NULL) exit(EXIT_FAILURE);
	thread_params = (struct thread_params *) param;


	int rc;

	log_info("setting thread name to %s (%d) (%ld)",
		thread_params->name, tid, thread);
	rc = pthread_setname_np(thread, thread_params->name);
	if (rc != 0)
		errExitEN(rc, "pthread setting name");

	for (int n = 0; n < N_MSGS; n++) {
		log_info("hello from %s (%d) message number %d",
			thread_params->name, tid, msg_sn++);
		usleep(thread_params->sleep_micros);
	}
	return NULL;
}


static char *filename = "logrotate.log";
static void print_help(char *progname) {
	fprintf(stderr, "usage: %s [-h] [-p] [-j] [-x]\n", progname);
	fprintf(stderr, "  -p selects programmatic logrotate\n");
	fprintf(stderr, "     (default is to simulating actual logrotate\n");
	fprintf(stderr, "  -x selects xml format\n");
	fprintf(stderr, "  -j selects json format\n");
	fprintf(stderr, "  -q selects \"quick\" mode for testing\n");
	fprintf(stderr, "  -h prints this help and exits\n");
}

int main(int argc, char **argv) {
	pthread_t worker;
	int status;
	pid_t my_pid;
	char *proc_comm;
	LOG_CHANNEL *ch1;
	char buf[FILENAME_MAX];
	int one_second = ONE_SECOND;

	thread_params.name = "worker";
	thread_params.sleep_micros = SLEEP_MICROS;

	log_formatter_t formatter = log_fmt_debug_tall;
	bool programmatic = false;

	// process command line options
	for (int n = 1; n < argc; n++) {
		if ((strcmp(argv[n], "-p") == 0)) {
			programmatic = true;
		} else if (strcmp(argv[n], "-j") == 0) {
			formatter = log_fmt_json;
			filename = "logrotate.json";
		} else if (strcmp(argv[n], "-x") == 0) {
			formatter = log_fmt_xml;
			filename = "logrotate.xml";
		} else if (strcmp(argv[n], "-q") == 0) {
			thread_params.sleep_micros /= 10;
			one_second /= 10;
		} else if (strcmp(argv[n], "-h") == 0) {
			print_help(argv[0]);
			exit(EXIT_SUCCESS);
		} else {
			print_help(argv[0]);
			exit(EXIT_FAILURE);
		}
	}

	/*
	 * start with a clean slate
	 */
	remove_or_exit(filename);
	snprintf(buf, sizeof(buf), "%s.rotated", filename);
	remove_or_exit(buf);

	/*
	 * get the pid of this process - needed for the kill command
	 */
	my_pid = getpid();

	/*
	 * get the process command string needed by the magic ps command
	 * See the threads.c example.
	 */
	proc_comm = get_proc_comm();

	/*
	 * create the command to display threads
	 */
	status = asprintf(&pscmd, "ps H -C %s -o 'pid tid cmd comm'\n", proc_comm);
	if (status == -1) exit(EXIT_FAILURE);

	/*
	 * create the command to monitor file sizes
	 */
	status = asprintf(&lscmd, "/bin/ls -l %s*", filename);
	if (status == -1) exit(EXIT_FAILURE);

	/*
	 * create the command to signal the  lograte thread
	 */
	status = asprintf(&killcmd, "kill -USR1 %d", my_pid);
	if (status == -1) exit(EXIT_FAILURE);

	/*
	 * Set up for Ctl-c handling to clean up before the program is done.
	 */
	if (signal(SIGINT, inthandler) == SIG_ERR) {
		perror("setting INT handler\n");
		fprintf(stderr, "error setting INT handler\n");
		exit(EXIT_FAILURE);
	}

	/*
	 * If we are going to use JSON, set a header "note". The JSON format
	 * is the only one that uses it.
	 *
	 * log_set_json_notes() only has an effect if the library was compiled with
	 * ENABLE_JSON_HEADER. ("notes" is a field in the header)
	 */

	if (formatter == log_fmt_json) {
		log_set_json_notes(
			"This is the first log. Its first record is sequence 1");
	}

	/*
	 * Set output to a file and use log_fmt_standard
	 * set the output file to filename
	 * set log level to LL_INFO
	 * set format to standard
	 * turn off line buffering
	 */
	ch1 = log_open_channel_f(filename, LL_INFO, formatter, false);
	(void) ch1;

	/*
	 * just the main thread
	 */
	printf("==== Just the main process/thread\n");
	system(pscmd);

	if (!programmatic) {
		/*
		 * enable log rotate signal
		 */
		log_enable_logrotate(SIGUSR1);

		/*
		 * the main thread and the log_sighandler thread
		 */
		printf("==== The main process/thread plus the log_sighandler thread\n");
		system(pscmd);

		/*
		 * Normal output buffering is used. Output will be buffered, and only
		 * written when the output buffer (4096) gets filled. Using tail -f will
		 * show the output being written in 4k chunks, with no relation to
		 * newlines.
		 */
		printf("use the following command to send a signal "
			"to \"rotate\" the log file\n%s\n", killcmd);
	}

	/*
	 * start a thread to create log messages
	 */
	status = pthread_create(&worker, NULL, threadFunc, &thread_params);
	if (status != 0)
		errExitEN(status, "pthread create");

	/*
	 * the main thread, the log_sighandler thread and a worker
	 */
	printf("==== The worker thread has been added\n");
	system(pscmd);

	/*
	 * watch the file grow in 4k chunks
	 */
	printf("==== Watch the file grow in chunks which are mutliples of 4k\n");

	for (int n = 0; n < 20; n++) {
		if ((n % 5) == 0) {
			system(lscmd);
		}
		usleep(one_second);
	}


	/*
	 * Simulate a logrotate by renaming the file, and then sending the
	 * logrotate signal.
	 */
	snprintf(buf, sizeof(buf), "%s.rotated", filename);
	rename(filename, buf);
	printf("===== %s renamed to %s\n", filename, buf);
	printf("===== it will continue to grow until the channel is re-opened\n");
	for (int n = 0; n < 5; n++) {
		system(lscmd);
		usleep(one_second);
	}

	/*
	 * If we are going to use JSON, set a header "note". The JSON format
	 * is the only one that uses it.
	 * If we don't change the "notes", it will use what was previously set.
	 */
	if (formatter == log_fmt_json) {
		log_set_json_notes(
			"This is the second log. Its first record is sequence 1 also");
	}

	/*
	 * send a signal to the logrotate thread or
	 * directly re-open the channel
	 */
	if (programmatic) {
		log_reopen_channel(ch1);
		printf("===== channel re-opened\n");
	} else {
		system(killcmd);
		printf("===== logrotate signal sent\n");
	}

	/*
	 * monitor the "new" log file growing
	 */
	for (int n = 0; n < 10; n++) {
		if ((n % 5) == 0) {
			system(lscmd);
		}
		usleep(one_second);
	}

	/*
	 * wait for the worker thread
	 */
	printf("===== the worker thread is still running\n");
	system(pscmd);

	/*
	 * continue monitoring the "new" log file grow
	 */
	for (int n = 0; n < 10; n++) {
		if ((n % 5) == 0) {
			system(lscmd);
		}
		usleep(one_second);
	}

	printf("===== wait until the worker thread is done\n");
	pthread_join(worker, NULL);
	printf("===== the worker thread is done\n");
	system(pscmd);
	system(lscmd);

	/*
	 * Stop the logrotate thread (if it was started)
	 * Flush and close the log file
	 */
	log_done();
	printf("===== log_done() called\n");
	system(pscmd);
	free_bufs();
	free(proc_comm);

	exit(EXIT_SUCCESS);
}

