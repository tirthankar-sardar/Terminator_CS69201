#!/usr/bin/env bash
set -eu
ROOT="${1:-$PWD/terminator_test_fixtures/task04}"
mkdir -p "$ROOT"
printf 'banana\napple\ncherry\n' > "$ROOT/fruits.txt"
printf 'alpha\nbeta\ngamma\n' > "$ROOT/input.txt"
printf 'Created fixtures in: %s\n' "$ROOT"
cat <<EOF

Run in Terminator after: cd "$ROOT"
1. sort < fruits.txt
   Expected: apple, banana, cherry.
2. echo Hello > output.txt
3. cat output.txt
   Expected: Hello.
4. sort < fruits.txt > sorted.txt
5. cat sorted.txt
   Expected: sorted fruit names.
6. echo Second > output.txt; cat output.txt
   Expected: Second (overwrite, not append).
7. cat < missing.txt
   Expected: missing-file error; shell remains usable.
8. wc -l < input.txt
   Expected: 3.
9. cat < input.txt > copy.txt
   Then compare input.txt and copy.txt outside Terminator with: cmp input.txt copy.txt
EOF
