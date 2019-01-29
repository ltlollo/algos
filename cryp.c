// This is free and unencumbered software released into the public domain.      
// For more information, see LICENSE

#include <immintrin.h>

static inline __m128i
p128sli(__m128i a, int imm) {
    __m128i ss = _mm_slli_epi64(a, imm);
    __m128i sr =  _mm_srli_epi64(_mm_slli_si128(a, 8), 64 - imm);
    return _mm_xor_si128(ss, sr);
}

static inline __m128i
p128sri(__m128i a, int imm) {
    __m128i ss = _mm_srli_epi64(a, imm);
    __m128i sr =  _mm_slli_epi64(_mm_srli_si128(a, 8), 64 - imm);
    return _mm_xor_si128(ss, sr);
}

__m128i
mulgf2e128mod0x87bitrev(__m128i x, __m128i h) {
    __m128i H, L, Mx, Mh, M ,hi, lo, mask, fst, lst, cmp,
            rdl, rb, rd, rab0, rab1, rab2, rab7; 

    H = _mm_clmulepi64_si128(x, h, 0x11);
    L = _mm_clmulepi64_si128(x, h, 0x00);
    Mx = _mm_xor_si128(x, _mm_bsrli_si128(x, 8));
    Mh = _mm_xor_si128(h, _mm_bsrli_si128(h, 8));
    M = _mm_clmulepi64_si128(Mx, Mh, 0x00);
    hi = _mm_xor_si128(H, _mm_bsrli_si128(H, 8));
    hi = _mm_xor_si128(hi, _mm_bsrli_si128(M, 8));
    hi = _mm_xor_si128(hi, _mm_bsrli_si128(L, 8));
    lo = _mm_xor_si128(L, _mm_bslli_si128(H, 8));
    lo = _mm_xor_si128(lo, _mm_bslli_si128(M, 8));
    lo = _mm_xor_si128(lo, _mm_bslli_si128(L, 8));

    // reduction mod P(2, 0x87)
    // 128 = 7, 2, 1, 0                | 248 = 127, 122, 121, 120
    // 249 = 123, 122, 121, 7, 2, 1, 0 | 253 = 127, 126, 125, 11, 6, 5, 4
    // 254 = 127, 126, 12, 6, 5, 2, 1, 0
    // 255 = 127, 13, 6, 3, 0  -- impossible

    mask = _mm_set_epi64x(1ull<<63, 1ull);
    lst = _mm_and_si128(mask, lo), fst = _mm_andnot_si128(mask, lo);
    cmp = _mm_set1_epi32(1), rd = _mm_shuffle_epi32(lst, 0);
    rdl = _mm_and_si128(rd, cmp);
    rdl = _mm_cmpeq_epi32(rdl, cmp);
    rdl = _mm_and_si128(rdl, _mm_set_epi64x(0xe608000000000000ull, 3ull));

    hi = p128sli(hi, 1);
    hi = _mm_xor_si128(hi, _mm_srli_epi64(_mm_srli_si128(lst, 8), 64 - 1));
    hi = _mm_xor_si128(hi, rdl);

    rb = _mm_srli_epi64(_mm_slli_epi64(_mm_slli_si128(fst, 15), 2), 1);

    rab1 = _mm_xor_si128(fst, rb);
    rab0 = p128sli(rab1, 1);
    rab2 = p128sri(rab1, 1);
    rab7 = p128sri(rab1, 6);

    rab0 = _mm_xor_si128(rab0, rab1);
    rab2 = _mm_xor_si128(rab2, rab7);
    hi = _mm_xor_si128(hi, rab0);
    hi = _mm_xor_si128(hi, rab2);

    return hi;
}

