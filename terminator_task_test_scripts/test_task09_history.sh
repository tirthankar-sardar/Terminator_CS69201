#!/usr/bin/env bash
set -eu
ROOT="${1:-$PWD/terminator_test_fixtures/task09}"
mkdir -p "$ROOT"
# Generate input lines for controlled manual/automation submission to Terminator.
i=1
: > "$ROOT/history_commands.txt"
while [ "$i" -le 10100 ]; do
  printf 'echo history_test_%s\n' "$i" >> "$ROOT/history_commands.txt"
  i=$((i+1))
done
printf 'Created %s/history_commands.txt (10100 commands)\n' "$ROOT"
cat <<EOF

Run in Terminator:
1. Execute several unique commands, then history. Recent commands should appear.
2. Open a second tab and run history; it should show commands from tab 1.
3. Restart Terminator; history should persist.
4. Use Ctrl+R, search for a recent exact command, press Enter.
   Expected: most recent exact match.
5. Search for a term with a shared substring longer than 2 characters but
   no exact match; expected longest-common-substring match(es).
6. Search for xyz when absent; expected:
   No match for search term in history
7. For limit testing, submit the lines in history_commands.txt THROUGH
   Terminator (not through your regular shell). Then verify the persistent
   history retains the last 10000 commands and 'history' displays the last 1000.
   Use a disposable test history file/setup; this generates many commands.
EOF
