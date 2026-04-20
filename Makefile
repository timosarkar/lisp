all:
		clang++ -O2 a.cpp -o lisp_compiler
		./run_tests.sh

clean:
		rm lisp_compiler
		rm -rf tests/build
