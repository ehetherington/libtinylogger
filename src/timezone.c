/*
 * (C) 2020 Edward Hetherington
 * This code is licensed under MIT license (see LICENSE in top dir for details)
 */

/** @file       timezone.c
 *  @brief      Find the Olson timezone.
 *  @details
 *
 *  In order to find an Olson timezone that matches the timezone in use, the
 *  behaviour of tzset(3) is mimicked. The process's environment is inspected
 *  first, then the system's default timezone.
 *
 *  If the TZ environment variable is set, then it looks in the zoneinfo
 *  directory for a matching zoneinfo file. If a matching zoneinfo file is
 *  found, it is used. If no matching file is found, the search ends.
 *
 *  If the TZ environment variable is NOT set, then it looks for the system
 *  default timezone. First it looks for an /etc/timezone file. If it exists.
 *  the contents of that file is returned. If there is no /etc/timezone file,
 *  the /etc/localtime file is inspected. If it is a symbolic link that
 *  contains a timezone string that can be found in the zoneinfo database, it
 *  is returned.
 *
 *  While considering the TZ environment variable or the /etc/localtime file,
 *  if the TZDIR environment variable is set, it is overrides the system
 *  zoneinfo directory, as in tzset(3).
 *
 *  @author     Edward Hetherington
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <limits.h>
#include <fcntl.h>

/* for dirname(3) */
#include <libgen.h>

#include <sys/types.h>
#include <sys/stat.h>

#include "config.h"

#include "private.h"

/** the system default timezone file (Olson plain text) */
#define TIMEZONE_FILE "/etc/timezone"

/** the system default localtime file (zoneinfo file or link to it) */
#define LOCALTIME_FILE "/etc/localtime"

/** where the zoneinfo database is installed */
/**
 * To override:
 *     - quick-start: set in quick-start/config.h
 *     - autoconf: $ ZONEINFO_DIR="/my/alternate/database" ./configure
 */
#ifndef ZONEINFO_DIR
#define ZONEINFO_DIR "/usr/share/zoneinfo"
#endif

/** zoneinfo files begin with "TZif" */
#define TZ_MAGIC "TZif"

/**
 * @fn bool is_zoneinfo_file(char const * const path)
 * @brief Verify that a file is actually a zoneinfo file.
 * @param path the full pathname of the file in question.
 * @return true if the TZ_MAGIC string is found as the first four characters
 */
static bool is_zoneinfo_file(char const * const path) {
	char magic_buf[5] = {0};	// null terminate
	int status;

	// read first 4 bytes of the file
	FILE *file = fopen(path, "r");
	if (file == NULL) return false;
	status = fread(magic_buf, 1, 4, file);
	fclose(file);
	if (status != 4) return false;

	// check the magic string
	status = strncmp(TZ_MAGIC, magic_buf, 4);
	
	return (status == 0) ? true : false;
}

/**
 * @fn bool check_zoneinfo_dir(char const * const path)
 * @brief   Check zoneinfo directory from environment (TZDIR) or
 *          the ZONEINFO_DIR macro.
 * @details It is verified that the path exists and is a directory, A trailing
 *          '/' (if necessary) is added for use as a "prefix" for further
 *          processing.
 *
 *          Memory for the buffer is obtained with calloc(3), and should be
 *          freed with free(3).
 * @param   path the pathname to trim
 * @return  A possibly modified copy of path, or NULL if it doesn't exist or is
 *          not a directory.
 */
static char *check_zoneinfo_dir(char const * const path) {
	struct stat sb;

	if ((path == NULL) || (path[0] == 0) || (path[0] != '/')) return NULL;

	// allow space to append a '/' if necessary
	char *buf = calloc(strlen(path) + 2, sizeof(char));
	strcpy(buf, path);

	// trim trailing slashes
	char *last_char = buf + strlen(buf) - 1;
	while ((*last_char == '/') && (last_char - buf) > 0) {
		*last_char-- = '\0';
	}

	// append one '/' character
	if (buf[strlen(buf) - 1] != '/') buf[strlen(buf)] = '/';

	// make sure it exists and is a directory
	if ((stat(buf, &sb) != 0) || ! S_ISDIR(sb.st_mode)) {
		return NULL;
	}

	return buf;
}

