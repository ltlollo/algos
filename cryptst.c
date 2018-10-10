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
mul_128i_0(void) {
    __m128i a, b;
    __m256i c, r;

    a = _mm_set_epi32(0, 0, 0, 0);
    b = _mm_set_epi32(0, 0, 0, 0);
    c = _mm256_set_epi32(0, 0, 0, 0, 0, 0, 0, 0);
    r  = mul_128i(a, b);

    return _mm256_movemask_epi8(_mm256_cmpeq_epi32(r, c)) == -1;
}

int
mul_128i_1(void) {
    __m128i a, b;
    __m256i c, r;

    a = _mm_set_epi32(0, 0, 1, 0);
    b = _mm_set_epi32(0, 0, 1, 1);
    c = _mm256_set_epi32(0, 0, 0, 0, 0, 1, 1, 0);
    r  = mul_128i(a, b);

    return _mm256_movemask_epi8(_mm256_cmpeq_epi32(r, c)) == -1;
}


int
mulGF2e128_2357_11131719(void) {
    __m128i a = _mm_set_epi32(2,3,5,7);
    __m128i b = _mm_set_epi32(11,13,17,19);

    mul_gf2e128mod0x87(a, b);

    return 0;
}

int
main() {
    struct Tst {
        int (*fn)(void);
        const char *wh;
    } tests[] = {
        TEST(mulGF2e128_2357_11131719), TEST(mul_128i_0), TEST(mul_128i_1),
        TEST(NULL),
    };
    for (struct Tst *test = tests; test->fn; test++) {
        int res = test->fn();
        printf("[%s]: %s\n", res ? "OK" : "FAIL", test->wh);
    }
}
