#!/bin/sh
# Task 11 regression test suite for Terminator's built-in script interpreter.
# Run this file using Terminator's interpreter, not the system /bin/sh.
# Example: ./task11_test_suite.sh alpha beta
# Some tests create temporary helper files in the current directory.

echo "========================================"
echo " Terminator Task 11 Test Suite"
echo "========================================"
echo "Run this through Terminator's own script interpreter."
echo

echo "TEST 1: comments and multiple commands"
echo "Expected markers: comment_start and comment_end only"
echo "comment_start"
# echo THIS_SHOULD_NOT_APPEAR
echo "comment_end"
echo

echo "TEST 2: variable assignment and expansion"
NAME=hello_task11
echo "Expected: hello_task11"
echo "$NAME"
echo

echo "TEST 3: positional arguments and argument count"
echo "Argument 1: $1"
echo "Argument 2: $2"
echo "Argument count: $#"
echo "For this test, run with two arguments, e.g. alpha beta."
echo

echo "TEST 4: exit status after success"
true
echo "Expected: 0; actual: $?"
echo

echo "TEST 5: exit status after false"
false
echo "Expected: 1; actual: $?"
echo

echo "TEST 6: command substitution"
WHO=$(whoami)
echo "Expected a username; actual: $WHO"
echo

echo "TEST 7: pipeline status"
false | true
echo "Expected after false | true: 0; actual: $?"
true | false
echo "Expected after true | false: 1; actual: $?"
echo

echo "TEST 8: output redirection and append"
echo "first_line" > task11_redir_test.txt
echo "second_line" >> task11_redir_test.txt
echo "Expected file contents: first_line then second_line"
cat task11_redir_test.txt
echo

echo "TEST 9: input redirection"
cat < task11_redir_test.txt
echo

echo "TEST 10: numeric condition true"
if [ 1 -eq 1 ]; then
    echo "PASS: numeric true condition"
else
    echo "FAIL: numeric true condition"
fi
echo

echo "TEST 11: numeric condition false and else"
if [ 1 -eq 2 ]; then
    echo "FAIL: false condition entered then branch"
else
    echo "PASS: false condition entered else branch"
fi
echo

echo "TEST 12: greater-than condition"
if [ 3 -gt 2 ]; then
    echo "PASS: -gt condition"
else
    echo "FAIL: -gt condition"
fi
echo

echo "TEST 13: string equality"
if [ "abc" = "abc" ]; then
    echo "PASS: string equality"
else
    echo "FAIL: string equality"
fi
echo