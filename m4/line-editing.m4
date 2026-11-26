# RU_LINE_EDITING
# ---------------
# Choose the line-editing library: GNU readline, or libedit.
#
# Both offer the readline API that qcmd's interactive mode uses, but
# libedit's emulation can't pre-fill a line, which icmd needs, so icmd
# drives libedit through its own interface instead. Which one was found
# is recorded as USE_GNU_READLINE or USE_LIBEDIT.
#
# A libedit build that installs <readline/readline.h> (macOS does)
# would satisfy a plain readline check, so GNU readline is recognised
# by rl_gnu_readline_p, which libedit doesn't define.
#
# Sets READLINE_LIBS.
AC_DEFUN([RU_LINE_EDITING], [
  AC_ARG_WITH([readline],
    [AS_HELP_STRING([--with-readline=@<:@auto|gnu|libedit@:>@],
      [line-editing library to use (default: GNU readline if present, otherwise libedit)])],
    [], [with_readline=auto])
  AS_CASE([$with_readline],
    [auto|gnu|libedit], [],
    [AC_MSG_ERROR([--with-readline must be auto, gnu or libedit])])

  ru_line_editing=
  ru_save_LIBS=$LIBS

  AS_IF([test "$with_readline" != libedit], [
    AC_MSG_CHECKING([for GNU readline])
    for ru_termlib in '' -ltinfo -lncurses -ltermcap; do
      LIBS="$ru_save_LIBS -lreadline $ru_termlib"
      AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#include <stdio.h>
#include <readline/readline.h>
#include <readline/history.h>
]], [[add_history ("x"); return rl_gnu_readline_p;]])],
        [ru_line_editing=gnu; READLINE_LIBS="-lreadline $ru_termlib"; break])
    done
    LIBS=$ru_save_LIBS
    AS_IF([test "$ru_line_editing" = gnu],
      [AC_MSG_RESULT([yes ($READLINE_LIBS)])],
      [AC_MSG_RESULT([no])])
  ])

  AS_IF([test -z "$ru_line_editing" && test "$with_readline" != gnu], [
    AC_MSG_CHECKING([for libedit])
    LIBS="$ru_save_LIBS -ledit"
    AC_LINK_IFELSE([AC_LANG_PROGRAM([[
#include <stdio.h>
#include <editline/readline.h>
#include <histedit.h>
]], [[EditLine *e = el_init ("x", stdin, stdout, stderr);
el_push (e, "x");
add_history ("x");
return readline ("x") == NULL;]])],
      [ru_line_editing=libedit; READLINE_LIBS=-ledit])
    LIBS=$ru_save_LIBS
    AS_IF([test "$ru_line_editing" = libedit],
      [AC_MSG_RESULT([yes ($READLINE_LIBS)])],
      [AC_MSG_RESULT([no])])
  ])

  AS_CASE([$ru_line_editing],
    [gnu], [AC_DEFINE([USE_GNU_READLINE], [1], [Define to use GNU readline.])],
    [libedit], [AC_DEFINE([USE_LIBEDIT], [1], [Define to use libedit.])],
    [AC_MSG_ERROR([neither GNU readline nor libedit was found (see --with-readline)])])
  AC_SUBST([READLINE_LIBS])
])
