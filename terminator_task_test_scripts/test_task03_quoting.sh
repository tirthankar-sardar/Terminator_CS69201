#!/usr/bin/env bash
set -eu
ROOT="${1:-$PWD/terminator_test_fixtures/task03}"
mkdir -p "$ROOT/subdir"
touch "$ROOT/a.txt" "$ROOT/b.txt" "$ROOT/subdir/c.txt"
printf 'Fixtures created in: %s\n' "$ROOT"
cat <<EOF

Run in Terminator after: cd "$ROOT"
1. echo "Hello
   World"
   Expected: continuation prompt; command runs only after closing quote.
2. echo Hello \\
   World
   Expected: line continuation; check your shell's defined spacing behavior.
3. echo 'literal: \$HOME'
   Expected: \$HOME printed literally.
4. echo "expanded: \$HOME"
   Expected: HOME value printed.
5. echo "\$(basename "\$(pwd)")"
   Expected: prints the current directory's basename.
6. echo "count: \$(ls | wc -l)"
   Expected: prints a count for this directory.
7. echo "A \`printf B\` C"
   Expected: A B C.
8. echo "\$(echo "\$(echo nested)")"
   Expected: nested.
9. Type an unmatched quote and press Enter; verify continuation rather than premature execution.
EOF
