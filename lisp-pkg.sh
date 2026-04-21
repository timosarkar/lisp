#!/bin/bash
# lisp-pkg - CLI wrapper for the lisp package manager

# Find the lisp compiler
if [ ! -f "./lisp_compiler" ]; then
    if [ -f "a.cpp" ]; then
        echo "Building lisp compiler..."
        clang++ -O2 a.cpp -o lisp_compiler
    fi
fi

# Run the package manager
if [ -f "lisp-pkg.lisp" ]; then
    # Compile if needed
    if [ ! -f "lisp-pkg-bin" ]; then
        ./lisp_compiler lisp-pkg.lisp -o lisp-pkg-bin
    fi
    ./lisp-pkg-bin "$@"
else
    echo "lisp-pkg.lisp not found"
    exit 1
fi