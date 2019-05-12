// This is free and unencumbered software released into the public domain.
// For more information, see LICENSE

#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "cryp.h"
#include "tbh.h"

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

    struct Bnc benchs[] = {
        BENCH(mulgf2e128mod0x87_1_2, &(struct benchpar) { 1<<26, a, b }),
        BENCH(NULL, NULL),
    };
    bench(__FILE__, benchs);
}
