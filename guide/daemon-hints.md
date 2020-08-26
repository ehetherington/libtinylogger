## daemon logging hints

Writing a daemon isn't a trivial task. SysV daemons have a lot of requirements,
while the new style daemons are much easier. A good starting reference is
from [freedesktop](https://www.freedesktop.org/software/systemd/man/daemon.html).

Devin Watson has a simple intro to old style 
[Linux Daemon Writing](http://www.netzmafia.de/skripten/unix/linux-daemon-howto.html).

syslog is a natural choice. If you are looking at this, for some reason you
have decided not to use syslog. But you should do a "man 3 syslog" just to be sure.


### systemd/journald

systemd daemons have few requirements. If you have this option, it is probably
the best bet. Assuming you have set up your systemd unit, integration with
systemd is simple. Just use the log_fmt_systemd formatter and stderr.

journalctl is quite adept at displaying your log in filtering messages and
displaying them in different formats.

Setup is:
```{.c}
	#include <tinylogger.h>>

	LOG_CHANNEL *ch1 = log_open_channel_s(stderr, LL_INFO, log_fmt_systemd);
    log_info("this message will be logged with systemd understanding the log levels");

```
This logger package was extracted from a daemon I wrote. It is run by systemd.
The program is called wol-broadcaster. The ExecStart entry says how to start
it. The --daemon option tells wol-broadcaster to use the systemd output format
to the stderr. The default logging level on that channel is INFO. That output
is then managed by the system, and available for reading with journalctl.

I also want to monitor it with much more verbose debugging output. The
-f /tmp/wol-broadcaster.log option tells wol-broadcaster to also log the
messages to /tmp/wol-broadcaster.log. The -L ALL option tells it to log all
message levels to that file.

My service unit file:

```
# base on:
# https://linuxconfig.org/how-to-create-systemd-service-unit-in-linux
# TODO: send mail on failure:
# https://northernlightlabs.se/2014-07-05/systemd-status-mail-on-unit-failure.html

[Unit]
Description=WOL broadcaster service
#Requires=network.target
After=network.target
StartLimitBurst=5
StartLimitIntervalSec=33

[Service]
Type=simple
Restart=always
RestartSec=3
ExecStart=/usr/local/bin/wol-broadcaster --announce --daemon -p 9 -e -L ALL -f /tmp/wol-broadcaster.log
#ExecStop=/bin/kill -HUP $MAINPID
#ExecReload=/bin/kill -HUP $MAINPID

[Install]
WantedBy=multi-user.target

```

### initd/logrotate

Logrotate support was implemented by using a background thread that
listens for a user specified signal to re-open the log file.
[Nominal Animal](https://stackoverflow.com/users/1475978/nominal-animal) made
an excellent
[post](https://stackoverflow.com/questions/53188731/logging-compatibly-with-logrotate#53201067)
on [StackOverflow](https://stackoverflow.com) illustrating that method.


"man lograte" is the first place to look to see what options are available.

Your app doesn't actually need to be a daemon to take advantage of logrotate.
Any long term log file may be managed by logrotate. logrotate can take care
of periodically saving, compressing and keeping a certain number of old files.
In order to take advantage of those services, all you need to do is listen for
a signal from logrotated, and flush and close current log, and restart logging.
This software does that for you.

You need to put a configuration file in /etc/logrotate.d. For example,
/etc/logrotate/yourapp.

If you only want to keep one log called /var/log/yourapp/yourapp.log and you
want to use the USR1 signal to communicate, then create a configuration file
called /etc/logrotate.d/yourapp. Use these contents.
```
/var/log/yourapp/yourapp.log {
    rotate 7
    weekly
    postrotate
        /usr/bin/killall -USR1 yourapp
    endscript
}
```

Matching code to match that: 

```
	#include <signal.h>
	#include <tinylogger.h>

    LOG_CHANNEL *ch1 = log_open_channel_f("/var/log/yourapp/yourapp.log", LL_INFO, log_fmt_standard);
    log_enable_logrotate(SIGUSR1);
    log_info("logging starts");
```

The code only supports two channels, so you can only keep two log files per
process. Changing the following line in private.h will allow any amount.
```
#define LOG_CH_COUNT 2  /**< The number of channels supported. */
```
