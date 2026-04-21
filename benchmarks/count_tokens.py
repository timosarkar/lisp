#!/usr/bin/env python3
"""Token counter for various languages"""

import sys
import re
from pathlib import Path

def count_tokens_c(code):
    """Count C tokens - comments, keywords, identifiers, operators, numbers"""
    # Remove comments
    code = re.sub(r'//.*?$', '', code, flags=re.MULTILINE)
    code = re.sub(r'/\*.*?\*/', '', code, flags=re.DOTALL)

    # Simple tokenizer: split on whitespace and operators/punctuation
    tokens = re.findall(r"[A-Za-z_][A-Za-z0-9_]*|--?\d+\.?\d*|==|!=|<=|>=|->|[+\-*/%<>=!&|^~?:;,.()\[\]{}\']", code)
    return len(tokens)

def count_tokens_python(code):
    """Count Python tokens using simple regex"""
    # Remove comments
    code = re.sub(r'#.*$', '', code, flags=re.MULTILINE)

    # Token-like chunks
    tokens = re.findall(r"[A-Za-z_][A-Za-z0-9_]*|-?\d+\.?\d*|==|!=|<=|>=|->|[+\-*/%<>=!@,~:;,.()\[\]{}]+", code)
    return len(tokens)

def count_tokens_lisp(code):
    """Count Lisp tokens - identifiers, numbers, operators, parens"""
    # Remove comments
    code = re.sub(r';.*$', '', code, flags=re.MULTILINE)

    # Tokenize: identifiers, numbers, operators, parens
    tokens = re.findall(r"[A-Za-z_][A-Za-z0-9_\-\?]*|-?\d+\.?\d*|[+\-*/%<>=!]+|[()]+", code)
    return len(tokens)

def main():
    benchmarks_dir = Path("benchmarks")

    print("=" * 70)
    print(f"{'Language':<15} {'File':<25} {'Tokens':<10}")
    print("=" * 70)

    # C files
    for f in sorted(benchmarks_dir.glob("*_c.c")):
        code = f.read_text()
        tokens = count_tokens_c(code)
        print(f"{'C':<15} {f.name:<25} {tokens:<10}")

    # Python files
    for f in sorted(benchmarks_dir.glob("*_py.py")):
        code = f.read_text()
        tokens = count_tokens_python(code)
        print(f"{'Python':<15} {f.name:<25} {tokens:<10}")

    # Lisp files
    for f in sorted(benchmarks_dir.glob("*_lisp.lisp")):
        code = f.read_text()
        tokens = count_tokens_lisp(code)
        print(f"{'Lisp':<15} {f.name:<25} {tokens:<10}")

    print("=" * 70)

    # Summary comparison
    print("\nToken Count Summary:")
    print("-" * 40)

    benchmarks = ["fact", "fib", "sieve"]
    for bench in benchmarks:
        c_file = benchmarks_dir / f"{bench}_c.c"
        py_file = benchmarks_dir / f"{bench}_py.py"
        lisp_file = benchmarks_dir / f"{bench}_lisp.lisp"

        if all(f.exists() for f in [c_file, py_file, lisp_file]):
            c_tok = count_tokens_c(c_file.read_text())
            py_tok = count_tokens_python(py_file.read_text())
            lisp_tok = count_tokens_lisp(lisp_file.read_text())

            print(f"{bench.upper()}:")
            print(f"  C:      {c_tok:4d} tokens")
            print(f"  Python: {py_tok:4d} tokens")
            print(f"  Lisp:   {lisp_tok:4d} tokens")
            print(f"  Ratio C/Lisp:   {c_tok/lisp_tok:.1f}x")
            print(f"  Ratio Python/Lisp: {py_tok/lisp_tok:.1f}x")
            print()

if __name__ == "__main__":
    main()