#!/bin/bash

# generate logs using all the four possible combinations of enabling timezone
# and json-header

# Only works from the top source dir, with autotools. And it must be run from
# the top source dir like this:
# $ utils/gen-json-examples.sh

# And, if you have been using the quick-start configuration, remember to run
# autoreconf -i first.

# The options are enabled by default - they must be disabled.

# Note: this runs 4 configuration/build cycles - don't be alarmed!

CLEAN_FILES=(
	log.json
	example-none.json
	example-timezone.json
	example-json-header.json
	example-timezone-json-header.json)

for F in "${CLEAN_FILES[@]}"; do
	rm -f "$F"
done

# all enabled
./configure
make
demo/json
mv log.json example-timezone-json-header.json

# json-header enabled
./configure --disable-timezone
make
demo/json
mv log.json example-json-header.json

# Olson timezone enabled
./configure --disable-json-header
make
demo/json
mv log.json example-timezone.json

# both disabled
./configure --disable-timezone --disable-json-header
make
demo/json
mv log.json example-none.json

