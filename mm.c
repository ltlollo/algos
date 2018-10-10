// This is free and unencumbered software released into the public domain.
// For more information, see LICENSE

#include <assert.h>
#include <errno.h>
#include <immintrin.h>
#include <pthread.h>
#include <string.h>
#include <stdio.h>
#include <stdint.h>

#if !defined (SELF)
#   define NT_MAX (64)
#   define FN_(a, b)        a##b
#   define FN(a, b)         FN_(a, b)
#   define stream_load(a)   (Vec) _mm256_stream_load_si256((void *)(a))
#   ifdef BOUND_PREFETCH
#       define prefetch(a, m, h) _mm_prefetch((void *)min(a, b), (h))
#   else
#       define prefetch(a, m, h)        \
            do {                        \
                _mm_prefetch((a), (h)); \
                (void)(m);              \
            } while (0)
#   endif
#   define _u(x)            ((uintptr_t)(x))
#   define min(x, y)        (_u(y) ^ ((_u(x) ^ _u(y)) & -(_u(x) < _u(y))))
#   define max(x, y)        (_u(x) ^ ((_u(x) ^ _u(y)) & -(_u(x) < _u(y))))
#   define align_down(n, p) (_u(n) & ~((p) - 1))
#   define align_up(n, p)   ((_u(n) + ((p) - 1)) & ~((p) - 1))
#   define pad(x)           (256 / 8 / sizeof (x))
#endif

#if !defined (F32T)
#   define LINE     (8)
#   define Num      float
#   define Vec      __m256
#   define set1     _mm256_set1_ps
#   define fmadd    _mm256_fmadd_ps
#   define store    _mm256_store_ps
#   define load     _mm256_load_ps
#   define stream   _mm256_stream_ps
#   define FS       f32
#   define F32T
#elif !defined (F64T)
#   define LINE     (4)
#   define Num      double
#   define Vec      __m256d
#   define set1     _mm256_set1_pd
#   define fmadd    _mm256_fmadd_pd
#   define store    _mm256_store_pd
#   define load     _mm256_load_pd
#   define stream   _mm256_stream_pd
#   define FS       f64
#   define F64T
#endif

int
FN(ckpksz, FS)(size_t m, size_t k, size_t n, size_t *l2, size_t *l3) {
    m = align_up(m, pad(Num));
    n = align_up(n, pad(Num));
    k = align_up(k, pad(Num));

    int res = 0;
    size_t L2 = *l2, L3 = *l3;

    size_t kc = align_down(min(L3 / n, k), LINE);
    if (kc == 0) {
        kc = align_up(min(L3 / n + 1, k), LINE);
        *l3 = kc * n;
        res = -1;
    }
    size_t mc = align_down(min(L2 / kc, m), LINE);
    if (mc == 0) {
        mc = align_up(min(L2 / kc + 1, m), LINE);
        *l2 = mc * m;
        res = -1;
    }
    return res;
}

Num *
FN(allocm, FS)(size_t m, size_t n) {
    m = align_up(m, pad(Num));
    n = align_up(n, pad(Num));
    Num *res = NULL;

    if (m  > (SIZE_MAX - 32) / sizeof (Num) / n) {
        errno = ENOMEM;
    }
    if ((res = aligned_alloc(32, m * n * sizeof (Num)))) {
        memset(res, 0, m * n * sizeof (Num));
    }
    return res;
}

Num *
FN(allocv, FS)(size_t m) {
    m = align_up(m, pad(Num));
    Num *res = NULL;

    if (m  > (SIZE_MAX - 32) / sizeof (Num)) {
        errno = ENOMEM;
    }
    if ((res = aligned_alloc(32, m * sizeof (Num)))) {
        memset(res, 0, m * sizeof (Num));
    }
    return res;
}

