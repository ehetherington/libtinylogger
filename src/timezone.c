/*
 * (C) 2020 Edward Hetherington
 * This code is licensed under MIT license (see LICENSE in top dir for details)
 */

/** @file       timezone.c
 *  @brief      Find the Olson timezone.
 *  @details
 *
 *  The process's environment is inspected first, then the system's default
 *  timezone.
 *
 *  If the TZ environment variable is set, then it looks in the zoneinfo
 *  database for a matching zoneinfo file. If the TZDIR environment variable
 *  is set, it is used in the same manner as tzset(). If a matching zoneinfo
 *  exists, it is used. If no matching file is found, the search ends.
 *
 *  If the TZ environment variable is NOT set, then it looks for the system
 *  default timezone. If the system default points to a zoneinfo file in
 *  zoneinfo database, it is used. Otherwise it is ignored.
 *
 *  If neither of the above methods succeeds, then no timezone is appended to
 *  the timestamp.
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

#include <sys/types.h>
#include <sys/stat.h>

#include "config.h"

/** the system default timezone_file (plain text) */
#define TIMEZONE_FILE "/etc/timezone"

/** the system default localtime file (zoneinfo file) */
#define LOCALTIME_FILE "/etc/localtime"

/** where the zoneinfo database is installed */
/**
 * To override:
 *     - quick-start: set in quick-start/config.h
 *     - autoconf:
 * $ ZONEINFO_DIR="/my/alternate/database" ./configure
 */
#ifndef ZONEINFO_DIR
#define ZONEINFO_DIR "/usr/share/zoneinfo"
#endif

/** zoneinfo files begin with "TZif" */
#define TZ_MAGIC "TZif"

/**
 * @fn static bool is_zoneinfo_file(char const * const path)
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
 * @fn ino_t get_inode(char const * const pathname)
 * @brief return the inode of the specified pathname.
 * @param the pathname of the file to check
 * @return inode on success, 0 on failure
 */
static ino_t get_inode(char const * const pathname) {
	struct stat sb;
	if (stat(pathname, &sb) == -1) return 0;
	return sb.st_ino;
}

/**
 * @fn static char *parseLocaltimeLink(char *, char *, char *, size_t)
 * @brief Strip the timezone database part of the link address to return the
 *        timezone part.
 *
 * /etc/localtime should be a symbolic link to the proper timezone file
 * Using `$ readlink /etc/timezone`, it should be a link to something like
 * (on RHEL) (relative):
 * "../usr/share/zoneinfo/America/New_York", where "America/New_York" is the TZ
 *
 * Or, on Raspbian (Debian base) (absolute):
 * "/usr/share/zoneinfo/America/New_York"
 *
 * The link must include the zoneinfo_dir, in either absolute or relative form.
 * A relative link, as on RHEL, is not properly followed. That is, the "../" or
 * "./" part is not directly accounted for. But after editing it, it is verified that
 * the resultant pathname points to the same file (same inode).
 *
 * @param localtime_file the path to the localtime file
 * @param zoneinfo_dir the timezone database directory
 * @param buf a buffer to return the results, which should be an Olson timezone
 * @param the length of that buffer
 * @return the address of buf on success, NULL otherwise
 */
