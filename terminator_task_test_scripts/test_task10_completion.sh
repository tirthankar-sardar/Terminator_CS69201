#!/usr/bin/env bash
set -eu
ROOT="${1:-$PWD/terminator_test_fixtures/task10}"
mkdir -p "$ROOT/subdir"
: > "$ROOT/abc.txt"
: > "$ROOT/abcd.txt"
: > "$ROOT/def.txt"
: > "$ROOT/subdir/file.txt"
printf 'Created completion fixtures in: %s\n' "$ROOT"
cat <<EOF

Run in Terminator after: cd "$ROOT"
1. Type: ./myprog de then press Tab.
   Expected: ./myprog def.txt
2. Type: ./myprog abc then press Tab.
   Expected: common prefix remains abc; ambiguity list offers abc.txt and abcd.txt.
3. Choose option 1; expected token becomes abc.txt.
4. Type: ./myprog xyz then press Tab.
   Expected: no change.
5. Type: ./myprog abc extra, place cursor after abc, press Tab.
   Expected: completion changes only the token and preserves the rest of the line.
6. Type: ./myprog subdir/fi then press Tab.
   Expected: path-aware completion to subdir/file.txt if supported by your implementation.
7. Repeat in two tabs with different current directories; each tab should complete
   using its own current directory.
EOF
