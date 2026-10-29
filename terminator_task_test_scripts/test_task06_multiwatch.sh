#!/usr/bin/env bash
set -eu
ROOT="${1:-$PWD/terminator_test_fixtures/task06}"
mkdir -p "$ROOT"
printf 'Created test directory: %s\n' "$ROOT"
cat <<'EOF'

Run these in Terminator. Press Ctrl+C to stop each multiWatch test.
1. multiWatch ["date +%T", "ls | wc -l"]
   Expected: both commands produce output; each block has quoted command,
   Unix timestamp, and the required separator lines.
2. multiWatch ["sleep 1; echo A", "sleep 1; echo B"]
   Expected: commands run concurrently, not one strictly after the other.
3. multiWatch ["echo hello", "echo world"]
   Expected: periodic reruns (default interval 2 seconds).
4. multiWatch ["date +%T"]
   Let it run for 6 seconds; expect multiple output cycles.
5. multiWatch ["sleep 100", "sleep 100"]
   Press Ctrl+C. Verify both children stop and the prompt returns.
6. Run: echo MULTIWATCH_SHELL_ALIVE
   Expected: shell is still usable.
EOF
