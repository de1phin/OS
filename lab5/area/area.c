#include "area.h"
#include "stdio.h"

float square(float a, float b) {
    return a * b;
}

float triangle(float a, float b) {
    return a * b / 2;
}

float (*area_func)(float, float) = square;

float area(float a, float b) {
    return area_func(a, b);
}

void area_switch_func() {
    if (area_func == square)
        area_func = triangle;
    else
        area_func = square;
}
