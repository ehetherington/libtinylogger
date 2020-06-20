#!/bin/bash

set -e

SRC_DIR="$1"
GUIDE_DIR="guide"
mkdir -p "$GUIDE_DIR"

fix_links.awk PFPR="$GUIDE_DIR" < $SRC_DIR/guide.md > "$GUIDE_DIR"/guide.md
fix_links.awk PFPR="$GUIDE_DIR" < $SRC_DIR/levels.md > "$GUIDE_DIR"/levels.md
fix_links.awk PFPR="$GUIDE_DIR" < $SRC_DIR/doxygen.md > "$GUIDE_DIR"/doxygen.md
fix_links.awk PFPR="$GUIDE_DIR" < $SRC_DIR/performance.md > "$GUIDE_DIR"/performance.md
fix_links.awk PFPR="$GUIDE_DIR" < $SRC_DIR/xml_formatter.md > "$GUIDE_DIR"/xml_formatter.md
fix_links.awk PFPR="$GUIDE_DIR" < $SRC_DIR/daemon-hints.md > "$GUIDE_DIR"/daemon-hints.md
cp "$SRC_DIR"/logger.dtd "$GUIDE_DIR"/logger.dtd
