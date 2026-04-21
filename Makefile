# Makefile for lisp compiler

CXX = clang++
CXXFLAGS = -O2 -std=c++17

.PHONY: all build test benchmarks fmt help clean

all: build test

build: lisp_compiler

lisp_compiler: a.cpp tokenizer.cpp tokenizer.h
	$(CXX) $(CXXFLAGS) a.cpp -o lisp_compiler

test: build
	./run_tests.sh

benchmarks: build
	./run_benchmarks.sh

fmt:
	clang-format -i a.cpp tokenizer.cpp tokenizer.h

clean:
	rm -f lisp_compiler a.out *.o *.ll *.s
	rm -f tests/build/* 2>/dev/null || true
	rm -f fact_lisp fib_lisp sieve_lisp hello_lisp string_ops_lisp
	rm -f benchmark.c benchmark.ll benchmark_lisp
	rm -f lisp-pkg lisp-pkg-bin test_ffi
	rm -f /tmp/fact_c /tmp/fib_c /tmp/sieve_c /tmp/*_lisp 2>/dev/null || true

help:
	@echo "Available targets:"
	@echo "  build      - Compile the lisp compiler"
	@echo "  test       - Run the test suite"
	@echo "  benchmarks - Run performance benchmarks"
	@echo "  fmt        - Format source code"
	@echo "  clean      - Remove generated files"