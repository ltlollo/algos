// This is free and unencumbered software released into the public domain.
// For more information, see LICENSE

#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "mm.h"

#define BENCH(x, t) {x, ""#x, t }
#define L3 (2048 * 1024)
#define L2 ( 256 * 1024)

static _Alignas(32) float A[L2 / sizeof (float)];
static _Alignas(32) float B[L3 / sizeof (float)];

void
bench_dgemmf32_256x256(size_t times) {
    size_t m, k, n;
    m = n = k = 256;

    float *a = mmallocf32(m, k);
    float *b = mmallocf32(k, n);
    float *c = mmallocf32(m, n);

    iotamf32(0.f, 0.1f, a, k, m);
    iotamf32(3.f, 0.1f, b, n, k);

    for (size_t i = 0; i < times; i++) {
        dgemmf32(a, b, c, m, k, n, A, B, L2, L3);
    }
}

void
bench_Tmf32_256x256(size_t times) {
    size_t m, n;
    m = n = 256;

    float *a = mmallocf32(m, n);
    float *b = mmallocf32(n, m);

    iotamf32(0.f, 1.f, a, m, n);

    for (size_t i = 0; i < times; i++) {
        Tmf32(a, b, m, n);
    }
}

int
main() {
    struct Bnc {
        void (*fn)(size_t);
        const char *wh;
        size_t times;
    } benchs[] = {
        BENCH(bench_dgemmf32_256x256, 1<<13),
        BENCH(bench_Tmf32_256x256, 1<<13),
        BENCH(NULL, 0),
    };
    struct timespec beg, end;
    char time[64];
    for (struct Bnc *bench = benchs; bench->fn; bench++) {
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &beg);
        bench->fn(bench->times);
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
        float ts = ((end.tv_sec * 1000000000 + end.tv_nsec) -
            (beg.tv_sec * 1000000000 + beg.tv_nsec)) * 0.001f;

        snprintf(time, 64, "%f%s", ts < 10000 ? ts : ts * 0.000001,
            ts < 10000 ? "ms" : "s");

        printf("[%s]: %s, niter: %ld, %f ms/niter\n", time, bench->wh,
            bench->times, ts / bench->times);
    }
}
