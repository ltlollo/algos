// This is free and unencumbered software released into the public domain.
// For more information, see LICENSE

#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "mm.h"
#include "tbh.h"

#define L3 (2048 * 1024 / sizeof (float))
#define L2 ( 512 * 1024 / sizeof (float))

static _Alignas(32) float A[L2];
static _Alignas(32) float B[L3];

struct benchpar {
    size_t times;
    float *a, *b, *c;
    size_t m, k, n, l2, l3;
};

void
pkdgemmf32_512x512(void *p) {
    struct benchpar *in = p;
    float *a = in->a, *b = in->b, *c = in->c;
    size_t m = in->m, k = in->k, n = in->n, l2 = in->l2, l3 = in->l3;
    size_t times = in->times;

    iotamf32(0.f, 0.1f, a, m, k);
    iotamf32(3.f, 0.1f, b, k, n);

    for (size_t i = 0; i < times; i++) {
        pkdgemmf32(a, b, c, m, k, n, A, B, l2, l3);
    }
}

void
dgemmf32_512x512(void *p) {
    struct benchpar *in = p;
    float *a = in->a, *b = in->b, *c = in->c;
    size_t m = in->m, k = in->k, n = in->n, l2 = in->l2, l3 = in->l3;
    size_t times = in->times;

    iotamf32(0.f, 0.1f, a, m, k);
    iotamf32(3.f, 0.1f, b, k, n);

    for (size_t i = 0; i < times; i++) {
        dgemmf32(a, b, c, m, k, n, l2, l3);
    }
}


void
mtdgemmf32_512x512(void *p) {
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
Tmf32_512x512(void *p) {
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
dgemv_512x512(void *p) {
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
    float *a = allocmf32(512, 512);
    float *b = allocmf32(512, 512);
    float *c = allocmf32(512, 512);
    size_t m, k, n;

    m = k = n = 512;

    struct Bnc benchs[] = {
        BENCH(dgemmf32_512x512, &(struct benchpar) {
            1<<7, a, b, c, m, k, n, L2, L3
        }),
        BENCH(mtdgemmf32_512x512, &(struct benchpar) {
            1<<10, a, b, c, m, k, n, L2, L3
        }),
        BENCH(pkdgemmf32_512x512, &(struct benchpar) {
            1<<7, a, b, c, m, k, n, L2, L3
        }),
        BENCH(Tmf32_512x512, &(struct benchpar) {
            1<<13, a, b, c, m, k, n, L2, L3
        }),
        BENCH(dgemv_512x512, &(struct benchpar) {
            1<<14, a, b, c, m, k, n, L2, L3
        }),
        BENCH(NULL, NULL),
    };

    float ts;
    struct timespec s, e;
    static char *fmt = "[%s]: time: %f ms, n: %ld iter, rate: %f ms/iter\n";
    for (struct Bnc *bench = benchs; bench->fn; bench++) {
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &s);
        bench->fn(bench->p);
        struct benchpar *p = bench->p;
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &e);
        ts = (e.tv_sec - s.tv_sec) * 1e3 + (e.tv_nsec - s.tv_nsec) * 1e-6;
        printf(fmt, bench->wh, ts, p->times, ts / p->times);
    }
}
