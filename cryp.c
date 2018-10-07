#include "cryp.h"

__m128i
mul_gf2e128mod0x87(__m128i x, __m128i h) {
    __m128i sx =_mm_shuffle_epi32(x, 0x4e), sh =_mm_shuffle_epi32(h, 0x4e);
    // x := [a|b|c|d], sx := [c|d|a|b], h := [e|f|g|h], sh = [g|h|e|f]

    __m256i a = _mm256_set_m128i(sx, x), b = _mm256_set_m128i(sh, h);
    // a := [a|b|c|d|c|d|a|b], b := [e|f|g|h|g|h|e|f]

    __m256i sa = _mm256_shuffle_epi32(a, 0x50),
            sb = _mm256_shuffle_epi32(b, 0x50);
    // sa := [a|a|b|b|c|c|d|d], sb := [e|e|f|f|g|g|h|h]

    __m256i m1 = _mm256_mul_epu32(sa, sb); // m1 := [ae  |bf  |cg  |dh  ]

    __m256i ssa = _mm256_shuffle_epi32(a, 0x45),
            ssb = _mm256_shuffle_epi32(b, 0x45);
    // ssa := [b|b|a|b|d|d|c|d], ssb := [f|f|e|f|h|h|g|h]

    __m256i xa = _mm256_xor_si256(ssa, a), xb = _mm256_xor_si256(ssb, b);
    // xa := [a+b|-|c+a|d+b|c+d|-|a+c|b+d], xb := [e+f|-|g+f|h+f|g+h|-|f+g|g+h]

    __m256i m2 = _mm256_mul_epu32(xa, xb);
    // m2 := [(a+b)(e+f)|(c+a)(g+e)|(c+d)(g+h)|(c+a)(g+e)]

    __m256i xm2 = _mm256_xor_si256(m2, m1);
    // xm2 := [be+fa+bf|????|dg+ch+dh|????]

    __m256i sm1 = _mm256_shuffle_epi32(m1, 0xe);
    // sm1 := [bf  |????|dh  |????]

    xm2 = _mm256_xor_si256(xm2, sm1);  // xm2 := [be+fa|????|dg+ch|????]
    xm2 = _mm256_bslli_epi128(xm2, 8); // xm2 := [-  |be+fa|-  |dg+ch]
    xm2 = _mm256_bsrli_epi128(xm2, 4); // xm2 := [-|be+fa|-|-|dg+ch|-]
    m1 = _mm256_xor_si256(m1, xm2);    // m1  := [ab*ef    |cd*gh    ]

    __m256i sxa = _mm256_shuffle_epi32(xa, 0xe5),
            sxb = _mm256_shuffle_epi32(xb, 0xe5);
    // sxa := [-|-|c+a|d+b|-|-|a+c|b+d], sxb := [-|-|g+e|h+f|-|-|g+e|h+f]

    __m256i ssxa = _mm256_shuffle_epi32(xa, 0x37),
            ssxb = _mm256_shuffle_epi32(xb, 0x37);
    // ssxa := [d+b|-|d+b|??|b+d|-|b+d|??] ssxb := [h+f|-|h+f|??|f+h|-|f+h|??]

    ssa = _mm256_xor_si256(sxa, ssxa); ssb = _mm256_xor_si256(sxb, ssxb);
    // ssa := [d+b|-|a+b+c+d|??|b+d|-|a+b+c+d|??],
    // ssb := [h+f|-|e+f+g+h|??|f+g|-|e+f+g+h|??]

    __m256i m3 = _mm256_mul_epu32(ssa, ssb);
    // m3 := [(d+b)(h+f)|(a+b+c+d)(e+f+g+h)|(d+b)(h+f)|(a+b+c+d)(e+f+g+h)]
    
    __m256i z = _mm256_setzero_si256();

    m2 = _mm256_blend_epi32(z, m2, 0xc);  // m2 := [-  |(c+a)(g+e)|-  |-  ]
    m1 = _mm256_xor_si256(m1, m2);

    m2 = _mm256_blend_epi32(z, m3, 0x30); // m2 := [-  |-  |(d+b)(h+f)|-  ]
    m1 = _mm256_xor_si256(m1, m2);

    m2 = _mm256_blend_epi32(z, m3, 0xc);
    // m2 := [-  |(a+b+c+d)(e+f+g+h)|-  |-  ]
    m2 = _mm256_bslli_epi128(m2, 2);
    // m2 := [-|-|-|(a+b+c+d)(e+f+g+h):lo|-|-|-|-]
    m1 = _mm256_xor_si256(m1, m2);

    m2 = _mm256_blend_epi32(z, m3, 0xc0);
    // m2 := [-  |-  |-  |(a+b+c+d)(e+f+g+h)]
    m2 = _mm256_bsrli_epi128(m2, 14);
    // m2 := [-|-|-|-|(a+b+c+d)(e+f+g+h):hi|-|-|-]
    m1 = _mm256_xor_si256(m1, m2);

    __m128i hi = _mm256_extracti128_si256(m1, 1),
            lo = _mm256_extracti128_si256(m1, 0);
    // reduction mod P(2, 0x87)
    // 128 = 7, 2, 1, 0                | 248 = 127, 122, 121, 120
    // 249 = 123, 122, 121, 7, 2, 1, 0 | 253 = 127, 126, 125, 11, 6, 5, 4
    // 254 = 127, 126, 12, 6, 5, 2, 1, 0
    // 255 = 127, 13, 6, 3, 0
    __m128i mask = _mm_set_epi64x(3ull<<62, 0);
    __m128i lst = _mm_and_si128(mask, hi), fst = _mm_andnot_si128(mask, hi);

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

