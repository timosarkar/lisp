#!/usr/bin/env python3
# fact_py.py - iterative factorial

def fact(n):
    r = 1
    for i in range(2, n + 1):
        r *= i
    return r

total = 0
for _ in range(100000):
    total += fact(12)
print(total)