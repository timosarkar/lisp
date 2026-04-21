#!/bin/bash
# run_benchmarks.sh - compile and run all benchmarks

cd "$(dirname "$0")"

echo "=============================================="
echo "Building lisp compiler..."
rm -f lisp_compiler a.out *.ll 2>/dev/null
clang++ -O2 a.cpp -o lisp_compiler
echo ""

echo "=============================================="
echo "Compiling benchmarks..."
echo ""

# Compile C benchmarks
for f in benchmarks/*_c.c; do
    name=$(basename "$f" .c)
    echo "Compiling C: $name"
    clang -O2 "$f" -o "/tmp/${name}_c" 2>&1 || echo "  C compilation failed"
done
echo ""

# Compile Lisp benchmarks
for f in benchmarks/*_lisp.lisp; do
    name=$(basename "$f" .lisp)
    echo "Compiling Lisp: $name"
    ./lisp_compiler "$f" -o "/tmp/${name}_lisp" 2>&1 || echo "  Lisp compilation failed"
done
echo ""

echo "=============================================="
echo "Running benchmarks..."
echo ""

echo "--- Fibonacci(30) x 100000 iterations ---"
printf "C:       "; /tmp/fib_c 2>/dev/null && echo "OK" || echo "FAILED"
printf "Python:  "; python3 benchmarks/fib_py.py 2>/dev/null && echo "OK" || echo "FAILED"
printf "Lisp:    "; /tmp/fib_lisp 2>/dev/null && echo "OK" || echo "FAILED"

echo ""
echo "--- Factorial(12) x 100000 iterations ---"
printf "C:       "; /tmp/fact_c 2>/dev/null && echo "OK" || echo "FAILED"
printf "Python:  "; python3 benchmarks/fact_py.py 2>/dev/null && echo "OK" || echo "FAILED"
printf "Lisp:    "; /tmp/fact_lisp 2>/dev/null && echo "OK" || echo "FAILED"

echo ""
echo "--- Count primes < 1000 ---"
printf "C:       "; /tmp/sieve_c 2>/dev/null && echo "OK" || echo "FAILED"
printf "Python:  "; python3 benchmarks/sieve_py.py 2>/dev/null && echo "OK" || echo "FAILED"
printf "Lisp:    "; /tmp/sieve_lisp 2>/dev/null && echo "OK" || echo "FAILED"

echo ""
echo "--- String output ---"
printf "Python:  "; python3 benchmarks/string_ops_py.py 2>/dev/null && echo "OK" || echo "FAILED"
printf "Lisp:    "; /tmp/string_ops_lisp 2>/dev/null && echo "OK" || echo "FAILED"

echo ""
echo "=============================================="
echo ""
echo "=== Token Count Analysis ==="
python3 benchmarks/count_tokens.py