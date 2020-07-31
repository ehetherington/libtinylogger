#ifndef _DEMO_UTILS_H
#define _DEMO_UTILS_H 1

// from kernel/sched.h
#define TASK_COMM_LEN 16	// includes null termination

/*
 * print an error message and exit
 */
#define errExitEN(en, msg) \
	do { errno = en; perror(msg); exit(EXIT_FAILURE); \
	} while (0)


void check_append(char *filename);
char *get_proc_comm(void);

#endif
