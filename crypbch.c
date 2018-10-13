// This is free and unencumbered software released into the public domain.
// For more information, see LICENSE

#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "cryp.h"

#define BENCH(x, ...) {x, ""#x, (__VA_ARGS__) }

struct benchpar {
    size_t times;
    __m128i a, b;
};

void
mulgf2e128mod0x87_1_2(void *p) {
    struct benchpar *in = p;
    __m128i a = in->a, b = in->b;
    size_t times = in->times;

    for (size_t i = 0; i < times; i++) {
        a = mulgf2e128mod0x87bitrev(a, b);
    }
}

int
main() {
    __m128i a = _mm_set1_epi32(1), b = _mm_set1_epi32(2);

    struct Bnc {
        void (*fn)(void *);
        const char *wh;
        void *p;
    } benchs[] = {
        BENCH(mulgf2e128mod0x87_1_2, &(struct benchpar) { 1<<26, a, b }),
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
