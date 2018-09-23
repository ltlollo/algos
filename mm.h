// This is free and unencumbered software released into the public domain.
// For more information, see LICENSE

#ifndef MM_H
#define MM_H
#include <stdint.h>
#include <stdio.h>
#include <immintrin.h>

#define _u(x) ((uintptr_t)(x))
#define min(x, y) (_u(y) ^ ((_u(x) ^ _u(y)) & -(_u(x) < _u(y))))
#define max(x, y) (_u(x) ^ ((_u(x) ^ _u(y)) & -(_u(x) < _u(y))))
#define align_down(n, p) (_u(n) & ~(p - 1))
#define align_up(n, p)   ((_u(n) + (p - 1)) & ~(p - 1))
#define pad(x) (256 / 8 / sizeof (x))

void gemm32(float *a, float *b, float *c, size_t m, size_t k, size_t n,
    float *A, float *B, size_t L2, size_t L3);
void printoffm32(float *m, size_t mx, size_t my, size_t ox, size_t oy,
    size_t dx, size_t dy);
void printm32(float *m, size_t mx, size_t my);
void iotaoffm32(float v, float d, float *m, size_t mx, size_t my, size_t ox,
    size_t oy, size_t dx, size_t dy);
void iotam32(float v, float d, float *m, size_t mx, size_t my);
int  eqm32(float *a, float *b, size_t x, size_t y);
void chordm32(float *mt, float *m, size_t mx, size_t my);

void gemm64(double *a, double *b, double *c, size_t m, size_t k, size_t n,
    double *A, double *B, size_t L2, size_t L3);
void printoffm64(double *m, size_t mx, size_t my, size_t ox, size_t oy,
    size_t dx, size_t dy);
void printm64(double *m, size_t mx, size_t my);
void iotaoffm64(double v, double d, double *m, size_t mx, size_t my, size_t ox,
    size_t oy, size_t dx, size_t dy);
void iotam64(double v, double d, double *m, size_t mx, size_t my);
int  eqm64(double *a, double *b, size_t x, size_t y);
void chordm64(double *mt, double *m, size_t mx, size_t my);

#endif // MM_H
