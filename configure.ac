# Process this file with autoconf to produce a configure script.
# From a git checkout, run ./bootstrap first.

AC_PREREQ([2.71])
AC_INIT([renameutils],
        m4_esyscmd([build-aux/git-version]),
        [https://github.com/unflab-app/renameutils/issues],
        [renameutils],
        [https://github.com/unflab-app/renameutils])
AC_CONFIG_SRCDIR([src/qcmd.h])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_AUX_DIR([build-aux])
AC_CONFIG_MACRO_DIRS([m4 gnulib-m4])
AM_INIT_AUTOMAKE([1.16 -Wall dist-xz subdir-objects])
AM_SILENT_RULES([yes])

AC_PROG_CC
gl_EARLY
AM_PROG_AR
AC_PROG_RANLIB
AC_PROG_LN_S
gl_INIT

# Translations. autoreconf runs autopoint, which installs the po/
# infrastructure of exactly this gettext version from its archive, so
# every machine bootstraps the same files. (gettext 1.0's needs GNU
# make 4, and macOS still ships make 3.81.)
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.21])

RU_LINE_EDITING

AC_CHECK_TYPES([comparison_fn_t], [], [], [[#include <stdlib.h>]])

AC_CONFIG_FILES([Makefile
                 po/Makefile.in
                 gnulib-po/Makefile.in
                 gnulib-lib/Makefile
                 src/Makefile
                 src/common/Makefile
                 src/editformats/Makefile
                 tests/Makefile
                 renameutils.spec])
AC_OUTPUT
