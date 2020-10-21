
# Documentation produced by doxygen is placed in subdirectories of doc.
# html docs will be in doc/html
# man pages will be in doc/man
# A pdf doc can be made in doc/latex (Makefile provided).
# If you have doxygen installed, try "make docs".
GENERATED_DIRS = doc guide

all: examples

logger:
	$(MAKE) -C logger
	$(MAKE) -C logger install

demo-lib:
	$(MAKE) -C demo-lib

examples: logger demo-lib
	$(MAKE) -C examples

clean:
	$(MAKE) -C logger clean
	$(MAKE) -C demo-lib clean
	$(MAKE) -C examples clean
	rm -rf $(GENERATED_DIRS)
	rm -f lib/libtinylogger.a

check: examples
	./regression.sh

# requires doxygen to be installed
docs:
	doxygen

.PHONY: all logger demo-lib examples clean docs check
