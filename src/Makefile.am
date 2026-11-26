SUBDIRS = common editformats

AM_CPPFLAGS = \
  -I$(top_srcdir)/gnulib-lib \
  -I$(top_builddir)/gnulib-lib
AM_CFLAGS = -Wall

bin_PROGRAMS = qcmd icmd deurlname
dist_man_MANS = qcmd.1 icmd.1 deurlname.1

qcmd_SOURCES = \
  apply.c \
  display.c \
  edit.c \
  list.c \
  interactive.c \
  line-editing.h \
  plan.c \
  qcmd.c \
  qcmd.h \
  quote.c \
  variables.c
qcmd_LDADD = \
  editformats/lib_editformats.a \
  common/lib_common.a \
  ../gnulib-lib/libgnu.a \
  $(LIBINTL) \
  $(READLINE_LIBS)

icmd_SOURCES = \
  icmd.c \
  line-editing.h
icmd_LDADD = \
  common/lib_common.a \
  ../gnulib-lib/libgnu.a \
  $(LIBINTL) \
  $(READLINE_LIBS)

deurlname_SOURCES = \
  deurlname.c
deurlname_LDADD = \
  common/lib_common.a \
  ../gnulib-lib/libgnu.a \
  $(LIBINTL)

# qcmd and icmd decide what to do from the name they're run as.
LINKS = qmv:qcmd qcp:qcmd imv:icmd icp:icmd

all-local:
	@for l in $(LINKS); do \
	  [ -e "$${l%%:*}" ] || $(LN_S) "$${l#*:}" "$${l%%:*}"; \
	done

install-exec-hook:
	@for l in $(LINKS); do \
	  rm -f "$(DESTDIR)$(bindir)/$${l%%:*}"; \
	  $(LN_S) "$${l#*:}" "$(DESTDIR)$(bindir)/$${l%%:*}"; \
	done

install-data-hook:
	@for l in $(LINKS); do \
	  rm -f "$(DESTDIR)$(man1dir)/$${l%%:*}.1"; \
	  $(LN_S) "$${l#*:}.1" "$(DESTDIR)$(man1dir)/$${l%%:*}.1"; \
	done

uninstall-hook:
	@for l in $(LINKS); do \
	  rm -f "$(DESTDIR)$(bindir)/$${l%%:*}" "$(DESTDIR)$(man1dir)/$${l%%:*}.1"; \
	done

clean-local:
	rm -f qmv qcp imv icp
