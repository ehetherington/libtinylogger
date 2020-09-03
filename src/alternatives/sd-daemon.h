/*
 * (C) 2020 Edward Hetherington
 * This code is licensed under MIT license (see LICENSE in top dir for details)
 */

/** @file       sd-daemon.h
 *  @brief      NOT THE REAL THING
 *  @author     Edward Hetherington
 *
 * THIS IS A SUBSTITUTE sd-daemon.h
 * It only defines the SD_XXX macros
 * It is used if the official <daemon/sd-daemon> is not installed.
 */

#ifndef _ALT_SYSTEMD_SD_DAEMON_H
#define _ALT_SYSTEMD_SD_DAEMON_H

#ifndef DOXYGEN_SHOULD_SKIP_THIS

/*
  Log levels for usage on stderr:

          fprintf(stderr, SD_NOTICE "Hello World!\n");

  This is similar to printk() usage in the kernel.
*/
#define SD_EMERG   "<0>"  /* system is unusable */
#define SD_ALERT   "<1>"  /* action must be taken immediately */
#define SD_CRIT    "<2>"  /* critical conditions */
#define SD_ERR     "<3>"  /* error conditions */
#define SD_WARNING "<4>"  /* warning conditions */
#define SD_NOTICE  "<5>"  /* normal but significant condition */
#define SD_INFO    "<6>"  /* informational */
#define SD_DEBUG   "<7>"  /* debug-level messages */

#endif /* DOXYGEN_SHOULD_SKIP_THIS */

#endif
