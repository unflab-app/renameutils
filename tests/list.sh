#!/bin/sh
# Which files qmv lists, and in what order, for each listing option.
# The expected orders are GNU ls's for the same files.

. "${srcdir:-.}/lib.sh"

mkdir flat tree
cd flat
# name size mtime
for spec in 'a.txt 30 202001010000' 'b.txt 20 202101010000' \
            'C.md 10 202201010000' 'back~ 5 201901010000' \
            'file10 40 202301010000' 'file2.tar.gz 50 202401010000' \
            'file9 60 201801010000' '.dot 1 201701010000'; do
  set -- $spec
  head -c "$2" /dev/zero >"$1"
  touch -t "$3" "$1"
done
touch -a -t 203001010000 C.md
cd ..

listed() {
  rm -f "$EDIT_DIR/edited.before"
  (cd flat && "$QMV" -f destination-only "$@" </dev/null >/dev/null 2>&1)
  cat "$EDIT_DIR/edited.before" 2>/dev/null
}

lines() { printf '%s\n' "$@"; }

check 'default: by name' "$(lines C.md a.txt b.txt back~ file10 file2.tar.gz file9)" "$(listed)"
check '-a: with dot files' "$(lines .dot C.md a.txt b.txt back~ file10 file2.tar.gz file9)" "$(listed -a)"
check '-A: the same as -a' "$(listed -a)" "$(listed -A)"
check '-B: without backups' "$(lines C.md a.txt b.txt file10 file2.tar.gz file9)" "$(listed -B)"
check '-r: reversed' "$(lines file9 file2.tar.gz file10 back~ b.txt a.txt C.md)" "$(listed -r)"
check '-S: largest first' "$(lines file9 file2.tar.gz file10 a.txt b.txt C.md back~)" "$(listed -S)"
check '--sort=size -r' "$(lines back~ C.md b.txt a.txt file10 file2.tar.gz file9)" "$(listed --sort=size -r)"
check '-t: newest first' "$(lines file2.tar.gz file10 C.md b.txt a.txt back~ file9)" "$(listed -t)"
check '-rt: oldest first' "$(lines file9 back~ a.txt b.txt C.md file10 file2.tar.gz)" "$(listed -rt)"
check '-u: by access time' "$(lines C.md file2.tar.gz file10 b.txt a.txt back~ file9)" "$(listed -u)"
check '--time=atime: as -u' "$(listed -u)" "$(listed --time=atime)"
check '-X: by extension' "$(lines back~ file10 file9 file2.tar.gz C.md a.txt b.txt)" "$(listed -X)"
check '--sort=extension: as -X' "$(listed -X)" "$(listed --sort=extension)"
check '--sort=version' "$(lines C.md a.txt b.txt back~ file2.tar.gz file9 file10)" "$(listed --sort=version)"
check '-U: every file, unsorted' "$(listed)" "$(listed -U | LC_ALL=C sort)"
check 'file operands, sorted' "$(lines a.txt file9)" "$(listed file9 a.txt)"
check 'file operands, -S' "$(lines file9 a.txt)" "$(listed -S a.txt file9)"

if (cd flat && "$QMV" --sort=bogus </dev/null >/dev/null 2>&1); then
  fail 'an invalid --sort word is rejected'
else
  pass 'an invalid --sort word is rejected'
fi

cd tree
mkdir -p sub/deep .hid
touch x sub/y sub/deep/z .hid/h
ln -s sub link
cd ..

tree_listed() {
  rm -f "$EDIT_DIR/edited.before"
  (cd tree && "$QMV" -f destination-only "$@" </dev/null >/dev/null 2>&1)
  cat "$EDIT_DIR/edited.before" 2>/dev/null
}

check '-R: depth first, links not followed' \
  "$(lines link sub x sub/deep sub/y sub/deep/z)" "$(tree_listed -R)"
check '-Ra: hidden directories too' \
  "$(lines .hid link sub x .hid/h sub/deep sub/y sub/deep/z)" "$(tree_listed -Ra)"
check '-d: directories as names' "$(lines sub x)" "$(tree_listed -d x sub)"
check 'directory operands are skipped' "$(lines x)" "$(tree_listed x sub)"

rm -f "$EDIT_DIR/edited.before"
"$QMV" -f destination-only tree </dev/null >/dev/null 2>&1
check 'a lone directory is listed relative to itself' \
  "$(lines link sub x)" "$(cat "$EDIT_DIR/edited.before")"

finish
