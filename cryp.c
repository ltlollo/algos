#include <immintrin.h>

__m128i
mul_gf2e128mod0x87(__m128i x, __m128i h) {
    // x := [a|b|c|d]
    __m128i sx = (__m128i) _mm_permute_pd((__m128d) x, 1);
    // sx := [c|d|a|b]
    __m128i sh = (__m128i) _mm_permute_pd((__m128d) h, 1);

    __m256i a = _mm256_set_m128i(x, sx);
    // a := [a|b|c|d|c|d|a|b]
    __m256i b = _mm256_set_m128i(h, sh);

    __m256i sa = _mm256_shuffle_epi32(a, 0x50);
    // sa := [a|a|b|b|c|c|d|d]
    __m256i sb = _mm256_shuffle_epi32(b, 0x50);

    __m256i m1 = _mm256_mul_epu32(sa, sb);
    // m1 := [ae  |bf  |cg  |dh  ]

    __m256i ssa = _mm256_shuffle_epi32(a, 0x45);
    // ssa := [b|b|a|b|d|d|c|d]
    __m256i ssb = _mm256_shuffle_epi32(b, 0x45);

    __m256i xxa = _mm256_xor_si256(ssa, a);
    // xxa := [a+b|-|c+a|d+b|c+d|-|a+c|b+d
    __m256i xxb = _mm256_xor_si256(ssb, b);
    
    __m256i m2 = _mm256_mul_epu32(xxa, xxb);
    // m2 := [(a+b)(e+f)|(c+a)(g+e)|(c+d)(g+h)|(c+a)(g+e)]

    __m256i xm2 = _mm256_xor_si256(m2, m1);
    // xm2 := [be+fa+bf|??  |dg+ch+dh|??  ]

    __m256i sm1 = _mm256_shuffle_epi32(m1, 0xe);
    // sm1 := [bf  |??  |dh  |??  ]

    xm2 = _mm256_xor_si256(xm2, sm1);
    // xm2 := [be+fa|??  |dg+ch|??  ]

    xm2 = _mm256_bslli_epi128(xm2, 8);
    // xm2 := [-  |be+fa|-  |dg+ch]

    xm2 = _mm256_bsrli_epi128(xm2, 4);
    // xm2 := [-|be+fa|-|-|dg+ch|-]

    m1 = _mm256_xor_si256(m1, xm2);
    // m1 := [ab*ef    |cd*gh    ]

    __m256i sxxa = _mm256_shuffle_epi32(xxa, 0xe5);
    // xxa := [-|-|c+a|d+b|-|-|a+c|b+d
    __m256i sxxb = _mm256_shuffle_epi32(xxb, 0xe5);

    __m256i ssxxa = _mm256_shuffle_epi32(xxa, 0x37);
    // ssxxa := [d+b|-|d+b|-|b+d|-|b+d|-
    __m256i ssxxb = _mm256_shuffle_epi32(xxb, 0x37);

    ssa = _mm256_xor_si256(sxxa, ssxxa);
    // ssa := [d+b|-|a+b+c+d|??|b+d|-|a+b+c+d|??
    ssb = _mm256_xor_si256(sxxb, ssxxb);

    __m256i m3 = _mm256_mul_epu32(ssa, ssb);
    // m3 := [(d+b)(h+f)|(a+b+c+d)(e+f+g+h)|(d+b)(h+f)|(a+b+c+d)(e+f+g+h)]
    
    __m256i z = _mm256_setzero_si256();

    m2 = _mm256_blend_epi32(z, m2, 0xc);
    // m2 := [-  |(c+a)(g+e)|-  |-  ]
    m1 = _mm256_xor_si256(m1, m2);

    m2 = _mm256_blend_epi32(z, m3, 0x30);
    m1 = _mm256_xor_si256(m1, m2);

    m2 = _mm256_blend_epi32(z, m3, 0xc);
    m2 = _mm256_bslli_epi128(m2, 2);
    m1 = _mm256_xor_si256(m1, m2);

    m2 = _mm256_blend_epi32(z, m3, 0xc0);
    m2 = _mm256_bsrli_epi128(m2, 14);
    m1 = _mm256_xor_si256(m1, m2);

    __m128i hi = _mm256_extracti128_si256(m1, 1);
    __m128i lo = _mm256_extracti128_si256(m1, 0);

    // reduction mod P(2, 0x87)
    // 128 = 7, 2, 1, 0                | 248 = 127, 122, 121, 120
    // 249 = 123, 122, 121, 7, 2, 1, 0 | 253 = 127, 126, 125, 11, 6, 5, 4
    // 254 = 127, 126, 12, 6, 5, 2, 1, 0
    // 255 = 127, 13, 6, 3, 0
    __m128i mask = _mm_set_epi64x(3ull<<62, 0);

    __m128i lst = _mm_and_si128(mask, hi);
    __m128i fst = _mm_andnot_si128(mask, hi);

    __m128i cmp = _mm_set1_epi32(0x40000000);
    __m128i rd = _mm_shuffle_epi32(lst, 0xff);

    __m128i rdh = _mm_cmpgt_epi32(rd, cmp);
    rdh = _mm_and_si128(rdh, _mm_set_epi64x(1ull<<63, 0x2049));

    __m128i rdl = _mm_and_si128(rd, cmp);
    rdl = _mm_cmpeq_epi32(rdl, cmp);
    rdl = _mm_and_si128(rdl, _mm_set_epi64x(3ull<<62, 0x1067));

    lo = _mm_xor_si128(lo, rdl);
    lo = _mm_xor_si128(lo, rdh);

    __m128i rb = _mm_srli_si128(fst, 15);
    rb = _mm_srli_epi32(rb, 2);

    __m128i rab0 = _mm_xor_si128(fst, rb);

    __m128i hrab = _mm_slli_si128(rab0, 8);
    __m128i rab1 = _mm_srli_epi32(hrab, 64 - 1);
    __m128i rab2 = _mm_srli_epi32(hrab, 64 - 2);
    __m128i rab7 = _mm_srli_epi32(hrab, 64 - 7);

    rab1 = _mm_or_si128(rab1, _mm_slli_epi64(rab0, 1));
    rab2 = _mm_or_si128(rab2, _mm_slli_epi64(rab0, 2));
    rab7 = _mm_or_si128(rab7, _mm_slli_epi64(rab0, 7));

    rab0 = _mm_xor_si128(rab0, rab1);
    rab2 = _mm_xor_si128(rab2, rab7);
    lo = _mm_xor_si128(lo, rab0);
    lo = _mm_xor_si128(lo, rab2);

    return lo;
}

