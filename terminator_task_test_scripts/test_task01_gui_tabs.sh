#!/usr/bin/env bash
set -eu
ROOT="${1:-$PWD/terminator_test_fixtures/task01}"
mkdir -p "$ROOT"
printf 'Task 1 fixtures created in: %s\n' "$ROOT"
cat <<'EOF'

MANUAL TESTS IN TERMINATOR GUI
1. Launch Terminator; confirm the X11 window and prompt appear.
2. Type: echo TASK1_OK
   Expected: TASK1_OK appears in the terminal window.
3. Type: pwd
   Expected: current directory is displayed in the window.
4. Open a second tab; run: echo SECOND_TAB_OK
   Expected: output appears in tab 2.
5. In tab 1 run: sleep 5
   Switch to tab 2 while it runs; tab 2 should remain responsive.
6. Generate output: i=1; while [ "$i" -le 40 ]; do echo "scroll-line-$i"; i=$((i+1)); done
   Expected: output can be scrolled if it exceeds the visible area.
7. Close one tab and verify another tab still works.
8. Leave the GUI idle for 30 seconds; it should remain responsive.
EOF
