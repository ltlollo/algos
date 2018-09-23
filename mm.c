// This is free and unencumbered software released into the public domain.
// For more information, see LICENSE

#if !defined (SELF)
#define SELF
#   include "mm.h"
#   define FN_(a, b)            a##b
#   define FN(a, b)             FN_(a, b)
#   define stream_load (Vec)    _mm256_stream_load_si256
#   define prefetch             _mm_prefetch
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
#   define FS       32
#   define F32T
#elif !defined (F64T)
#   define LINE     (4)
#   define Num      double
#   define Vec      __m256d
#   define __m256   __m256d
#   define set1     _mm256_set1_pd
#   define fmadd    _mm256_fmadd_pd
#   define store    _mm256_store_pd
#   define load     _mm256_load_pd
#   define stream   _mm256_stream_pd
#   define FS       64
#   define F64T
#endif

void
FN(gemm, FS)(Num *a, Num *b, Num *c, size_t m, size_t k, size_t n, Num *A,
    Num *B, size_t L2, size_t L3) {
    size_t _kc = align_up(min(L3 / n + 1, k), LINE);
    size_t xa, ya, xb, yb, xc, yc, kc, mc;
    Vec ra, rb, rc[LINE];
    Num *lc = c + m * n - LINE;

    for (size_t ki = 0; ki < k; ki += _kc) {
        kc = ki + _kc > k ? k - ki : _kc;
        xa = yb = ki;

        for (size_t j = 0; j < n; j++) {
            for (size_t i = 0; i < kc; i += LINE) {
                void *vb = b + k * j + yb + i;
                Num *vc = (void *)min(c + kc * j + i, lc);
                rb = stream_load(vb);
                store(B + kc * j + i, rb);
                prefetch(vc, _MM_HINT_T2);
            }
        }
        size_t _mc = align_up(min(L2 / kc + 1, m), LINE);
        for (size_t mi = 0; mi < m; mi += _mc) {
            mc = mi + _mc > m ? align_up(m - mi, LINE) : _mc;
            ya = yc = mi;
            for (size_t j = 0; j < kc; j++) {
                for (size_t i = 0; i < mc; i += LINE) {
                    void *va = a + (xa + j) * m + (ya + i);
                    Num *vc = (void *)min(c + m * j + yc, lc);
                    ra = stream_load(va);
                    store(A + mc * j + i, ra);
                    prefetch(vc, _MM_HINT_T1);
                }
            }
            for (size_t ni = 0; ni < n; ni += LINE) {
                xc = xb = ni;
                for (size_t oi = 0; oi < mc; oi += LINE) {
                    for (size_t j = 0; j < LINE; j++) {
                        void *vc = c + m * (xc + j) + yc + oi;
                        rc[j] = stream_load(vc);
                    }
                    for (size_t pi = 0; pi < kc; pi++) {
                        ra = load(A + mc * pi + oi);
                        for (size_t j = 0; j < LINE; j++) {
                            rb = set1(B[kc * (xb + j) + pi]);
                            rc[j] = fmadd(ra, rb, rc[j]);
                        }
                    }
                    for (size_t j = 0; j < LINE; j++) {
                        stream(c + m * (xc + j) + yc + oi, rc[j]);
                    }
                }
            }
        }
    }
}

void
FN(printoffm, FS)(Num *m, size_t mx, size_t my, size_t ox, size_t oy,
    size_t dx, size_t dy) {
    for (size_t y = oy; y < oy + dy; y++) {
        for (size_t x = ox; x < ox + dx; x++) {
            printf("%0.2f,\t", m[my * x + y]);
        } 
        printf("\n");
    } 
    printf("\n");
}

void
FN(printm, FS)(Num *m, size_t mx, size_t my) {
    FN(printoffm, FS)(m, mx, my, 0, 0, mx, my);
}

void
FN(iotaoffm, FS)(Num v, Num d, Num *m, size_t mx, size_t my, size_t ox,
    size_t oy, size_t dx, size_t dy) {
    for (size_t x = ox; x < ox + dx; x++) {
        for (size_t y = oy; y < oy + dy; y++, v += d) {
            m[my * x + y] = v;
        } 
    } 
}

void
FN(iotam, FS)(Num v, Num d, Num *m, size_t mx, size_t my) {
    FN(iotaoffm, FS)(v, d, m, mx, my, 0, 0, mx, my);
}

int
FN(eqm, FS)(Num *a, Num *b, size_t x, size_t y) {
    for (size_t i = 0; i < x; i++) {
        for (size_t j = 0; j < y; j++) {
            if (a[y * i + j] != b[y * i + j]) {
                return -1;
            }
        }
    }
    return 0;
}

void
FN(chordm, FS)(Num *mt, Num *m, size_t mx, size_t my) {
    for (size_t i = 0; i < my; i++) {
        for (size_t j = 0; j < mx; j++) {
            Num q = m[mx * i + j];
            mt[my * j + i] = q;
        }
    }
}

#undef LINE
#undef Num
#undef Vec
#undef set1
#undef fmadd
#undef store
#undef load
#undef stream
#undef FS

#if !defined (F64T)
#   include __FILE__
#endif
