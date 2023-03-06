#include "primes/primes.h"
#include "area/area.h"
#include "stdio.h"

int main() {
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

    return 0;
}