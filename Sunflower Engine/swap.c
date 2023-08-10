#include "swap.h"

void int_swap(int* a, int* b) {
    int t = *a;
    *a = *b;
    *b = t;
}

void float_swap(float* a, float* b) {
    float t = *a;
    *a = *b;
    *b = t;
}
