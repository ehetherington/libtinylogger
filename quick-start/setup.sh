#!/bin/bash

# This script makes COPIES of the relevant sources in a tree representative
# of a typical setup.
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

DOC_DIR="doc"		# doxygen output
GUIDE_DIR="guide"	# markdown for doxygen


# making the docs creates html man and latex sub-directories in doc
DOC_DIRS="$DOC_DIR $GUIDE_DIR"

# directories to remove for "clean"
CLEAN_DIRS="$LIB_DIR $INC_DIR $LOGGER_DIR $EXAMPLE_DIR $DEMO_LIB_DIR $DOC_DIRS"

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
		cd $SCRIPT_DIR
		echo "cleaning $SCRIPT_DIR"
		rm -rf $CLEAN_DIRS
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

# copy the top Makefiles and setup.sh over (if necessary)
if [ "$SCRIPT_DIR" != "$( pwd )" ]; then
	cp "$SCRIPT_DIR"/config.h .
	cp "$SCRIPT_DIR"/Makefile* .
	cp "$SCRIPT_DIR"/setup.sh .
	cp "$SCRIPT_DIR"/Doxyfile .
fi

# copy the guide markdown
mkdir -p $GUIDE_DIR
cp "$TOP_SRCDIR"/guide/*.md $GUIDE_DIR