/**
 * @fn char *get_zoneinfo_dir(void)
 * @brief Look for TZDIR in the environment first, otherwise use the system
 * zoneinfo directory.
 *
 * @details The string found in the environment TZDIR variable and the ZONEINFO
 * macro must begin with '/'. Since they are always used to prepend to the
 * candidate Olson timezone, a trailing '/' is appended.
 *
 * It is verified that the candidate directory is actually a directory.
 *
 * @return A dynamically allocated string that starts and ends with '/' if
 * successful, NULL otherwise. The returned buffer must be free(3)'d.
 */
static char *get_zoneinfo_dir(void) {
	char *env_tz;
	char *zoneinfo_dir;

	if ((env_tz = getenv("TZDIR")) != NULL) {
		// zoneinfo database from environment (gnu extension)
		zoneinfo_dir = check_zoneinfo_dir(env_tz);
	} else {
		// system zoneinfo database
		zoneinfo_dir = check_zoneinfo_dir(ZONEINFO_DIR);
	}

	return zoneinfo_dir;
}

/**
 * @fn ino_t get_inode(char const * const pathname)
 * @brief return the inode of the specified pathname.
 * @param pathname the pathname of the file to check
 * @return inode on success, 0 on failure
 */
static ino_t get_inode(char const * const pathname) {
	struct stat sb;
	if (stat(pathname, &sb) == -1) return 0;
	return sb.st_ino;
}


/* include linked list function */
#ifndef DOXYGEN_SHOULD_SKIP_THIS
#define LL_PUSH_POP_TYPE LL_PUSH_POP_LIFO
#endif
#include "linked-list.c"

/**
 * @fn push_components(DLL_LIST *list, char *path)
 * @brief separate the path components and push them onto the list
 * @details Tokens are extracted from the path by strtok(3) using "/" as the
 *          delimiter string. Tokens of "." are dropped. ".." tokens cause the
 *          previous token to be "pop()'d". Other tokens are "push()'d".
 * @param list the list to add the tokens to
 * @param path the pathname to tokenize and push to the list
 * @return 
 */
static void push_components(DLL_LIST *list, char *path) {
	char *token = strtok(path, "/");
	while (token != NULL) {
		if (strcmp(".", token) == 0) {
			;                  // drop
		} else if (strcmp("..", token) == 0) {
			pop(list);         // backup
		} else {
			push(list, token); // add
		}
		token = strtok(NULL, "/");
	}
}

/**
 * @fn char *log_eval_symlink(char const *const pathname)
 * @brief   Produce a compact pathname from a symbolic link.
 * @details Given the pathname of a symbolic link, evaluate it's contents.
 *          If it is an absolute path, return it. Otherwise, it is relative
 *          to directory of the symbolic link. Combine the directory of the
 *          symbolic link and the contents it, and reduce "dir/../" and other
 *          excess components to produce the resulting pathname.
 * @param   pathname the pathname of the symbolic link
 * @return  the resulting pathname, or NULL if the pathname doesn't point to
 *          symbolic link.
 */
static char *log_eval_symlink(char const * const pathname) {
	DLL_LIST list = {0};
	struct stat sb;
	int link_len;
	char link_contents[PATH_MAX] = {0};

    // Make sure the path actually is a symbolic link.
    // We can't get the Olson timezone from an actual zoneinfo file or hard
    // link. We need a symbolic link that contains the timezone to extract the
    // timezone name.
	// (the lstat(2) is not necessary, as the following readlink(2) will pick up
	// a path that doesn't reference a symlink by failing also).
    if ((lstat(pathname, &sb) != 0) || ! S_ISLNK(sb.st_mode)) return NULL;

	// read the symbolic link
	link_len = readlink(pathname, link_contents, sizeof(link_contents) - 1);
	if (link_len == -1) return NULL;

	// if the symbolic link contents is absolute, we are done.
	if (link_contents[0] == '/') return strdup(link_contents);

	// dirname(3) modifies its argument - make a copy
	char *tmp_pathname = strdup(pathname);
	char *link_dir = dirname(tmp_pathname);

	// combine link_dir with link_contents and remove unnecessary components
	push_components(&list, link_dir);
	push_components(&list, link_contents);

	// construct revised path
	char realpath[PATH_MAX] = {0};

	// retrieve components in FIFO order
	while (! is_empty(&list)) {
		snprintf(realpath + strlen(realpath),
			sizeof(realpath) - strlen(realpath),
			"/%s", delete_tail(&list));
	}

	// contents of tmp_pathname have been copied to realpath
	free(tmp_pathname);

	return strdup(realpath);
}

