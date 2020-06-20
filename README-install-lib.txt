After installing in /usr/local/lib, running
./libtool --finish /usr/local/lib
produces this...

----------------------------------------------------------------------
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
----------------------------------------------------------------------

While this is important information, in order to use the library in
the normal fashion, just run ldconfig with no options/args.
ldconfig -n /usr/local/lib doesn't do anything.

I added ldconfig in src/Makefile.am to the install-exec-hook. If you
are installing in a non-standard place this does no harm. If you are
installing in /usr/local/bin, this actually gets the job done.

To verify that the library got into the ld.so cache, try:
ldconfig -p | grep tiny

if it made it's way into the cache, you will get:

        libtinylogger.so.0 (libc6,hard-float) => /usr/local/lib/libtinylogger.so.0
        libtinylogger.so (libc6,hard-float) => /usr/local/lib/libtinylogger.so
