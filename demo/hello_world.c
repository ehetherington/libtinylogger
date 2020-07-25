#include "tinylogger.h"

/**
 * @fn int main(void)
 * @brief The minimum program to use the logger library.
 *
 * No initialization is necessary. The library will default to printing
 * messages to stderr in standard format that are at or above the pre-init
 * log level. The pre-init level defaults to LL_INFO, but may be changed using
 * log_set_pre_init_level(LOG_LEVEL log_level).
 */
int main(void) {
	// log_set_pre_init_level(LL_NONE);	// this would turn of all messages
	log_info("hello, %s", "world");
}
