#include <stdio.h>
#include <time.h>

#include "tinylogger.h"

#define FMT_STD "%04d-%02d-%02d %02d:%02d:%02d.%09ld"
struct timespec parse_timespec(char const *timestamp) {
	struct timespec ts = {0};
	int year, month, day;
	char tee;
	int hour, minute, second;

	int n_converted =
		sscanf(timestamp, "%d-%d-%d%c%d:%d:%d",
			&year, &month, &day,
			&tee,
			&hour, &minute, &second);
	if (n_converted == 7) {
		struct tm tm;
		tm.tm_year = year - 1900;
		tm.tm_mon = month - 1;
		tm.tm_mday = day;
		tm.tm_hour = hour;
		tm.tm_min = minute;
		tm.tm_sec = second;

		ts.tv_sec = mktime(&tm);
	}

	return ts;
}

int main(void) {
	struct timespec ts;
	char date[32];
	char **ptz;
	long tz_remainder;
	long hours, minutes, seconds;

	char *sample = "2020-01-02T12:01:23.987654321";
	ts = parse_timespec(sample);
	log_format_timestamp(&ts, FMT_ISO | SP_MILLI, date, sizeof(date));

	ptz = tzname;
	for (int n = 0; n < 2; n++) {
		printf("tzname = %s\n", ptz[n]);
	}

	hours = timezone / (60 *60);
	tz_remainder = timezone % (60 * 60);
	minutes = tz_remainder / 60;
	tz_remainder = tz_remainder % 60;
	seconds = tz_remainder;

	printf("offset = %s %s %02ld:%02ld:%02ld (%d)\n",
		date, *tzname, hours, minutes, seconds, daylight);
}