void
FN(dger, FS)(Num *a, Num *b, Num *restrict c, size_t m, size_t n) {
    m = align_up(m, pad(Num));
    n = align_up(n, pad(Num));

    Vec rb, ra, rc;

    for (size_t i = 0; i < n; i++) {
        rb = set1(b[i]);
        for (size_t j = 0; j < m; j += LINE) {
            rc = stream_load(c + m * i + j);
            ra = stream_load(a + j);
            rc = fmadd(ra, rb, rc);
            store(c + m * i + j, rc);
        }
    }
}

void
FN(dgemv, FS)(Num *a, Num *b, Num *c, size_t m, size_t n) {
    m = align_up(m, pad(Num));
    n = align_up(n, pad(Num));

    Vec rb, ra, rc;

    for (size_t i = 0; i < n; i++) {
        rb = set1(b[i]);
        for (size_t j = 0; j < m; j += LINE) {
            rc = load(c + j);
            ra = stream_load(a + m * i + j);
            rc = fmadd(ra, rb, rc);
            store(c + j, rc);
        }
    }
}

#ifndef F64T
float
FN(hadd, FS)(__m256 in) {
    __m128 lo, hi, sv;
    lo = _mm256_castps256_ps128(in);
    hi = _mm256_extractf128_ps(in, 1);
    sv = _mm_add_ps(lo, hi);
    sv = _mm_hadd_ps(sv, sv);
    sv = _mm_hadd_ps(sv, sv);
    return _mm_cvtss_f32(sv);
}
#else
double
FN(hadd, FS)(__m256d in) {
    __m128d lo, hi, sv;
    lo = _mm256_castpd256_pd128(in);
    hi = _mm256_extractf128_pd(in, 1);
    sv = _mm_add_pd(lo, hi);
    sv = _mm_hadd_pd(sv, sv);
    return _mm_cvtsd_f64(sv);
}
#endif

void
FN(axpy, FS)(Num *a, Num b, Num *c, size_t m) {
    m = align_up(m, pad(Num));

    Vec rb, ra, rc;

    rb = set1(b);
    for (size_t j = 0; j < m; j += LINE) {
        rc = stream_load(c + j);
        ra = stream_load(a + j);
        rc = fmadd(ra, rb, rc);
        store(c + j, rc);
    }
}

Num
FN(dot, FS)(Num *a, Num *b, size_t m) {
    m = align_up(m, pad(Num));

    Vec rb, ra, rc = set1(0);

    for (size_t j = 0; j < m; j += LINE) {
        ra = stream_load(a + j);
        rb = stream_load(b + j);
        rc = fmadd(ra, rb, rc);
    }
    return FN(hadd, FS)(rc);
}

void
FN(pkdgemm, FS)(Num *a, Num *b, Num *restrict c, size_t m, size_t k, size_t n,
    Num *A, Num *B, size_t L2, size_t L3) {
    m = align_up(m, pad(Num));
    k = align_up(k, pad(Num));
    n = align_up(n, pad(Num));

    size_t _kc = align_down(min(L3 / n, k), LINE);
    size_t xa, ya, xb, yb, xc, yc, kc, mc;
    Vec ra, rb, rc[LINE];
    Num *lc = c + m * n - LINE;
    assert(_kc);

    for (size_t ki = 0; ki < k; ki += _kc) {
        kc = ki + _kc > k ? k - ki : _kc;
        xa = yb = ki;

        for (size_t j = 0; j < n; j++) {
            for (size_t i = 0; i < kc; i += LINE) {
                Num *vb = b + k * j + yb + i;
                rb = stream_load(vb);
                store(B + kc * j + i, rb);
                prefetch(c + kc * j + i, lc, _MM_HINT_T2);
            }
        }
        size_t _mc = align_down(min(L2 / kc, m), LINE);
        assert(_mc);
        for (size_t mi = 0; mi < m; mi += _mc) {
            mc = mi + _mc > m ? align_up(m - mi, LINE) : _mc;
            ya = yc = mi;
            for (size_t j = 0; j < kc; j++) {
                for (size_t i = 0; i < mc; i += LINE) {
                    Num *va = a + (xa + j) * m + (ya + i);
                    ra = stream_load(va);
                    store(A + mc * j + i, ra);
                    prefetch(c + m * j + yc, lc, _MM_HINT_T1);
                }
            }
            for (size_t ni = 0; ni < n; ni += LINE) {
                xc = xb = ni;
                for (size_t oi = 0; oi < mc; oi += LINE) {
                    for (size_t j = 0; j < LINE; j++) {
                        Num *vc = c + m * (xc + j) + yc + oi;
                        rc[j] = stream_load(vc);
                        prefetch(vc + LINE, lc, _MM_HINT_T0);
                    }
                    for (size_t pi = 0; pi < kc; pi++) {
                        ra = load(A + mc * pi + oi);
                        for (size_t j = 0; j < LINE; j++) {
                            rb = set1(B[kc * (xb + j) + pi]);
                            rc[j] = fmadd(ra, rb, rc[j]);
                        }
                    }
                    for (size_t j = 0; j < LINE; j++) {
                        store(c + m * (xc + j) + yc + oi, rc[j]);
                    }
                }
            }
        }
    }
}

