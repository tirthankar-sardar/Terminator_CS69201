#!/usr/bin/env bash
set -eu
ROOT="${1:-$PWD/terminator_test_fixtures/task02}"
mkdir -p "$ROOT"
cat > "$ROOT/test_source.sh" <<'EOF'
TASK2_SOURCE_VALUE=source_ok
echo "source script ran"
EOF
printf 'Created fixture: %s/test_source.sh\n' "$ROOT"
cat <<EOF

Run these in Terminator (first: cd "$ROOT"):
  echo TASK2_OK
  pwd
  cd /tmp
  pwd
  cd "$ROOT"
  cd /directory_that_does_not_exist
  notacommand
  false
  echo \$?
  true
  echo \$?
  source test_source.sh
  echo \$TASK2_SOURCE_VALUE
  sleep 30 &
  jobs
  exit

Check: errors do not kill the shell; false then echo \$? prints 1;
true then echo \$? prints 0; source executes in the shell's intended scope.
Run 'exit' only in a test tab because it closes/exits that shell.
EOF
