#!/usr/bin/env python3
# fib_py.py - iterative fibonacci

def fib(n):
    a, b = 0, 1
    for _ in range(n):
        a, b = b, a + b
    return a

total = 0
for _ in range(100000):
    total += fib(30)
print(total)