/**
 * @fn char *parse_localtime_link(char * const, size_t const,
 *                                       char const * const)
 * @brief See if /etc/localtime is a symbolic link that can be parsed for the
 *        timezone.
 *
 * /etc/localtime may be a symbolic link to the actual timezone file.
 * Using `$ readlink /etc/timezone`, if the link includes the timezone, the
 * timezone may be extracted from it.
 * 
 * Examples are:
 *
 * (on RHEL) (relative):
 * "../usr/share/zoneinfo/America/New_York", where "America/New_York" is the TZ
 *
 * Or, on Raspbian (Debian base) (absolute):
 * "/usr/share/zoneinfo/America/New_York"
 *
 * @param buf          a buffer to return the embedded timezone
 * @param buf_len      the length of that buffer
 * @param zoneinfo_dir the directory of the Olson timezone database
 * @return             the address of buf on success, NULL otherwise
 */
static char *parse_localtime_link(char * const buf, size_t const buf_len,
	char const * const zoneinfo_dir) {
	struct stat sb;
	ino_t link_inode;
	char *result = NULL;	// expect failure

	if ((buf == NULL) || (buf_len < 1)) return NULL;

	// Make sure LOCALTIME_FILE exists and points to a zoneinfo file.
	if (!is_zoneinfo_file(LOCALTIME_FILE)) return NULL;

	// Remember the inode of that file to verify the symbolic link "editing"
	// worked.
	if ((link_inode = get_inode(LOCALTIME_FILE)) == 0) return NULL;

	// Make sure the path actually is a symbolic link.
	// We can't get the Olson timezone from an actual zoneinfo file or hard
	// link. We need a symbolic link that contains the timezone to extract the
	// timezone name.
	if ((lstat(LOCALTIME_FILE, &sb) != 0) || ! S_ISLNK(sb.st_mode)) return NULL;

	// evaluate the symlink
	char *symlink = log_eval_symlink(LOCALTIME_FILE);

	// see if it starts with the expected zoneinfo dir
	if (strstr(symlink, zoneinfo_dir) != symlink) goto freemem;

	// make sure it is the same as the original link
	if (link_inode != get_inode(symlink)) goto freemem;

	// make sure the path actually fits into the result buffer
	// leave room for the NUL termination
	if (1 + (strlen(symlink) - strlen(zoneinfo_dir)) >= buf_len) goto freemem;

	// success - copy it to the result buffer
	strncpy(buf, symlink + strlen(zoneinfo_dir), buf_len);
	result = buf;

freemem:
	free(symlink);

	return result;
}

/**
 * @fn char *log_read_timezone_file(char * const buf, size_t const buf_len)
 * @brief Read the system timezone file and return its contents.
 * @param buf a buffer to copy the file's contents
 * @param buf_len the length of that buffer
 * @return the supplied buffer on success, NULL on failure
 */
