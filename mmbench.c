// This is free and unencumbered software released into the public domain.
// For more information, see LICENSE

#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "mm.h"

#define BENCH(x, ...) {x, ""#x, (__VA_ARGS__) }
#define L3 (2048 * 1024 / sizeof (float))
#define L2 ( 256 * 1024 / sizeof (float))

static _Alignas(32) float A[L2];
static _Alignas(32) float B[L3];

struct benchpar {
    size_t times;
    float *a, *b, *c;
    size_t m, k, n, l2, l3;
};

void
bench_dgemmf32_256x256(void *p) {
    struct benchpar *in = p;
    float *a = in->a, *b = in->b, *c = in->c;
    size_t m = in->m, k = in->k, n = in->n, l2 = in->l2, l3 = in->l3;
    size_t times = in->times;

    iotamf32(0.f, 0.1f, a, m, k);
    iotamf32(3.f, 0.1f, b, k, n);

    for (size_t i = 0; i < times; i++) {
        dgemmf32(a, b, c, m, k, n, A, B, l2, l3);
    }
}

void
bench_mtdgemmf32_256x256(void *p) {
    struct benchpar *in = p;
    float *a = in->a, *b = in->b, *c = in->c;
    size_t m = in->m, k = in->k, n = in->n, l2 = in->l2, l3 = in->l3;
    size_t times = in->times;

    iotamf32(0.f, 0.1f, a, m, k);
    iotamf32(3.f, 0.1f, b, k, n);

    for (size_t i = 0; i < times; i++) {
        mtdgemmf32(a, b, c, m, k, n, l2, l3, 8);
    }
}

void
bench_Tmf32_256x256(void *p) {
    struct benchpar *in = p;
    float *a = in->a, *b = in->b;
    size_t m = in->m, n = in->n;
    size_t times = in->times;

    iotamf32(0.f, 1.f, a, m, n);

    for (size_t i = 0; i < times; i++) {
        Tmf32(a, b, m, n);
    }
}

void
bench_dgemv_256x256(void *p) {
    struct benchpar *in = p;
    float *a = in->a, *b = in->b, *c = in->c;
    size_t m = in->m, n = in->n;
    size_t times = in->times;

    iotamf32(0.f, 0.1f, a, m, n);
    iotavf32(3.f, 0.1f, b, n);

    for (size_t i = 0; i < times; i++) {
        dgemvf32(a, b, c, m, n);
    }
}

int
main() {
    float *a = allocmf32(400, 400);
    float *b = allocmf32(400, 400);
    float *c = allocmf32(400, 400);
    size_t m, k, n;

    m = k = n = 400;

    struct Bnc {
        void (*fn)(void *);
        const char *wh;
        void *p;
    } benchs[] = {
        BENCH(bench_dgemmf32_256x256,   &(struct benchpar){
            1<<11, a, b, c, m, k, n, L2, L3
        }),
        BENCH(bench_mtdgemmf32_256x256, &(struct benchpar){
            1<<12, a, b, c, m, k, n, L2, L3
        }),
        BENCH(bench_Tmf32_256x256,      &(struct benchpar){
            1<<15, a, b, c, m, k, n, L2, L3
        }),
        BENCH(bench_dgemv_256x256,      &(struct benchpar){
            1<<14, a, b, c, m, k, n, L2, L3
        }),
        BENCH(NULL, NULL),
    };

    struct timespec beg, end;
    for (struct Bnc *bench = benchs; bench->fn; bench++) {
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &beg);
        bench->fn(bench->p);
        struct benchpar *p = bench->p;
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &end);
        float ts = ((end.tv_sec * 1000000000 + end.tv_nsec) -
            (beg.tv_sec * 1000000000 + beg.tv_nsec)) * 1e-6;
        printf("[%s]: time: %f ms, n: %ld iter, rate: %f ms/iter\n", bench->wh,
            ts, p->times, ts / p->times);
    }
}
