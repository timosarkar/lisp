// sieve_c.c - sieve of eratosthenes to find primes up to N
#include <stdio.h>
#include <string.h>

#define N 10000
char is_prime[N + 1];

int main() {
    int count = 0;
    for (int i = 2; i <= N; i++) is_prime[i] = 1;

    for (int i = 2; i * i <= N; i++) {
        if (is_prime[i]) {
            for (int j = i * i; j <= N; j += i) {
                is_prime[j] = 0;
            }
        }
    }

    for (int i = 2; i <= N; i++) {
        if (is_prime[i]) count++;
    }
    printf("%d\n", count);
    return 0;
}