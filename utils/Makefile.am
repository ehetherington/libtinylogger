
noinst_SCRIPTS = check-symbols regression
CLEANFILES = $(noinst_SCRIPTS)  # for make clean to remove them

EXTRA_DIST = check-symbols.sh regression.sh

# need to add this rule (normally a default rule)
%: %.sh
	cat $< > $@
	chmod a+x $@

