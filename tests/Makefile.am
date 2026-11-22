TESTS = list.sh rename.sh deurlname.sh interactive.sh
EXTRA_DIST = $(TESTS) lib.sh pty-run.py

# The tests find the programs relative to builddir, and lib.sh in srcdir.
AM_TESTS_ENVIRONMENT = builddir='$(builddir)' srcdir='$(srcdir)'; export builddir srcdir;