static char *parseLocaltimeLink(char const * const localtime_file,
	char const * const zoneinfo_dir, char *buf, size_t buf_len) {
	int	link_len;
	struct stat sb;
	char link[PATH_MAX + 1] = {0};
	ino_t link_inode, target_inode;

	// make sure it points to a zoneinfo file
	if (! is_zoneinfo_file(localtime_file)) return NULL;

	// remember the inode of that file to make sure the slicing and dicing
	// worked
	if ((link_inode = get_inode(localtime_file)) == 0) {
		return NULL;
	}
	printf("inode of localtime_file = %ld\n", link_inode);

	// clear the results buffer
	memset(buf, 0, buf_len);

	// make sure the path actually is a symbolic link
	// we can't get the Olson timezone from an actual zoneinfo file or hard link
	// we need a symbolic link to extract the timezone name
	if ((lstat(localtime_file, &sb) != 0) || ! S_ISLNK(sb.st_mode)) return NULL;

	// get the symbolic link
	link_len = readlink(localtime_file, link, sizeof(link) - 1);
	if (link_len == -1) return NULL;

	char zoneinfo_path[PATH_MAX] = {0};	// make sure we have a terminating null
	if (link[0] == '/') { // absolute path
		memcpy(zoneinfo_path, link, strlen(link));
	} else { // relative path
		// build the path to the zoneinfo file in zoneinfo_path[]
		// need the directory of the localtime file
		char *last_slash = rindex(localtime_file, '/');
		if (last_slash == NULL) return NULL;

		// include the last slash (which could be the only character)
		memcpy(zoneinfo_path, localtime_file, 1 + last_slash - localtime_file);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-truncation"
		// append the symbolic link to the timezone_file directory
		strncat(zoneinfo_path, link,
			sizeof(zoneinfo_path) - (last_slash - localtime_file));
#pragma GCC diagnostic pop
	}

	// append "/' to the zoneinfo_dir to get a string to find
	char zid_prefix[PATH_MAX];
	strcpy(zid_prefix, zoneinfo_dir);
	strcat(zid_prefix, "/");

	// see if the expected zid_prefix is in the path
	// (temporarily points to the start)
	char *zoneinfo_dir_end = strstr(zoneinfo_path, zid_prefix);
	if ((zoneinfo_dir_end) == NULL) return NULL;

	// make sure it is the same as the original link
	target_inode = get_inode(zoneinfo_dir_end);
	printf("original inode = %ld, zoneinfo_dir_end = %s, zoneinfo_path inode = %ld\n",
		link_inode, zoneinfo_dir_end, target_inode);
	
	// make sure the path actually fits into the result buffer
	// leave room for the null termination
	if (strlen(zoneinfo_path) >= buf_len) return NULL;

	// adjust zoneinfo_dir_end to actually point to the end of zoneinfo_dir
	zoneinfo_dir_end += strlen(zid_prefix);

	// copy it to the result buffer
	memcpy(buf, zoneinfo_dir_end, strlen(zoneinfo_dir_end));

	return buf;
}

/**
 * @fn char *log_read_timezone_file(char const * const timezone_file,
 *                                  char *buf, size_t buf_len)
 * @brief Read the system timezone file and return its contents.
 * @param timezone_file the pathname of that file
 * @param buf a buffer to copy the file's contents
 * @param buf_len the length of that buffer
 * @return the supplied buffer on success, NULL on failure
 */
static char *log_read_timezone_file(char const * const timezone_file,
	char *buf, size_t buf_len) {
	size_t n_read;
	char *timezone = NULL;
	FILE *file;

	if (buf_len < 1) return NULL;
	buf[0] = '\0';

	file = fopen(timezone_file, "r");
	if (file == NULL) return NULL;

	// turn off complaint about %ms being posix
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wformat"
	n_read = fscanf(file, "%ms", &timezone);
#pragma GCC diagnostic pop

	if (n_read == 1) {
		if (strlen(timezone) < buf_len) {
			strncpy(buf, timezone, buf_len);
		}
		free(timezone);
		return buf;
	}

	return NULL;
}

/**
 * @fn char *log_get_default_tz(char *buf, size_t buf_len)
 * @brief get the system default timezone
 * @param buf the buffer to copy the timezone string to
 * @param buf_len the length of that buffer
 * @return the supplied buffer on success, NULL on failure
 */
