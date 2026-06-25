#!/bin/bash
set -euo pipefail

# tests/hulk/run_tests.sh - Compatibility shim inspired by the matcom course runner
# Behavior:
#  - Build the project if no ./hulk binary is found (attempts "make build" or "make")
#  - Runs ./hulk against every .hulk file in tests/input/
#  - Returns 0 if all executions succeeded (exit code 0); non-zero otherwise

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"
cd "$REPO_ROOT"

# Try to build if binary is missing
if [ ! -x "./hulk" ]; then
    if command -v make >/dev/null 2>&1; then
        echo "Building project with make..."
        if make build 2>&1 || make 2>&1; then
            echo "Build finished"
        else
            echo "Build failed, but continuing to check for binary..." >&2
        fi
    fi
fi

if [ ! -x "./hulk" ]; then
    echo "Error: ./hulk binary not found. Please build the project first." >&2
    exit 65
fi

FAIL=0
for f in tests/input/*.hulk; do
    [ -e "$f" ] || continue
    echo "---- Running: $f ----"
    # Run the binary; we don't assert program output here, only that it runs without crashing
    if ./hulk "$f" >/dev/null 2>&1; then
        echo "OK: $f"
    else
        echo "FAIL: $f" >&2
        FAIL=1
    fi
done

if [ "$FAIL" -eq 0 ]; then
    echo "All tests executed successfully (no output assertions)"
    exit 0
else
    echo "Some tests failed (runtime errors)." >&2
    exit 1
fi
