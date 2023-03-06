#include "stdlib.h"
#include "stdio.h"
#include "dlfcn.h"

void *arealib, *primeslib;

void (*area_switch_func)() = NULL;
float (*area)(float, float) = NULL;

int (*count_primes)(int, int) = NULL;
void (*primes_switch_func)() = NULL;

void cleanup() {
    dlclose(arealib);
    dlclose(primeslib);
}

void load_libs() {
    arealib = dlopen("./area.so", RTLD_LAZY);
    if (!arealib) {
        printf("failed to load area lib\n");
        exit(1);
    }

    primeslib = dlopen("./primes.so", RTLD_NOW);
    if (!primeslib) {
        dlclose(arealib);
        printf("failed to load primes lib\n");
        exit(1);
    }

    area_switch_func = dlsym(arealib, "area_switch_func");
    if (!area_switch_func) {
        printf("%s\n", dlerror());
        cleanup();
        exit(1);
    }
    
    area = dlsym(arealib, "area");
    if (!area) {
        printf("%s\n", dlerror());
        cleanup();
        exit(1);
    }

    count_primes = dlsym(primeslib, "count_primes");
    if (!count_primes) {
        printf("%s\n", dlerror());
        cleanup();
        exit(1);
    }
    
    primes_switch_func = dlsym(primeslib, "primes_switch_func");
    if (!primes_switch_func) {
        printf("%s\n", dlerror());
        cleanup();
        exit(1);
    }
}

int main() {
    load_libs();

    while (1) {
        int cmd;
        scanf("%d", &cmd);

        switch (cmd) {
        case -1:
            return 0;
        case 0:
            primes_switch_func();
            area_switch_func();
            break;
        case 1: {
            int a, b;
            scanf("%d %d", &a, &b);
            int res = count_primes(a, b);
            printf("%d\n", res);
            break;
        }
        case 2: {
            float a, b;
            scanf("%f %f", &a, &b);
            float res = area(a, b);
            printf("%f\n", res);
            break;
        }
        default:
            printf("unknown command\n");
            break;
        }
    }

    cleanup();

    return 0;
}