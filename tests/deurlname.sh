#!/bin/sh
. "${srcdir:-.}/lib.sh"

touch 'a%20b' 'caf%C3%A9' 'slash%2Fnot' 'plain' 'bad%zz'
"$DEURLNAME" 'a%20b' 'caf%C3%A9' 'slash%2Fnot' 'plain' 'bad%zz'
check 'decodes, leaving %2F and invalid escapes' \
  "$(printf '%s\n' 'a b' 'bad%zz' "$(printf 'caf\303\251')" plain 'slash%2Fnot')" "$(entries)"

echo keep >'x y'
echo new >'x%20y'
if "$DEURLNAME" 'x%20y' 2>/dev/null; then
  fail 'refuses to replace an existing file'
else
  pass 'refuses to replace an existing file'
fi
check 'the existing file is untouched' keep "$(cat 'x y')"
check 'the source is left in place' new "$(cat 'x%20y')"

mkdir dir
touch 'dir/p%21'
"$DEURLNAME" 'dir/p%21'
check 'decodes only the last component' 'p!' "$(entries dir)"

finish
