#!/bin/bash

# This script makes COPIES of the relevant sources in a tree representative
# of typical setup.
#
# This script finds its location, and finds the relevant files relative to it.
#
# It may be executed from its distributed position.
#
# The script may be executed from anywhere, but must not be moved.
#
# A project tree will be created under the directory from which the script is
# executed.
#
# The examples Makefile expects every source file in the examples directory to
# be a separate program.

# create a source tree in subdirectories of the working director"
LIB_DIR="lib"				# where to put the logger library
INC_DIR="include"			# where to put the logger library public includes
LOGGER_DIR="logger"			# where to put the logger library source files
EXAMPLE_DIR="examples"		# where to put the examples
DEMO_LIB_DIR="demo-lib"		# where to put demo utils

# where to find the original sources
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" >/dev/null 2>&1 && pwd )"
TOP_SRCDIR="$( dirname "$SCRIPT_DIR" )"

# sanity check
if [ ! -e "$TOP_SRCDIR"/README.md ]; then
	echo "It appears that the script has moved from its original position"
	exit 1
fi

# The subdirectories created can be removed by:
# $ setup.sh clean
if [ $# = 1 ]; then
	if [ "$1" = clean ]; then 
		echo $1
		rm -rf logger include lib demo-lib examples
		exit $?
	fi
fi

# create and populate the include dir
mkdir -p "$INC_DIR"
cp "$TOP_SRCDIR"/src/tinylogger.h "$INC_DIR"

# create a lib directory to deposit the library
mkdir -p $LIB_DIR

# populate the logger source dir
mkdir -p "$LOGGER_DIR"/alternatives
cp "$SCRIPT_DIR"/config.h "$LOGGER_DIR"
cp "$TOP_SRCDIR"/src/private.h "$LOGGER_DIR"
cp "$TOP_SRCDIR"/src/alternatives/sd-daemon.h "$LOGGER_DIR"/alternatives
cp "$TOP_SRCDIR"/src/*.c "$LOGGER_DIR"
cp "$SCRIPT_DIR"/Makefile.logger "$LOGGER_DIR"/Makefile

# populate the example source dir
mkdir -p "$EXAMPLE_DIR"
cp "$TOP_SRCDIR"/demo/*.[ch] "$EXAMPLE_DIR"
cp "$SCRIPT_DIR"/Makefile.examples "$EXAMPLE_DIR"/Makefile

# re-shuffle the common demo utils
mkdir -p "$DEMO_LIB_DIR"
mv "$EXAMPLE_DIR"/demo-utils.[ch] "$DEMO_LIB_DIR"
cp "$SCRIPT_DIR"/Makefile.demo-lib "$DEMO_LIB_DIR"/Makefile

# copy the top Makefile over (if necessary)
if [ "$SCRIPT_DIR" != "$( pwd )" ]; then
	cp "$SCRIPT_DIR"/Makefile .
fi

