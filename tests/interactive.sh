#!/bin/sh
# imv, icp and qmv's command mode, typed at through a pseudo-terminal.

. "${srcdir:-.}/lib.sh"

PYTHON=${PYTHON:-python3}
if ! command -v "$PYTHON" >/dev/null 2>&1; then
  echo "skip: no $PYTHON to drive a terminal"
  exit 77
fi
type_into() { "$PYTHON" "$testsrc/pty-run.py" "$@"; }

# Ctrl-A, Ctrl-E, Ctrl-U: start of line, end of line, kill the line.
touch 'a file.txt'
type_into '\x05.bak\r' "$IMV" 'a file.txt' >/dev/null
check 'imv starts with the old name' 'a file.txt.bak' "$(entries)"

type_into '\x01new-\r' "$IMV" 'a file.txt.bak' >/dev/null
check 'imv edits anywhere in the name' 'new-a file.txt.bak' "$(entries)"

type_into '\x15b.txt\r' "$ICP" 'new-a file.txt.bak' >/dev/null
check 'icp copies' "$(printf '%s\n' b.txt 'new-a file.txt.bak')" "$(entries)"

touch -- -dash
out=$(type_into '\x15plain\r' "$IMV" -v -- -dash)
check 'imv: a name starting with -, and mv options' "$(printf '%s\n' b.txt 'new-a file.txt.bak' plain)" "$(entries)"
case $out in
  *'-dash -> plain'*|*"'-dash' -> 'plain'"*|*"renamed '-dash' -> 'plain'"*) pass 'imv passes -v to mv' ;;
  *) fail 'imv passes -v to mv'; printf '%s\n' "$out" ;;
esac

if use_utf8_locale; then
  touch 'café'
  type_into '\x05s\r' "$IMV" 'café' >/dev/null
  if [ -f 'cafés' ]; then pass 'imv: UTF-8 name'; else fail 'imv: UTF-8 name'; entries; fi
  LC_ALL=C
fi

echo keep >keep
echo other >other
type_into '\x15keep\r\x00n\r\x00\x03' "$IMV" other >/dev/null
check 'imv: declining to overwrite keeps both' 'keep other' "$(cat keep) $(cat other)"

type_into '\x03' "$IMV" plain >/dev/null
check 'imv: Ctrl-C changes nothing' plain "$(entries | grep -x plain)"

# Command mode: Ctrl-C abandons the half-typed line, tab completes.
mkdir cm
touch cm/x
out=$(cd cm && type_into 'lis\t\x00\x03\x00hel\t ls\r\x00quit\r' "$QMV" -i .)
case $out in
  *'Listing options:'*) pass 'qmv -i: Ctrl-C abandons the line, tab completes' ;;
  *) fail 'qmv -i: Ctrl-C abandons the line, tab completes'; printf '%s\n' "$out" | tail -20 ;;
esac
case $out in
  *'list help'*|*'list hel'*) fail 'qmv -i: the abandoned line is not run' ;;
  *) pass 'qmv -i: the abandoned line is not run' ;;
esac

finish