struct FN(wthpar, FS) {
    Num *a, *b, *c;
    size_t m, k, n, nt, i, L2, L3;
};

void *
FN(wthdgemm, FS)(void *p) {
    struct FN(wthpar, FS) *in = p;

    Num *a = in->a, *b = in->b, *c = in->c;
    size_t m = in->m, k = in->k, n = in->n, nt = in->nt, i = in->i,
           L2 = in->L2, L3 = in->L3;
    size_t _kc = align_up(min(L3 / n + 1, k), LINE);
    size_t xa, ya, xb, yb, xc, yc, kc, mc;

    Vec ra, rb, rc[LINE];
    Num *lc = c + m * n - LINE;
    Num *la = a + m * k - LINE;
    Num *lb = b + k * n - LINE;

    for (size_t ki = i * _kc; ki < k; ki += _kc * nt) {
        kc = ki + _kc > k ? k - ki : _kc;
        xa = yb = ki;
        for (size_t j = 0; j < n; j++) {
            for (size_t i = 0; i < kc; i += LINE) {
                Num *vb = b + k * j + yb + i;
                prefetch(vb, lb, _MM_HINT_T2);
                prefetch(c + kc * j + i, lc, _MM_HINT_T2);
            }
        }
        size_t _mc = align_up(min(L2 / kc + 1, m), LINE);
        for (size_t mi = 0; mi < m; mi += _mc) {
            mc = mi + _mc > m ? align_up(m - mi, LINE) : _mc;
            ya = yc = mi;
            for (size_t j = 0; j < kc; j++) {
                for (size_t i = 0; i < mc; i += LINE) {
                    Num *va = a + (xa + j) * m + (ya + i);
                    prefetch(va, la, _MM_HINT_T1);
                    prefetch(c + m * j + yc, lc, _MM_HINT_T1);
                }
            }
            for (size_t ni = 0; ni < n; ni += LINE) {
                xc = xb = ni;
                for (size_t oi = 0; oi < mc; oi += LINE) {
                    for (size_t j = 0; j < LINE; j++) {
                        Num *vc = c + m * (xc + j) + yc + oi;
                        rc[j] = stream_load(vc);
                        prefetch(vc + LINE, lc, _MM_HINT_T0);
                    }
                    for (size_t pi = 0; pi < kc; pi++) {
                        ra = load(a + mc * (xa + pi) + oi + ya);
                        for (size_t j = 0; j < LINE; j++) {
                            rb = set1(b[kc * (xb + j) + pi + yb]);
                            rc[j] = fmadd(ra, rb, rc[j]);
                        }
                    }
                    for (size_t j = 0; j < LINE; j++) {
                        store(c + m * (xc + j) + yc + oi, rc[j]);
                    }
                }
            }
        }
    }
    return NULL;
}

