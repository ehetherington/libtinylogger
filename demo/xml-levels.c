#include "tinylogger.h"

#define LOG_FILE "xml-levels.xml"

/**
 * @fn int main(void)
 *
 * @brief print messages with all available logging levels
 *
 * This shows the mapping of log levels to levels compatible with
 * java.util.logging levels. Where the level is a standard
 * java.util.logging.Level, the text label is used. Otherwise, a numeric level
 * interpolated or extrapolated relative to the java.util.logging.Levels.
 *
 * Notice that the sd-daemon levels not available in java.util.logging.Level
 * are printed with numberic values.
 */
int main(void) {
	/*
	 * Print a series of xml formatted messages
	 * Use the xml formatter
	 * Use line buffered output
	 */
	LOG_CHANNEL *ch1 = log_open_channel_f(LOG_FILE, LL_ALL, log_fmt_xml, true);
	(void) ch1;	// quiet the "unused variable" warning

	log_emerg("emerg level");	// sd-daemon only
	log_alert("alert level");	// sd-daemon only
	log_crit("crit level");		// sd-daemon only
	log_severe("severe level");
	log_err("err level");		// sd-daemon only
	log_warning("warning level");
	log_notice("notice level");	// sd-daemon only
	log_info("info level");
	log_config("config level");
	log_debug("debug level");	// sd-daemon only
	log_fine("fine level");
	log_finer("finer level");
	log_finest("finest level");

	log_done();

	return 0;
}
