#include "primes.h"
#include "stdlib.h"
#include "stdbool.h"

int is_prime(int x) {
    if (x == 1) return 0;
    for (int a = 2; a*a <= x; a++) {
        if (x % a == 0)
            return 0;
    }
    return 1;
}

int brute(int a, int b) {
    int result = 0;
    for (int i = a; i <= b; i++)
        result += is_prime(i);
    return result;
}

int eratosthenes(int a, int b) {
    int* prime = malloc(sizeof(int)*(b+1));
    prime[0] = 0;
    for (int i = 1; i <= b; i++)
        prime[i] = 1;
    
    int result = 0;
    for (int i = 2; i <= b; i++) {
        if (!prime[i])
            continue;
        if (i >= a)
            result++;
        for (int x = i+i; x <= b; x += i)
            prime[x] = 0;
    }

    free(prime);
    return result;
}

int (*primes_func)(int, int) = brute;

int count_primes(int a, int b) {
    return primes_func(a, b);
}

void primes_switch_func() {
    if (primes_func == brute)
        primes_func = eratosthenes;
    else
        primes_func = brute;
}
