#!/usr/bin/env python3
# sieve_py.py - sieve of eratosthenes

N = 10000
is_prime = [True] * (N + 1)
is_prime[0] = is_prime[1] = False

for i in range(2, int(N**0.5) + 1):
    if is_prime[i]:
        for j in range(i*i, N + 1, i):
            is_prime[j] = False

count = sum(1 for i in range(2, N + 1) if is_prime[i])
print(count)