#include "tinylogger.h"

/**
 * fn int main(void)
 *
 * @brief The clock used to take the timestamp may be selected.
 * 
 * The same clock is used for both channels so that the timestamps will agree.
 *
 * Selecting a clock also resets the elapsed time.
 *
 * CLOCK_REALTIME is the default, and should be used in most cases.
 * CLOCK_MONOTONIC_RAW is most appropriate if using an elapsed time format, as
 * it is guaranteed not to go backward, and the function that computes delta
 * time is expecting increasing values of time.
 */
int main(void) {

	log_select_clock(CLOCK_REALTIME);
	log_info("CLOCK_REALTIME");

	log_select_clock(CLOCK_REALTIME_COARSE);
	log_info("CLOCK_REALTIME_COARSE");

	log_select_clock(CLOCK_MONOTONIC);
	log_info("CLOCK_MONOTONIC");

	log_select_clock(CLOCK_MONOTONIC_COARSE);
	log_info("CLOCK_MONOTONIC_COARSE");

	log_select_clock(CLOCK_MONOTONIC_RAW);
	log_info("CLOCK_MONOTONIC_RAW");

	log_select_clock(CLOCK_BOOTTIME);
	log_info("CLOCK_BOOTTIME");

}
