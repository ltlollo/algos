// This is free and unencumbered software released into the public domain.
// For more information, see LICENSE

#include <immintrin.h>
#include <stdint.h>

__m256i muli128(__m128i, __m128i);
__m128i mulgf2e128mod0x87bitrev(__m128i, __m128i);

uint16_t crc16kermit(void *, size_t);
uint16_t crc16kermitlite(void *, size_t);