void
FN(dgemm, FS)(Num *a, Num *b, Num *restrict c, size_t m, size_t k, size_t n,
    size_t L2, size_t L3) {
    m = align_up(m, pad(Num));
    k = align_up(k, pad(Num));
    n = align_up(n, pad(Num));

    struct FN(wthpar, FS) wthp = {
        a, b, c, m, k, n, 1, 0, L2, L3,
    };
    FN(wthdgemm, FS)(&wthp);
}

void
FN(mtdgemm, FS)(Num *a, Num *b, Num *restrict c, size_t m, size_t k, size_t n,
    size_t L2, size_t L3, size_t nt) {
    m = align_up(m, pad(Num));
    k = align_up(k, pad(Num));
    n = align_up(n, pad(Num));
    nt = min(nt, NT_MAX);

    struct FN(wthpar, FS) wthp[NT_MAX];
    pthread_t wth[NT_MAX];

    for (size_t i = 0; i < nt; i++) {
        wthp[i] = (struct FN(wthpar, FS)) {
            a, b, c, m, k, n, nt, i, L2, L3,
        };
        pthread_create( wth + i, NULL, FN(wthdgemm, FS), wthp + i);
    }
    for (size_t i = 0; i < nt; i++) {
        pthread_join( wth[i], NULL);
    }
}

void
FN(printoffvT, FS)(Num *m, size_t mx, size_t ox, size_t dx) {
    mx = align_up(mx, pad(Num));

    for (size_t x = ox; x < ox + dx; x++) {
        printf("%.2f,\t", m[x]);
    }
    printf("\n");
}

void
FN(printvT, FS)(Num *m, size_t mx) {
    FN(printoffvT, FS)(m, mx, 0, mx);
}

void
FN(printoffv, FS)(Num *m, size_t my, size_t oy, size_t dy) {
    my = align_up(my, pad(Num));

    for (size_t y = oy; y < oy + dy; y++) {
        printf("%.2f,\n", m[y]);
    }
}

void
FN(printv, FS)(Num *m, size_t my) {
    FN(printoffv, FS)(m, my, 0, my);
}

void
FN(printoffm, FS)(Num *m, size_t my, size_t mx, size_t oy, size_t ox,
    size_t dy, size_t dx) {
    mx = align_up(mx, pad(Num));
    my = align_up(my, pad(Num));

    for (size_t y = oy; y < oy + dy; y++) {
        for (size_t x = ox; x < ox + dx; x++) {
            printf("%.2f,\t", m[my * x + y]);
        }
        printf("\n");
    }
    printf("\n");
}

void
FN(printm, FS)(Num *m, size_t my, size_t mx) {
    FN(printoffm, FS)(m, my, mx, 0, 0, my, mx);
}

void
FN(iotaoffv, FS)(Num v, Num d, Num *m, size_t my, size_t oy, size_t dy) {
    my = align_up(my, pad(Num));

    for (size_t y = oy; y < oy + dy; y++, v += d) {
        m[y] = v;
    }
}

void
FN(iotav, FS)(Num v, Num d, Num *m, size_t my) {
    FN(iotaoffv, FS)(v, d, m, my, 0, my);
}


void
FN(iotaoffm, FS)(Num v, Num d, Num *m, size_t my, size_t mx, size_t oy,
    size_t ox, size_t dy, size_t dx) {
    mx = align_up(mx, pad(Num));
    my = align_up(my, pad(Num));

    for (size_t x = ox; x < ox + dx; x++) {
        for (size_t y = oy; y < oy + dy; y++, v += d) {
            m[my * x + y] = v;
        }
    }
}

void
FN(iotam, FS)(Num v, Num d, Num *m, size_t my, size_t mx) {
    FN(iotaoffm, FS)(v, d, m, my, mx, 0, 0, my, mx);
}

int
FN(eqm, FS)(Num *a, Num *b, size_t my, size_t mx) {
    mx = align_up(mx, pad(Num));
    my = align_up(my, pad(Num));

    for (size_t i = 0; i < mx; i++) {
        for (size_t j = 0; j < my; j++) {
            if (a[my * i + j] != b[my * i + j]) {
                return -1;
            }
        }
    }
    return 0;
}