static char *log_read_timezone_file(char * const buf, size_t const buf_len) {
	size_t n_read;
	char *timezone = NULL;
	FILE *file;

	if ((buf == NULL) || (buf_len < 1)) return NULL;

	file = fopen(TIMEZONE_FILE, "r");
	if (file == NULL) return NULL;

	// turn off complaint about %ms being posix
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat"
	n_read = fscanf(file, "%ms", &timezone);
#pragma GCC diagnostic pop

	if (n_read == 1) {
		// make sure it fits in the buffer
		if (strlen(timezone) < buf_len) {
			strncpy(buf, timezone, buf_len);
		}
		free(timezone);
		return buf;
	}

	return NULL;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overflow"
#pragma GCC diagnostic ignored "-Wstringop-truncation"
/**
 * @fn char *check_env(char *buf, size_t const buf_len,
 *                         char const * env_tz, char const * const zoneinfo_dir)
 * @brief See if the TZ variable in the environment is set and usable.
 * @details The TZ variable in the environment may be in two different forms.
 *
 * 			Only Olson timezone names are supported.
 *
 *          The first form specifies the details of the timezone, and is not
 *          supported. The second form specifies a timezone file. Only files
 *          in the system timezone database directory are supported, as the
 *          names of those files are Olson timezones.
 *
 * (See the tzset(3) man page and
 * https://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html)
 *
 * @param buf buffer to place a discovered timezone
 * @param buf_len the length of that buffer
 * @param env_tz the timezone set in the environment
 * @param zoneinfo_dir the directory where the Olson timzone files can be found
 * @return the address of the timezone parameter on success
 */
static char *check_env(char *buf, size_t const buf_len,
	char const * env_tz, char const * const zoneinfo_dir) {
	char path[PATH_MAX];
	int	tz_len;

	if ((buf == NULL) || (buf_len < 1)) return NULL;

	if (env_tz == NULL) {
		return NULL;
	}

	tz_len = strlen(env_tz);

	// shortest timezones are GB and NZ
	if (tz_len < 2) return NULL;

	// Accept files relative to the system timezone database directory - the
	// names of the timezone files in that directory are the names of timezones!
	// Also accept absolute pathnames that start with the database directory.
	//
	// leading ':' indicates timezone file being specified
	//             Three possibilites
	//             A) Only ':' or what follows cannot be interpreted - use UTC.
	//             Reject, as no Olson timezone can be found.
	//             B) ':' followed by relative path. The path is taken relative
	//             to the system timezone directory. The relative filename may
	//             contain the Olson timezone, so give it a try.
	//             C) ':' followed by absolute path. The absolute path may
	//             Olson timezone info, so give it a try.
	//
	// lack of ":" indicates try "format 1" before file based specification
	//            As file based timezone selection is the only form that can
	//            give Olson timezones, that is the only form we consider. Gnu
	//            clib tries file based selection if it can't parse the normal
	//            form. This software always tries that.
	if (env_tz[0] == ':') {
		env_tz += 1;
		tz_len -= 1;
	}

	if (env_tz[0] == '/') {	// absolute path
		// must start with "/usr/share/zoneinfo" (ZONEINFO_DIR) or TZDIR
		// obtained from the environment.
		if (strstr(env_tz, zoneinfo_dir) != env_tz) return NULL;
		strcpy(path, env_tz);
	} else {
		// relative to the system zoneinfo dir (which may be overridden by
		// TZDIR)
		strncpy(path, zoneinfo_dir, sizeof(path));
		strncat(path, env_tz, sizeof(path));
	}

	// make sure it is actually a zoneinfo file
	if (! is_zoneinfo_file(path)) return NULL;

	// copy the results while skipping the zoneinfo_dir + '/'
	if (strlen(path + strlen(zoneinfo_dir)) < buf_len) {
		strncpy(buf, path + strlen(zoneinfo_dir), buf_len);
		return buf;
	}

	return NULL;
}

/**
 * @fn char *log_get_timezone(char * const buf, size_t const buf_len)
 * @brief Look for an Olson timezone string.
 * @details
 *
 * Search for an Olson timezone string using the process's environment first,
 * then the system default. If a timezone string is found, it is copied to the
 * supplied buffer, and the buffer address is retured. Otherwise NULL is
 * returned.
 *
 * @param buf a buffer to return the timezone name in
 * @param buf_len the length of that buffer
 * @return buf on success, NULL on failure
 */
char *log_get_timezone(char * const buf, size_t const buf_len) {
	char *env_tz = NULL;
	char *zoneinfo_dir;
	char *result = NULL;

/* This code mimics glibc2 tzset(3) behaviour. It may not work otherwise. */
// Assumes all versions of glibc version 2 work. May want to check.
// __GLIBC_MINOR__ also.
/*
#if ! defined __GLIBC__ || __GLIBC__ < 2
	return NULL;
#endif
*/

	// Make sure we were given a place to put the results
	if ((buf == NULL) || (buf_len < 1)) return NULL;

	// both check_env() and parse_localtime_link() use zoneinfo_dir
	zoneinfo_dir = get_zoneinfo_dir();
	if (zoneinfo_dir == NULL) return NULL;

	// check if the system timezone is overridden in the environment
	env_tz = getenv("TZ");

	// Environment overrides all - if TZ is set, see if an Olson timezone
	// can be found from it, otherwise stop looking.
	// TZ may be in the form of 'TZ="NZST-12:00:00NZDT-13:00:00,M10.1.0,M3.3.0"'
	// In that case, no zoneinfo file matching it will be found, so we don't
	// return a misleading timezone from the system default. See tzset(3).
	if (env_tz != NULL) {
		if (check_env(buf, buf_len, env_tz, zoneinfo_dir) == buf) {
			result = buf; // success
		}
		goto free_mem;
	}

	// See if there is an /etc/timezone file
	if (log_read_timezone_file(buf, buf_len) == buf) {
		result = buf;
		goto free_mem;
	}

	// Try if /etc/localtime is a symbolic link that can be parsed
	if (parse_localtime_link(buf, buf_len, zoneinfo_dir) == buf) {
		result = buf;
	}

free_mem:
	free(zoneinfo_dir);

	return result;
}
