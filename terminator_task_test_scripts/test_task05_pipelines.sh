#!/usr/bin/env bash
set -eu
ROOT="${1:-$PWD/terminator_test_fixtures/task05}"
mkdir -p "$ROOT"
printf 'banana\napple\ncherry\n' > "$ROOT/fruits.txt"
printf 'Created fixtures in: %s\n' "$ROOT"
cat <<EOF

Run in Terminator after: cd "$ROOT"
1. printf "a\\nb\\nc\\n" | wc -l
   Expected: 3.
2. cat fruits.txt | sort
   Expected: apple, banana, cherry.
3. cat fruits.txt | sort | head -2
   Expected: apple and banana.
4. printf "hello\\n" | tr a-z A-Z | wc -l
   Expected: 1.
5. cat < fruits.txt | sort > result.txt
   Then: cat result.txt
   Expected: sorted fruit names.
6. printf "one\\ntwo\\n" | cat | cat | wc -l
   Expected: 2.
7. seq 1 100000 | wc -l
   Expected: 100000 and no hang.
8. cat missing.txt | wc -l
   Expected: input error, then shell remains usable.
EOF
