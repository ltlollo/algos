// This is free and unencumbered software released into the public domain.
// For more information, see LICENSE

#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "mm.h"

#define BENCH(x, t) {x, ""#x, (t) }
#define L3 (2048 * 1024)
#define L2 ( 256 * 1024)

static _Alignas(32) float A[L2 / sizeof (float)];
static _Alignas(32) float B[L3 / sizeof (float)];

struct benchpar {
    float *a, b, c;
    size_t m, k, n, l2, l3;
    size_t times;
};

void
bench_dgemmf32_256x256(size_t times) {
    size_t m, k, n;
    m = n = k = 256;

    float *a = allocmf32(m, k);
    float *b = allocmf32(k, n);
    float *c = allocmf32(m, n);

    iotamf32(0.f, 0.1f, a, m, k);
    iotamf32(3.f, 0.1f, b, k, n);

    for (size_t i = 0; i < times; i++) {
        dgemmf32(a, b, c, m, k, n, A, B, L2, L3);
    }
}

void
bench_mtdgemmf32_256x256(size_t times) {
    size_t m, k, n;
    m = n = k = 256;

    float *a = allocmf32(m, k);
    float *b = allocmf32(k, n);
    float *c = allocmf32(m, n);

    iotamf32(0.f, 0.1f, a, m, k);
    iotamf32(3.f, 0.1f, b, k, n);

    for (size_t i = 0; i < times; i++) {
        mtdgemmf32(a, b, c, m, k, n, L2, L3, 8);
    }
}

void
bench_Tmf32_256x256(size_t times) {
    size_t m, n;
    m = n = 256;

    float *a = allocmf32(m, n);
    float *b = allocmf32(n, m);

    iotamf32(0.f, 1.f, a, m, n);

    for (size_t i = 0; i < times; i++) {
        Tmf32(a, b, m, n);
    }
}

void
bench_dgemv_256x256(size_t times) {
    size_t m, n;
    m = 400;
    n = 400;

    float *a = allocmf32(m, n);
    float *b = allocvf32(n);
    float *c = allocvf32(m);

    iotamf32(0.f, 0.1f, a, m, n);
    iotavf32(3.f, 0.1f, b, n);

    for (size_t i = 0; i < times; i++) {
        dgemvf32(a, b, c, m, n);
    }
}

int
main() {
    struct Bnc {
        void (*fn)(size_t);
        const char *wh;
        size_t times;
    } benchs[] = {
        BENCH(bench_mtdgemmf32_256x256, 1<<12),
        BENCH(bench_dgemmf32_256x256, 1<<11),
        BENCH(bench_Tmf32_256x256, 1<<15),
        BENCH(bench_dgemv_256x256, 1<<14),
        BENCH(NULL, 0),
    };

    struct timespec beg, end;
    for (struct Bnc *bench = benchs; bench->fn; bench++) {
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &beg);
        bench->fn(bench->times);
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
        float ts = ((end.tv_sec * 1000000000 + end.tv_nsec) -
            (beg.tv_sec * 1000000000 + beg.tv_nsec)) * 1e-6;
        printf("[%s]: time: %f ms, n: %ld iter, rate: %f ms/iter\n", bench->wh,
            ts, bench->times, ts / bench->times);
    }
}
