#!/bin/sh
# Renaming and copying through the editor.

. "${srcdir:-.}/lib.sh"

fresh() {
  rm -rf d
  mkdir d
  for f in "$@"; do printf '%s\n' "$f" >"d/$f"; done
}

qmv() { (cd d && "$QMV" -f destination-only "$@" . </dev/null >/dev/null 2>&1); }
content() { cat "d/$1" 2>/dev/null; }

fresh a b
EDIT_SCRIPT='s/^a$/z/' qmv
check 'rename' "$(printf '%s\n' b z)" "$(entries d)"
check 'rename keeps the contents' a "$(content z)"

# On a case-insensitive file system, A is found to exist already:
# it's a itself, and the rename has to go ahead.
fresh a b
EDIT_SCRIPT='s/^a$/A/' qmv
check 'a case-only rename' "$(printf '%s\n' A b)" "$(entries d)"

# Two hard links to one file: renaming one onto the other is refused.
fresh a
ln d/a d/b
EDIT_SCRIPT='s/^a$/b/' qmv
check 'renaming onto another link to the same file changes nothing' "$(printf '%s\n' a b)" "$(entries d)"

fresh a b
EDIT_SCRIPT='s/^a$/T/; s/^b$/a/; s/^T$/b/' qmv
check 'swap: files' "$(printf '%s\n' a b)" "$(entries d)"
check 'swap: contents' "b a" "$(content a) $(content b)"

fresh a b c
EDIT_SCRIPT='1s/.*/b/; 2s/.*/c/; 3s/.*/a/' qmv
check 'cycle a->b->c->a' "c a b" "$(content a) $(content b) $(content c)"

fresh a b
EDIT_SCRIPT='s/^a$/b/' qmv
check 'renaming onto an existing file changes nothing' "a b" "$(content a) $(content b)"

fresh a b
EDIT_SCRIPT='s/^a$/sub\/a/' qmv
check 'renaming into a missing directory changes nothing' "$(printf '%s\n' a b)" "$(entries d)"

fresh a
(cd d && EDIT_SCRIPT='s/^a$/a2/' "$QCP" -f destination-only . </dev/null >/dev/null 2>&1)
check 'qcp copies' "a a" "$(content a) $(content a2)"

fresh a
EDIT_SCRIPT='' qmv
check 'no edit, no change' a "$(entries d)"

# Each format, renaming a.txt to z.txt.
for format in dual-column single-column destination-only; do
  case $format in
    dual-column) script='s/a\.txt$/z.txt/' ;;
    single-column) script='n; s/^a\.txt$/z.txt/' ;;
    destination-only) script='s/^a\.txt$/z.txt/' ;;
  esac
  fresh a.txt 'b c.txt'
  (cd d && EDIT_SCRIPT=$script "$QMV" -f "$format" . </dev/null >/dev/null 2>&1)
  check "format $format" "$(printf '%s\n' 'b c.txt' z.txt)" "$(entries d)"
done

# Awkward names survive the trip through the editor: each gets .x.
if use_utf8_locale; then
  rm -rf d
  mkdir d
  names='café.txt|日本語|emoji 😀|back\slash|-dash|  spaces  |tab	here'
  (IFS='|'; for n in $names; do printf '%s' "$n" >"d/$n"; done)
  printf 'nl' >"d/new
line"
  EDIT_SCRIPT='s/$/.x/' qmv
  ok=true
  (IFS='|'; for n in $names; do [ -f "d/$n.x" ] && [ "$(cat "d/$n.x")" = "$n" ] || exit 1; done) || ok=false
  [ -f "d/new
line.x" ] || ok=false
  if $ok; then pass 'UTF-8, control characters and odd names'; else fail 'UTF-8, control characters and odd names'; entries d; fi

  check 'UTF-8 names appear as themselves in the editor' 1 "$(grep -c '^café\.txt$' "$EDIT_DIR/edited.before")"
  check 'control characters are escaped in the editor' 1 "$(grep -c '^new\\nline$' "$EDIT_DIR/edited.before")"
else
  printf 'skip UTF-8 names: no UTF-8 locale\n'
fi

# The editor's failure stops everything, and is reported sensibly.
fresh a
out=$(cd d && EDITOR=false "$QMV" . </dev/null 2>&1)
case $out in
  *'exited with status 1'*) pass 'a failing editor is reported' ;;
  *) fail 'a failing editor is reported'; printf '%s\n' "$out" ;;
esac
check 'a failing editor changes nothing' a "$(entries d)"

# An empty VISUAL falls through to EDITOR.
fresh a
(cd d && VISUAL= EDIT_SCRIPT='s/^a$/z/' "$QMV" -f destination-only . </dev/null >/dev/null 2>&1)
check 'an empty VISUAL is ignored' z "$(entries d)"

finish
