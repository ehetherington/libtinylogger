EXTRA_DIST = fix_links.awk fix_guide.sh

CLEANFILES = docs cpmarkdown fixmarkdown markdown *.md

CLEANDIRS = man html latex guide demo src

#SHELL = /bin/bash

# for man page installation
man3mans = $(shell if [ -d man ]; then find man -type f -name "*.3"; fi )

code_srcs = $(shell find $(top_srcdir)/src -name "*.[ch]")
demo_srcs = $(shell find $(top_srcdir)/demo -name "*.[ch]")
md_srcs = $(shell find $(top_srcdir)/guide -name "*.md")

# need to prepend $(top_srcdir) to each element in the following...
#guide_srcs = $(shell ls $(top_srcdir)/guide | grep -v guide.md)
# leave README.md in $(top_srcdir) - no translation needed
GUIDE_SRCS = \
	$(top_srcdir)/guide/guide.md \
	$(top_srcdir)/guide/levels.md \
	$(top_srcdir)/guide/performance.md \
	$(top_srcdir)/guide/xml_formatter.md \
	$(top_srcdir)/guide/logger.dtd \
	$(top_srcdir)/guide/daemon-hints.md \
	$(top_srcdir)/guide/doxygen.md

# dependancies need work
SOURCES = \
	doxygen.conf \
	$(GUIDE_SRCS) \
	$(demo_srcs) \
	$(code_srcs)

# do nothing by default
default:

# strategy: build the docs Out-Of-Tree for two reasons
# 1) Github and doxygen treat markdown links differently. The goal is to have
#    Gutub understand the links "as is" so they work on github. The are "fixed
#    up" in the copy to the doxygen build tree.
# 2) Make it easy to distribute built docs as a separate tarball. The markdown
#    references both the actual source code and the demo/example source code.
#    Copy all of them to the build/docs distrubution build tree.
#
#	tar c $(demo_srcs) | tar -C html -xp
#	tar c $(code_srcs) | tar -C html -xp
docs: $(SOURCES) markdown
	mkdir -p html/guide
	cp -f $(top_srcdir)/guide/logger.dtd html/guide
	mkdir -p html/src
	tar -C $(top_srcdir) -c \
		$(shell cd $(top_srcdir); find src  -name "*.[ch]") \
		| tar -C html -xp
	mkdir -p html/demo
	tar -C $(top_srcdir) -c \
		$(shell cd $(top_srcdir); find demo -name "*.[ch]") \
		| tar -C html -xp
	doxygen doxygen.conf
	touch docs

# gendocs is a .PHONY target, docs is a timestamp file
gendocs: docs

# separate cpmarkdown from fixmarkdown so that the file list in
# fixmarkdown is evaluated after the files in guide are updated
#	tar -c $(md_srcs) | tar -xp
cpmarkdown: $(md_srcs)
	@echo "================ updating copies of markdown ==============="
	rm -rf guide
	tar -C $(top_srcdir) -c \
		$(shell cd $(top_srcdir); find guide -name "*.md") | tar -xp
	touch cpmarkdown

fixmarkdown: cpmarkdown
	@echo "================ adjusting markdown links    ==============="
	for MD in $(shell find guide -type f -name "*.md"); \
	do \
		awk -f $(top_srcdir)/doc/fix_links.awk PFPR=guide < $$MD > tmp.md; \
		rm -f $$MD; \
		mv tmp.md $$MD; \
	done
	touch fixmarkdown

markdown: fixmarkdown
	touch markdown

tarball: gendocs
	tar cf $(top_srcdir)/$(PACKAGE)-$(VERSION)-doc.tar.gz html

install-data-local: gendocs

#install-man-local: gendocs

clean-local:
	rm -rf $(CLEANDIRS)

man3_MANS = $(man3mans)

.PHONY: gendocs tarball clean
