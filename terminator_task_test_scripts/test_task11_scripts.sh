#!/usr/bin/env bash
set -eu
ROOT="${1:-$PWD/terminator_test_fixtures/task11}"
mkdir -p "$ROOT/subdir"
printf 'sample\n' > "$ROOT/sample.txt"
cat > "$ROOT/basic.sh" <<'EOF'
# comment test
echo First
echo Second
EOF
cat > "$ROOT/args.sh" <<'EOF'
NAME=Alice
echo "Hello $NAME"
echo "First=$1 Second=$2"
echo "Count=$#"
EOF
cat > "$ROOT/check.sh" <<'EOF'
if [ -f "$1" ]; then
    echo "File exists"
else
    echo "File missing"
fi
EOF
cat > "$ROOT/exit_test.sh" <<'EOF'
echo Before exit
exit 7
echo After exit
EOF
cat > "$ROOT/bad.sh" <<'EOF'
if [ -f "sample.txt" ]; then
    echo "File exists"
else
    echo "File missing"
EOF
printf 'Created Task 11 scripts in: %s\n' "$ROOT"
cat <<EOF

Run these in Terminator after: cd "$ROOT"
1. ./basic.sh
   Expected: First then Second.
2. ./args.sh Bob Charlie
   Expected: Hello Alice; First=Bob Second=Charlie; Count=2.
3. ./check.sh sample.txt
   Expected: File exists.
4. ./check.sh absent.txt
   Expected: File missing.
5. ./exit_test.sh
   Expected: Before exit only; script status should be 7.
6. ./bad.sh
   Expected: syntax error reported BEFORE any script command executes.
7. Create/run additional scripts for -d, -eq, -gt, string equality,
   nested if/elif/else, pipelines, redirection, command substitution,
   and $? as listed in the project specification.
8. Test ./script.sh, bash script.sh, and source script.sh separately.
   The project requirement says your own interpreter must interpret scripts,
   not delegate script execution to /bin/bash.
EOF