static char *log_get_default_tz(char *buf, size_t buf_len)
{
	if (log_read_timezone_file(TIMEZONE_FILE, buf, buf_len) == buf) {
		return buf;
	}

	if (parseLocaltimeLink(LOCALTIME_FILE, ZONEINFO_DIR, buf, buf_len) == buf) {
		return buf;
	}

	return NULL;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-overflow"
#pragma GCC diagnostic ignored "-Wstringop-truncation"
/**
 * @fn static char *check_env(char *env_tz, char *tz_buf, int tz_buf_len)
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
 * (See the tzset() man page and
 * https://www.gnu.org/software/libc/manual/html_node/TZ-Variable.html)
 *
 * @param env_tz the timezone set in the environment
 * @param timezone buffer to place a discovered timezone
 * @param timezone_len the length of that buffer
 * @return the address of the timezone parameter on success
 */
static char *check_env(char *env_tz, char *tz_buf, int tz_buf_len) {
	char *zoneinfo_dir;
	char path[PATH_MAX];
	int	tz_len;

	if (env_tz == NULL) {
		return NULL;
	}

	tz_len = strlen(env_tz);

	// shortest timezones are GB and NZ
	if (tz_len < 2) return NULL;

	zoneinfo_dir = getenv("TZDIR");
	if ((zoneinfo_dir == NULL) || (strlen(zoneinfo_dir) < 1)) {
		zoneinfo_dir = ZONEINFO_DIR;
	}
	if (zoneinfo_dir[0] != '/') return NULL;

	char *last_slash;
	while ((strlen(zoneinfo_dir) > 1) &&
		(last_slash = rindex(zoneinfo_dir, '/')) ==
			(zoneinfo_dir - 1 + strlen(zoneinfo_dir))) {
		zoneinfo_dir[strlen(zoneinfo_dir) - 1] = '\0';
	}

	// Accept files relative to the system timezone database directory - the
	// names of the timezone files in that directory are the names of timezones!
	// Also accept absolute pathnames that start with the database directory.
	//
	// leading ':' indicates timezone file being specified
	// lack of ":" indicates try "format 1" before file based specification
	// This code only accepts file based specification, and is selective at
	// that!
	if (env_tz[0] == ':') {
		env_tz += 1;
		tz_len -= 1;
	}

	if (env_tz[0] == '/') {	// absolute path
		// must start with "/usr/share/zoneinfo" (ZONEINFO_DIR)
		if (strstr(env_tz, zoneinfo_dir) != env_tz) {
			return NULL;
		}
		strcpy(path, env_tz);
	} else {
		// append it to "/usr/share/zoneinfo" (ZONEINFO_DIR)
		strncpy(path, zoneinfo_dir, sizeof(path));
		if (path[strlen(path) - 1] != '/') {
			strncat(path, "/", sizeof(path));
		}
		strncat(path, env_tz, sizeof(path));
	}

	// make sure it is actually a zoneinfo file
	if (!is_zoneinfo_file(path)) return NULL;

	// copy the results while skipping the zoneinfo_dir + '/'
	strncpy(tz_buf, path + strlen(zoneinfo_dir) + 1, tz_buf_len);
	return tz_buf;
}

/**
 * @fn char *log_get_timezone(char *buf, size_t buf_len)
 * @brief Look for an Olson timezone string.
 * @details
 *   First, the supplied buffer (buf) is cleared to 0.
 *
 *   First look in the environment for the TZ variable. If it is set and there
 *   is a matching zoneinfo file in the system zoneinfo directory, that string
 *   is copied to buf. If that fails, NULL is returned immediately - the system
 *   default timezone is not considered.
 *
 *   While considering the TZ environment variable, The TZDIR environment
 *   variable, if set, is used as in tzset().
 *
 *   If TZ is not set, the system default timezone is determined by looking at
 *   /etc/timezone, then /etc/localtime.
 *
 *   If /etc/timezone exists, its contents are trusted and used directly
 *   without further inspection. The /etc/timezone contents are copied to buf.
 *
 *   If /etc/localtime is not a symbolic link, Olson timezone names can not be
 *   determined, and nothing is copied to buf.
 *
 *   If it is a symbolic link, it is read and inspected. If a matching zoneinfo
 *   file in the zoneinfo database directory is found, the Olson timezone name
 *   in the pathname is copied to buf.
 *
 * @param buf a buffer to return the timezone name in
 * @param buf_len the length of that buffer
 * @return buf on success, NULL on failure
 */
char *log_get_timezone(char *buf, size_t buf_len) {
	char *env_tz = NULL;

	env_tz = getenv("TZ");
	if (env_tz != NULL) {
		return (check_env(env_tz, buf, buf_len) == buf) ? buf : NULL;
	}

	return (log_get_default_tz(buf, buf_len) == buf) ? buf : NULL;
}
