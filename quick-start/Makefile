
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
	rm -f lib/libtinylogger.a

.PHONY: all logger demo-lib examples clean
