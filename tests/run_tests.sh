#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
# Run all unit tests for ufs-utils

set -e

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PASS=0
FAIL=0
ERRORS=""

for test_bin in "$SCRIPT_DIR"/test_*; do
    # Skip non-executables and source files
    [ -x "$test_bin" ] || continue
    [[ "$test_bin" == *.c ]] && continue
    [[ "$test_bin" == *.h ]] && continue
    [[ "$test_bin" == *.o ]] && continue

    name=$(basename "$test_bin")
    echo "========================================"
    echo "Running: $name"
    echo "========================================"
    if "$test_bin"; then
        PASS=$((PASS + 1))
    else
        FAIL=$((FAIL + 1))
        ERRORS="$ERRORS  $name\n"
    fi
    echo ""
done

echo "========================================"
echo "Test Summary: $PASS passed, $FAIL failed"
echo "========================================"
if [ $FAIL -gt 0 ]; then
    echo "Failed tests:"
    printf "$ERRORS"
    exit 1
fi
exit 0
