#!/usr/bin/env bash
set -eu
ROOT="${1:-$PWD/terminator_test_fixtures/task08}"
mkdir -p "$ROOT"
printf 'Created test directory: %s\n' "$ROOT"
cat <<'EOF'

Run interactively in Terminator:
1. sleep 100
   Press Ctrl+C. Prompt should return; Terminator stays alive.
2. Run: echo JOB_CONTROL_ALIVE
   Expected: JOB_CONTROL_ALIVE.
3. Run: sleep 100
   Press Ctrl+Z. Then run jobs; the job should be stopped.
4. Run bg %1 (replace 1 with the actual job ID); verify it resumes.
5. Run fg %1; press Ctrl+C; verify the foreground job terminates.
6. Run sleep 10 &; then jobs. Shell should accept another command.
7. Start jobs in two tabs. Press Ctrl+C in one tab; the other tab's job
   must not be affected.
8. Repeat in process mode and verify tab/job isolation.
EOF
