// This is free and unencumbered software released into the public domain.
// For more information, see LICENSE

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "cryp.h"

#define TEST(x) {x, ""#x }

int
muli128_0(void) {
    __m128i a, b;
    __m256i c, r;

    a = _mm_set_epi32(0, 0, 0, 0);
    b = _mm_set_epi32(0, 0, 0, 0);
    c = _mm256_set_epi32(0, 0, 0, 0, 0, 0, 0, 0);
    r  = muli128(a, b);

    return _mm256_movemask_epi8(_mm256_cmpeq_epi32(r, c)) == -1;
}

int
muli128_1(void) {
    __m128i a, b;
    __m256i c, r;

    a = _mm_set_epi32(0, 0, 1, 0);
    b = _mm_set_epi32(0, 0, 1, 1);
    c = _mm256_set_epi32(0, 0, 0, 0, 0, 1, 1, 0);
    r  = muli128(a, b);

    return _mm256_movemask_epi8(_mm256_cmpeq_epi32(r, c)) == -1;
}

int
muli128_11(void) {
    __m128i a;
    __m256i c, r;

    a = _mm_set_epi32(0, 0, 1, 1);
    c = _mm256_set_epi32(0, 0, 0, 0, 0, 1, 0, 1);
    r  = muli128(a, a);

    return _mm256_movemask_epi8(_mm256_cmpeq_epi32(r, c)) == -1;
}

int
mulGF2e128_2357_11131719(void) {
    __m128i a, b, c, r;

    a = b = _mm_set_epi32(-1, -1, -1, -1);
    c = _mm_set_epi32(0xf402aaaa, 0xaaaaaaaa, 0xaaaaaaaa, 0xaaaaaaaa);
    r = mulgf2e128mod0x87bitrev(a, b);

    return _mm_movemask_epi8(_mm_cmpeq_epi32(r, c)) == 0xffff;
}

int
main() {
    struct Tst {
        int (*fn)(void);
        const char *wh;
    } tests[] = {
        TEST(mulGF2e128_2357_11131719), TEST(muli128_0), TEST(muli128_1),
        TEST(muli128_11),
        TEST(NULL),
    };
    for (struct Tst *test = tests; test->fn; test++) {
        int res = test->fn();
        printf("[%s]: %s\n", res ? "OK" : "FAIL", test->wh);
    }
}
