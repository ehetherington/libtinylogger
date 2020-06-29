## Installing as a lib in /usr/local/lib

To configure, build, and install as a lib in /usr/local/lib:

```
$ autoreconf -i
$ ./configure
$ make
$ sudo make install
```

After installing in /usr/local/lib, running ./libtool --finish /usr/local/lib
produces this...

```
Libraries have been installed in:
   /usr/local/lib

If you ever happen to want to link against installed libraries
in a given directory, LIBDIR, you must either use libtool, and
specify the full pathname of the library, or use the '-LLIBDIR'
flag during linking and do at least one of the following:
   - add LIBDIR to the 'LD_LIBRARY_PATH' environment variable
     during execution
   - add LIBDIR to the 'LD_RUN_PATH' environment variable
     during linking
   - use the '-Wl,-rpath -Wl,LIBDIR' linker flag
   - have your system administrator add LIBDIR to '/etc/ld.so.conf'

See any operating system documentation about shared libraries for
more information, such as the ld(1) and ld.so(8) manual pages.
```

This is important information describing ways to use the library installed
in alternate places.

### Raspbian (RaspberryPi) Notes

After install, programs compiled fine with:
```
$ gcc -o hello hello.c -ltinylogger -lpthread
```
but failed to run because the library wasn't found.

Referring to "have your system administrator add LIBDIR to
'/etc/ld.so.conf'", on Raspbian /usr/local/lib already is in /etc/ld.so.conf
via /etc/ld.conf.so.d/libc.conf.

Running ./libtool --finish /usr/local/lib apparently runs ldconfig -n
/usr/local/lib. And that doesn't seem to accomplish anything. But running
ldconfig with no options does get libtinylogger into the ld.so.cache, and
all is fine.

I added ldconfig in src/Makefile.am to the install-exec-hook. If you
are installing in a non-standard place this does no harm. If you are
installing in /usr/local/bin, this actually gets the job done.

To verify that the library got into the ld.so cache, try:
ldconfig -p | grep tinylogger

if it made it's way into the cache, you will get:

        libtinylogger.so.0 (libc6,hard-float) => /usr/local/lib/libtinylogger.so.0
        libtinylogger.so (libc6,hard-float) => /usr/local/lib/libtinylogger.so

### Other distros

If you want to use the library without using the above libtool suggestions...

Check /etc/ld.so.conf and /etc/ld.so.conf.d/ files. Make sure /usr/local/lib
(or wherever you told configure to put it) is mentioned. If it isn't, add
a configuration file with the following contents to /etc/ld.so.conf.d, perhaps
local-lib.conf.

```
# add /usr/local/lib to the configuration
/usr/local/lib
```

