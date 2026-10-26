#!/bin/sh
# Task 11 isolated test: pipeline exit status
# Run this through Terminator's own script interpreter, not macOS /bin/sh.

echo "PIPELINE STATUS TEST 1: false | true"
false | true
echo "Expected status: 0"
echo "Actual status: $?"

echo ""
echo "PIPELINE STATUS TEST 2: true | false"
true | false
echo "Expected status: 1"
echo "Actual status: $?"