#!/usr/bin/env bash
set -eu
ROOT="${1:-$PWD/terminator_test_fixtures/task07}"
mkdir -p "$ROOT"
printf 'Created test directory: %s\n' "$ROOT"
cat <<'EOF'

MANUAL KEYBOARD TESTS IN TERMINATOR
1. Type echo abcdef; press Left; type X. Verify insertion at cursor.
2. Type a command; Ctrl+A should move cursor to start; Ctrl+E to end.
3. Run echo FIRST and echo SECOND; use Up/Down to browse history.
4. Type a line, move cursor to its middle; Ctrl+U deletes cursor-to-start.
5. Type a line, move cursor to its middle; Ctrl+K deletes cursor-to-end.
6. Type echo hello world; at end press Ctrl+W; previous word should be deleted.
7. Generate output and press Ctrl+L; visible screen clears.
8. On an empty prompt press Ctrl+D; verify documented EOF/tab-close behavior.
9. Test Ctrl+Shift+C and Ctrl+Shift+V with another X11 application.
10. Change a binding using your implemented configuration method, restart,
    and verify it persists. Test invalid/conflicting binding fallback.
EOF
