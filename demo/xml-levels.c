#include "tinylogger.h"

#define LOG_FILE "xml-levels.xml"

int main(void) {
	/*
	 * Print a series of xml formatted messages
	 * Use the xml formatter
	 * Use line buffered output
	 */
	LOG_CHANNEL ch1 = log_open_channel_f(LOG_FILE, LL_ALL, log_fmt_xml, true);
	(void) ch1;	// quiet the "unused variable" warning

	log_emerg("emerg level");
	log_alert("alert level");
	log_crit("crit level");
	log_severe("severe level");
	log_err("err level");
	log_warning("warning level");
	log_notice("notice level");
	log_info("info level");
	log_config("config level");
	log_debug("debug level");
	log_fine("fine level");
	log_finer("finer level");
	log_finest("finest level");

	log_done();

	return 0;
}
