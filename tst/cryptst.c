// This is free and unencumbered software released into the public domain.
// For more information, see LICENSE

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "cryp.h"
#include "tbh.h"

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
    struct Tst tests[] = {
        TEST(mulGF2e128_2357_11131719),
        TEST(NULL),
    };
    return test(__FILE__, tests);
}
