// fib_c.c - iterative fibonacci with long long
#include <stdio.h>

long long fib(int n) {
    long long a = 0, b = 1;
    for (int i = 0; i < n; i++) {
        long long temp = a;
        a = b;
        b = temp + b;
    }
    return a;
}

int main() {
    long long sum = 0;
    for (int i = 0; i < 100000; i++) {
        sum += fib(30);
    }
    printf("%lld\n", sum);
    return 0;
}