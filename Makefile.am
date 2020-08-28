EXTRA_DIST = LICENSE README.md utils

EXTRA_DIST += \
	quick-start/setup.sh \
	quick-start/Makefile \
	quick-start/Makefile.logger \
	quick-start/Makefile.demo-lib \
	quick-start/Makefile.examples \
	quick-start/config.h

SUBDIRS = src demo guide doc

# regression.sh isn't a unit test
# It executes each demo program, and checks that it returns 0.
# It re-runs the program with valgrind to check for memory leaks.
# It takes a 3 or 4 minutes - please be patient.
# Tests are run by "make check" or "make distcheck".
TESTS = utils/regression.sh
