#!/bin/bash

# generate logs using all the four possible combinations of enabling timezone
# and json-header

# Only works from the top source dir, with autotools. And it must be run from
# the top source dir like this:
# $ utils/gen-json-examples.sh

# And, if you have been using the quick-start configuration, remember to run
# autoreconf -i first.

# Note: this runs 4 configuration/build cycles - don't be alarmed!

CLEAN_FILES=(
	tinylogger.json
	tinylogger-none.json
	tinylogger-timezone.json
	tinylogger-json-header.json
	tinylogger-timezone-json-header.json)

for F in "${CLEAN_FILES[@]}"; do
	rm -f "$F"
done

./configure
make
demo/json
mv tinylogger.json tinylogger-none.json

./configure --enable-timezone
make
demo/json
mv tinylogger.json tinylogger-timezone.json

./configure --enable-json-header
make
demo/json
mv tinylogger.json tinylogger-json-header.json

./configure --enable-timezone --enable-json-header
make
demo/json
mv tinylogger.json tinylogger-timezone-json-header.json

