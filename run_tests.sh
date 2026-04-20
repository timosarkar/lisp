#!/bin/bash
# Test suite for lisp compiler

COMPILER="./lisp_compiler"
BUILD_DIR="tests/build"
PASS=0
FAIL=0

# Create build directory
mkdir -p "$BUILD_DIR"

run_test() {
    local name=$1
    local expected=$2
    local file=$3
    local binary="$BUILD_DIR/$name"

    echo -n "Testing $name... "

    # Compile
    if ! $COMPILER "$file" -o "$binary" > /dev/null 2>&1; then
        echo "FAIL (compile error)"
        ((FAIL++))
        return
    fi

    # Run
    output=$("$binary" 2>&1)
    local status=$?

    if [ $status -ne 0 ]; then
        echo "FAIL (non-zero exit: $status)"
        ((FAIL++))
        return
    fi

    # For tests with empty expected, just check no crash
    if [ -n "$expected" ]; then
        output=$(echo "$output" | tr -d '\n\r')
        expected=$(echo "$expected" | tr -d '\n\r')

        if [ "$output" = "$expected" ]; then
            echo "PASS"
            ((PASS++))
        else
            echo "FAIL (got '$output', expected '$expected')"
            ((FAIL++))
        fi
    else
        echo "PASS"
        ((PASS++))
    fi
}

# Rebuild compiler
echo "Building compiler..."
clang++ -O2 a.cpp -o lisp_compiler || exit 1
echo ""

# Run tests
echo "=== Running test suite ==="
echo ""

# Simple tests
run_test "simple_add" "3" tests/simple_add.lisp
run_test "test_var" "10" tests/test_var.lisp
run_test "test_two_vars" "30" tests/test_two_vars.lisp

# Operators and comparisons
run_test "test_lt" "1" tests/test_lt.lisp
run_test "test_gt" "1" tests/test_gt.lisp
run_test "test_eq" "1" tests/test_eq.lisp
run_test "test_neq" "1" tests/test_neq.lisp

# Conditionals
run_test "test_if_true" "100" tests/test_if_true.lisp
run_test "test_if_false" "200" tests/test_if_false.lisp

# Types
run_test "test_float" "10.000000" tests/test_float.lisp
run_test "test_char" "a" tests/test_char.lisp
run_test "test_string" "hello world" tests/test_string.lisp

# Complex expressions
run_test "test_nested" "45" tests/test_nested.lisp

# Cond
run_test "test_cond_true" "7" tests/test_cond_true.lisp

# Loop - just verify it runs without crash
run_test "test_loop" "" tests/test_loop.lisp

# While
run_test "test_while" "5" tests/test_while.lisp

# For
run_test "test_for" "5" tests/test_for.lisp

# Macros
run_test "test_macro" "6" tests/test_macro.lisp
run_test "test_macro2" "7" tests/test_macro2.lisp

# LLVM IR
run_test "test_raw_ir" "3" tests/test_raw_ir.lisp

# Import
run_test "test_import" "10" tests/test_import.lisp

# FFI
run_test "test_extern" "-1" tests/test_extern.lisp

echo ""
echo "=== Results ==="
echo "PASSED: $PASS"
echo "FAILED: $FAIL"
echo ""

if [ $FAIL -eq 0 ]; then
    echo "All tests passed!"
    exit 0
else
    echo "Some tests failed."
    exit 1
fi
