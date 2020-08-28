#!/bin/bash

# produce a list of public symbols of libtinylogger

# Must be run in place. VPATH builds are not supported - the executables must
# be in the source tree.
#
# Initially in top_srcdir/utils.
# May be copied to top_srcdir/quick-start.
#
# This really wants to be configured via autotools, but make it useable by
# quick-start.
#
# Must be run after a make (objects must exist).

ARCHIVE=libtinylogger.a
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
if [ $(basename $SCRIPT_DIR) == "utils" ]; then
	# top_srcdir/utils
	# build an archive for convenience
	ar ru $ARCHIVE ../src/*.o
	ranlib $ARCHIVE
else
	# top_srcdir/quick-start
	# cp the existing archive
	cp logger/libtinylogger.a $ARCHIVE
fi


# add known external globals to this list
EXTERN_SYMS=()
EXTERN_SYMS+=("clock_gettime")
EXTERN_SYMS+=("__ctype_b_loc")
EXTERN_SYMS+=("__errno_location")
EXTERN_SYMS+=("exit")
EXTERN_SYMS+=("fclose")
EXTERN_SYMS+=("fflush")
EXTERN_SYMS+=("fopen")
EXTERN_SYMS+=("fprintf")
EXTERN_SYMS+=("free")
EXTERN_SYMS+=("getpid")
EXTERN_SYMS+=("__libc_current_sigrtmax")
EXTERN_SYMS+=("localtime_r")
EXTERN_SYMS+=("malloc")
EXTERN_SYMS+=("perror")
EXTERN_SYMS+=("pthread_attr_destroy")
EXTERN_SYMS+=("pthread_attr_init")
EXTERN_SYMS+=("pthread_attr_setstacksize")
EXTERN_SYMS+=("pthread_create")
EXTERN_SYMS+=("pthread_getname_np")
EXTERN_SYMS+=("pthread_join")
EXTERN_SYMS+=("pthread_kill")
EXTERN_SYMS+=("pthread_mutex_lock")
EXTERN_SYMS+=("pthread_mutex_unlock")
EXTERN_SYMS+=("pthread_self")
EXTERN_SYMS+=("pthread_setname_np")
EXTERN_SYMS+=("pthread_sigmask")
EXTERN_SYMS+=("setvbuf")
EXTERN_SYMS+=("sigaddset")
EXTERN_SYMS+=("sigemptyset")
EXTERN_SYMS+=("sigwaitinfo")
EXTERN_SYMS+=("snprintf")
EXTERN_SYMS+=("stderr")
EXTERN_SYMS+=("strcasecmp")
EXTERN_SYMS+=("strdup")
EXTERN_SYMS+=("strerror_r")
EXTERN_SYMS+=("strlen")
EXTERN_SYMS+=("syscall")
EXTERN_SYMS+=("vsnprintf")

# the number of external globals
len=${#EXTERN_SYMS[@]}
# build a pattern for grep (sym1|sym2|...)
EXTERN_PATTERN=${EXTERN_SYMS[0]}
for (( i=1; i<len; i++ ));
do
	EXTERN_PATTERN=$EXTERN_PATTERN"|"${EXTERN_SYMS[i]}
done

GLOBALS=$(readelf -s $ARCHIVE | grep GLOBAL | cut -c 60-)
GLOBALS=$(echo "$GLOBALS" | sort | uniq)
echo "$GLOBALS" | grep -E -v "$EXTERN_PATTERN"

rm $ARCHIVE
