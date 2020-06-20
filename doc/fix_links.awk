#!/usr/bin/awk -f

# usage: awk -f fixLinks.awk PATH_FROM_PROJECT_ROOT="path/from/project/root"

# see:

# git vs doxygen problem:
# https://stackoverflow.com/questions/41476971/is-it-possible-to-write-markdown-links-in-a-way-that-satisfies-both-github-and-d

# awk sheebang
# https://stackoverflow.com/questions/1418245/invoking-a-script-which-has-an-awk-shebang-with-parameters-vars
# debian may not like this (mawk vs awk?) ...
# worked fine for me with mawk on Raspbian (invoked by /usr/bin/awk,
# which is a link to /etc/alternatives/awk which is a link to /usr/bin/mawk

BEGIN {
	current_pattern    = "] *\\( *\\./"
	parent_pattern     = "] *\\( *\\.\\./"
	g_parent_pattern   = "] *\\( *\\.\\./\\.\\./"
	gg_parent_pattern  = "] *\\( *\\.\\./\\.\\./\\.\\./"
	ggg_parent_pattern = "] *\\( *\\.\\./\\.\\./\\.\\./\\.\\./"
}

NR==1 {
	if (PATH_FROM_PROJECT_ROOT) {
		n_dirs = split(PATH_FROM_PROJECT_ROOT, path, "/")
	} else if (PFPR) {
		n_dirs = split(PFPR, path, "/")
	} else {
		print "must set project root with PATH_FROM_PROJECT_ROOT=\"a/b/c\"" > "/dev/stderr"
		print "or PFPR=\"a/b/c\"" > "/dev/stderr"
		exit(1)
	}
}

function print_env() {
	printf "n_dirs = %d\n", n_dirs > "/dev/stderr"
	for (n = 1; n <= n_dirs; n++) {
		printf "dir[%d] = %s\n", n, path[n] > "/dev/stderr"
	}
}

function count_tail_dirs(TAIL) {
	return split(TAIL, tail_dirs, "/");
}

function insert(HEAD, TAIL, COUNT) {
#	if (COUNT == 1) {
#		printf "old TAIL: %s\n", TAIL
#		TAIL = substr(TAIL, 2)
#		printf "new TAIL: %s\n", TAIL
#	}

	if (COUNT > n_dirs) {
		printf "too many levels deep (%d)", COUNT > "/dev/stderr"
		printf " at line %d\n", NR  > "/dev/stderr"
		printf "%s\n", $0 > "/dev/stderr"
		print_env()
		exit(1);
	}
	result = ""
	for (n = 1; n <= n_dirs - COUNT; n++) {
		result = result path[n] "/"
	}
	return HEAD result TAIL
}

# replace "../../../.."
match($0, ggg_parent_pattern) {
	head = substr($0, 0, RSTART + RLENGTH - 13)
	tail = substr($0, RSTART + RLENGTH)
	print insert(head, tail, 4)
	next
}

# replace "../../.."
match($0, gg_parent_pattern) {
	head = substr($0, 0, RSTART + RLENGTH - 10)
	tail = substr($0, RSTART + RLENGTH)
	print insert(head, tail, 3)
	next
}

# replace "../.."
match($0, g_parent_pattern) {
	head = substr($0, 0, RSTART + RLENGTH - 7)
	tail = substr($0, RSTART + RLENGTH)
	print insert(head, tail, 2)
	next
}

# replace ".."
match($0, parent_pattern) {
	head = substr($0, 0, RSTART + RLENGTH - 4)
	tail = substr($0, RSTART + RLENGTH)
	print insert(head, tail, 1)
	next
}

# replace "."
match($0, current_pattern) {
	head = substr($0, 0, RSTART + RLENGTH - 3)
	tail = substr($0, RSTART + RLENGTH)
	print insert(head, tail, 0)
	next
}

{ print }
