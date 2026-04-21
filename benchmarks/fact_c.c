// fact_c.c - iterative factorial with long long
#include <stdio.h>

long long fact(int n) {
    long long r = 1;
    for (int i = 2; i <= n; i++) {
        r *= i;
    }
    return r;
}

int main() {
    long long sum = 0;
    for (int i = 0; i < 100000; i++) {
        sum += fact(12);
    }
    printf("%lld\n", sum);
    return 0;
}