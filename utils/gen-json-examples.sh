#!/bin/bash

# generate logs using all the four possible combinations of enabling timezone
# and json-header

# Note: this requires 4 configuration/build cycles - don't be alarmed!

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