int
FN(eqv, FS)(Num *a, Num *b, size_t my) {
    my = align_up(my, pad(Num));

    for (size_t j = 0; j < my; j++) {
        if (a[j] != b[j]) {
            return -1;
        }
    }
    return 0;
}

void
FN(chordm, FS)(Num *mt, Num *restrict m, size_t my, size_t mx) {
    mx = align_up(mx, pad(Num));
    my = align_up(my, pad(Num));

    for (size_t i = 0; i < my; i++) {
        for (size_t j = 0; j < mx; j++) {
            Num q = m[mx * i + j];
            mt[my * j + i] = q;
        }
    }
}

#ifndef F64T
void
FN(Tm, FS)(Num *m, Num *restrict mc, size_t my, size_t mx) {
    mx = align_up(mx, pad(Num));
    my = align_up(my, pad(Num));
    Num *lc = m + mx * my - LINE;

    __m256i regs[8];
    __m256i tmp[8];
    for (size_t x = 0; x < mx; x += LINE) {
        for (size_t y = 0; y < my; y += LINE) {
            for (size_t i = 0; i < LINE; i++) {
                void *src = m + (x + i) * my + y;
                regs[i] = _mm256_stream_load_si256(src);
            }
            for (size_t i = 0; i < LINE; i++) {
                prefetch(m + (x + i) * my + y + LINE, lc, _MM_HINT_T0);
            }
            tmp[0] = _mm256_permute2x128_si256(regs[4], regs[0], 0x3);
            tmp[1] = _mm256_permute2x128_si256(regs[5], regs[1], 0x3);
            tmp[2] = _mm256_permute2x128_si256(regs[6], regs[2], 0x3);
            tmp[3] = _mm256_permute2x128_si256(regs[7], regs[3], 0x3);

            regs[0] = _mm256_blend_epi32(regs[0], tmp[0], 0xf0);
            regs[4] = _mm256_blend_epi32(regs[4], tmp[0], 0x0f);
            regs[1] = _mm256_blend_epi32(regs[1], tmp[1], 0xf0);
            regs[5] = _mm256_blend_epi32(regs[5], tmp[1], 0x0f);
            regs[2] = _mm256_blend_epi32(regs[2], tmp[2], 0xf0);
            regs[6] = _mm256_blend_epi32(regs[6], tmp[2], 0x0f);
            regs[3] = _mm256_blend_epi32(regs[3], tmp[3], 0xf0);
            regs[7] = _mm256_blend_epi32(regs[7], tmp[3], 0x0f);

            tmp[0] = _mm256_shuffle_epi32(regs[0], 0x0e);
            tmp[1] = _mm256_shuffle_epi32(regs[1], 0x0e);
            tmp[2] = _mm256_shuffle_epi32(regs[2], 0x40);
            tmp[3] = _mm256_shuffle_epi32(regs[3], 0x40);
            tmp[4] = _mm256_shuffle_epi32(regs[4], 0x0e);
            tmp[5] = _mm256_shuffle_epi32(regs[5], 0x0e);
            tmp[6] = _mm256_shuffle_epi32(regs[6], 0x40);
            tmp[7] = _mm256_shuffle_epi32(regs[7], 0x40);

            regs[0] = _mm256_blend_epi32(regs[0], tmp[2], 0xcc);
            regs[1] = _mm256_blend_epi32(regs[1], tmp[3], 0xcc);
            regs[2] = _mm256_blend_epi32(regs[2], tmp[0], 0x33);
            regs[3] = _mm256_blend_epi32(regs[3], tmp[1], 0x33);
            regs[4] = _mm256_blend_epi32(regs[4], tmp[6], 0xcc);
            regs[5] = _mm256_blend_epi32(regs[5], tmp[7], 0xcc);
            regs[6] = _mm256_blend_epi32(regs[6], tmp[4], 0x33);
            regs[7] = _mm256_blend_epi32(regs[7], tmp[5], 0x33);

            tmp[0] = _mm256_shuffle_epi32(regs[0], 0x31);
            tmp[1] = _mm256_shuffle_epi32(regs[1], 0x80);
            tmp[2] = _mm256_shuffle_epi32(regs[2], 0x31);
            tmp[3] = _mm256_shuffle_epi32(regs[3], 0x80);
            tmp[4] = _mm256_shuffle_epi32(regs[4], 0x31);
            tmp[5] = _mm256_shuffle_epi32(regs[5], 0x80);
            tmp[6] = _mm256_shuffle_epi32(regs[6], 0x31);
            tmp[7] = _mm256_shuffle_epi32(regs[7], 0x80);

            regs[0] = _mm256_blend_epi32(regs[0], tmp[1], 0xaa);
            regs[1] = _mm256_blend_epi32(regs[1], tmp[0], 0x55);
            regs[2] = _mm256_blend_epi32(regs[2], tmp[3], 0xaa);
            regs[3] = _mm256_blend_epi32(regs[3], tmp[2], 0x55);
            regs[4] = _mm256_blend_epi32(regs[4], tmp[5], 0xaa);
            regs[5] = _mm256_blend_epi32(regs[5], tmp[4], 0x55);
            regs[6] = _mm256_blend_epi32(regs[6], tmp[7], 0xaa);
            regs[7] = _mm256_blend_epi32(regs[7], tmp[6], 0x55);

            for (size_t i = 0; i < 8; i++) {
                Num *src = mc + (y + i) * mx + x;
                store(src, (Vec) regs[i]);
            }
        }
    }
}
#else
void
FN(Tm, FS)(Num *m, Num *restrict mc, size_t my, size_t mx) {
    mx = align_up(mx, pad(Num));
    my = align_up(my, pad(Num));
    Num *lc = m + mx * my - LINE;

    __m256i regs[LINE];
    __m256i tmp[LINE];
    for (size_t x = 0; x < mx; x += LINE) {
        for (size_t y = 0; y < my; y += LINE) {
            for (size_t i = 0; i < LINE; i++) {
                void *src = m + (x + i) * my + y;
                regs[i] = _mm256_stream_load_si256(src);
            }
            for (size_t i = 0; i < LINE; i++) {
                prefetch(m + (x + i) * my + y + LINE, lc, _MM_HINT_T0);
            }
            tmp[0] = _mm256_permute2x128_si256(regs[2], regs[0], 0x3);
            tmp[1] = _mm256_permute2x128_si256(regs[3], regs[1], 0x3);

            regs[0] = _mm256_blend_epi32(regs[0], tmp[0], 0xf0);
            regs[2] = _mm256_blend_epi32(regs[2], tmp[0], 0x0f);
            regs[1] = _mm256_blend_epi32(regs[1], tmp[1], 0xf0);
            regs[3] = _mm256_blend_epi32(regs[3], tmp[1], 0x0f);

            tmp[0] = _mm256_shuffle_epi32(regs[0], 0x0e);
            tmp[1] = _mm256_shuffle_epi32(regs[1], 0x40);
            tmp[2] = _mm256_shuffle_epi32(regs[2], 0x0e);
            tmp[3] = _mm256_shuffle_epi32(regs[3], 0x40);

            regs[0] = _mm256_blend_epi32(regs[0], tmp[1], 0xcc);
            regs[1] = _mm256_blend_epi32(regs[1], tmp[0], 0x33);
            regs[2] = _mm256_blend_epi32(regs[2], tmp[3], 0xcc);
            regs[3] = _mm256_blend_epi32(regs[3], tmp[2], 0x33);

            for (size_t i = 0; i < LINE; i++) {
                Num *src = mc + (y + i) * mx + x;
                store(src, (Vec) regs[i]);
            }
        }
    }
}
#endif

#undef LINE
#undef Num
#undef Vec
#undef set1
#undef fmadd
#undef store
#undef load
#undef stream
#undef FS
#define SELF

#if !defined (F64T)
#   include __FILE__
#endif
