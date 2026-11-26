# Shared setup for the tests, sourced by each one.
#
# Each test runs in a fresh scratch directory, with the programs from
# the build tree and an editor that the test controls through
# $EDIT_SCRIPT, a sed script applied to the edit file.

set -u

: "${builddir:=.}"
: "${srcdir:=.}"
bindir=$(cd "$builddir/../src" && pwd)
testsrc=$(cd "$srcdir" && pwd)

QMV=$bindir/qmv
QCP=$bindir/qcp
IMV=$bindir/imv
ICP=$bindir/icp
DEURLNAME=$bindir/deurlname

failures=0
name=$(basename "$0" .sh)
work=$(mktemp -d "${TMPDIR:-/tmp}/ru-$name.XXXXXX") || exit 99
trap 'rm -rf "$work"' EXIT
# The tests run in $work/t, so the editor's files stay out of listings.
mkdir "$work/t" && cd "$work/t" || exit 99

# Listing order depends on collation; pin it. Tests that need UTF-8
# names switch to a UTF-8 locale with use_utf8_locale.
LC_ALL=C
export LC_ALL
unset VISUAL EDITOR
PAGER=cat
export PAGER

use_utf8_locale() {
  for l in C.UTF-8 C.utf8 en_US.UTF-8 en_US.utf8 en_GB.UTF-8; do
    if locale -a 2>/dev/null | grep -qx "$l"; then
      LC_ALL=$l
      export LC_ALL
      return 0
    fi
  done
  return 1
}

# The editor: copy the edit file to $EDIT_DIR/edited.before, apply
# $EDIT_SCRIPT (if any) with sed, and write it back.
editor=$work/editor
cat >"$editor" <<'EOF'
#!/bin/sh
cp "$1" "$EDIT_DIR/edited.before"
if [ -n "${EDIT_SCRIPT:-}" ]; then
  sed -e "$EDIT_SCRIPT" "$1" >"$1.new" && mv "$1.new" "$1"
fi
EOF
chmod +x "$editor"
EDITOR=$editor
EDIT_DIR=$work
export EDITOR EDIT_DIR

pass() { printf 'ok   %s\n' "$1"; }
fail() { printf 'FAIL %s\n' "$1"; failures=$((failures + 1)); }

# check DESCRIPTION EXPECTED ACTUAL
check() {
  if [ "$2" = "$3" ]; then
    pass "$1"
  else
    fail "$1"
    printf '  expected:\n%s\n  actual:\n%s\n' "$2" "$3" | sed 's/^/    /'
  fi
}

# The directory's entries, one per line, sorted bytewise.
entries() {
  ls -A "${1:-.}" | LC_ALL=C sort
}

finish() {
  [ "$failures" -eq 0 ] && exit 0
  printf '%d failure(s)\n' "$failures"
  exit 1
}
