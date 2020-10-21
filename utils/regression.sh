#!/bin/bash

# Run all the examples and check for exit value of 0 (EXIT_SUCCESS).
# Run all the examples with vgrind and check for no memory leaks.
#
# This script does a limited search for the examples to run.
#
# It assumes the executable are already built.
#
# Direct invocation of regression.sh works from non VPATH build trees, whether
# regression.sh the original copy in utils, or the copy in quick-start. The
# examples to run are found relative to the shell script.
# Invocation via "make check" or "make distcheck" are supported whether it is
# from VPATH builds or not.

# set EXAMPLE_DIR if you want to override the search
#EXAMPLE_DIR=

# valgrind seems to have serious problems on Raspbian (RaspberryPi)
# see: https://www.raspberrypi.org/forums/viewtopic.php?t=166035
# possible workaround: comment out #/usr/lib/arm-linux-gnueabihf/libarmmem.so in
#         /etc/ld.so.preload
# uname -i reports "unknown" on Raspbian - use it to disable valgrind checks
PLATFORM_STRING=$( uname -i )
if [ "$PLATFORM_STRING" == "unknown" ]; then
	DO_VALGRIND=false
else
	DO_VALGRIND=true
fi

SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"

# try to find the examples relative to the script
if [ -z "$EXAMPLE_DIR" ]; then
	BASENAME="$(basename $SCRIPT_DIR)"
	if [ "$BASENAME" == "utils" ]; then
		# examples are in top_srcdir/demo
		EXAMPLE_DIR="$SCRIPT_DIR/../demo"
	elif [ "$BASENAME" == "quick-start" ]; then
		# examples are in top_srcdir/quick-start/examples
		EXAMPLE_DIR="$SCRIPT_DIR/examples"
	fi

	# get a clean pathname for EXAMPLE_DIR
	EXAMPLE_DIR="$( cd "$EXAMPLE_DIR" >/dev/null 2>&1 && pwd )"
fi

# make sure there are samples in that directory
if [ -n "$EXAMPLE_DIR" ]; then
	PROGS_LIST=( $(find "$EXAMPLE_DIR" -type f -executable) )
	if [ "${#PROGS_LIST[@]}" -lt 1 ]; then
		EXAMPLE_DIR=
	fi
fi

function print_help {
echo "===== can't find executable examples ====="
echo "If you are running regression.sh from the command line, make sure you"
echo "run make first."
echo "If you are running make check or make distcheck, run from the top level."
}

# If that fails, look relative to the working directory - it may be a VPATH
# build (build outputs are not in the source tree).
# Expecting executables in the demo subdirectory.
if [ -z "$EXAMPLE_DIR" ]; then
	RUN_DIR=$( pwd )
	if [ -d "$RUN_DIR/demo" ]; then
		EXAMPLE_DIR="$RUN_DIR/demo"
		PROGS_LIST=( $(find "$EXAMPLE_DIR" -type f -executable) )
		if [ "${#PROGS_LIST[@]}" -lt 1 ]; then
			echo "no executables in $EXAMPLE_DIR"
			print_help
			exit 1
		fi
	else
		print_help
		exit 1
	fi
fi

# where to place temporary output
TMP_DIR="/tmp/libtinylogger-regression"
WORK_DIR="$TMP_DIR/work"				# for output files, which may be LONG
LOG_FILE="$TMP_DIR/stdout-stderr.log"	# for stdout/stderr

# leave the stdout-stderr log, but remove individual output files
function cleanup {
	rm -rf "$WORK_DIR"
}

# options for each example
declare -A options
options["beehive"]="-v"
options["check-timezone"]="--pass"

# run a test
function run_test {
	local status
	local PWD

	PWD=$( pwd )
	
	# make clean temp dir and go there
	rm -rf "$WORK_DIR"
	mkdir -p "$WORK_DIR"
	cd "$WORK_DIR" || exit 1

	$* >> "$LOG_FILE" 2>&1
	status=$?

	# return to original directory
	cd "$PWD" || exit 1

	# cleanup
	rm -rf "$WORK_DIR"
	return $status
}

# clean up on exit
trap cleanup EXIT

# start with a clean slate
rm -rf $TMP_DIR
mkdir $TMP_DIR

# make sure we found some examples
#PROGS_LIST=( $(find "$EXAMPLE_DIR" -type f -executable) )
#if [ "${#PROGS_LIST[@]}" -lt 1 ]; then
#	echo "no examples"
#	exit 1
#fi

# run the tests
# exit on first error
for PROG in ${PROGS_LIST[@]}; do
	PROG_BASENAME="$( basename "$PROG")"
	OPTIONS="${options[$PROG_BASENAME]}"
	echo "==== testing ====> $PROG"

	EXE="$PROG"

	# check program return status (should be 0)
	echo "---- command with options: $PROG_BASENAME $OPTIONS"
	run_test $EXE $OPTIONS
	status=$?
	if [ $status != 0 ]; then
		echo "$PROG failed with status $status"
		exit $status
	else
		echo "--   status: $status"
	fi

	# check that there are no memory leaks (valgrind status should be 0)
	if $DO_VALGRIND; then
		echo "---- valgrind with options: valgrind --error-exitcode=1 $PROG_BASENAME $OPTIONS"
		run_test valgrind --error-exitcode=1 $EXE $OPTIONS
		status=$?
		if [ $status != 0 ]; then
			echo "$PROG failed with status $status"
			exit $status
		else
			echo "--   status: $status"
		fi
	fi
done

echo "===================================" 
echo "============  SUCCESS  ============" 
echo "===================================" 
