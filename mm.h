// This is free and unencumbered software released into the public domain.
// For more information, see LICENSE

#ifndef MM_H
#define MM_H
#include <stdint.h>

#define NT_MAX (64)

void axpyf32(float *a, float b, float *c, size_t m);
void dgerf32(float *a, float *b, float *restrict c, size_t m, size_t n);
void dgemvf32(float *a, float *b, float *restrict c, size_t m, size_t n);
void dgemmf32(float *a, float *b, float *restrict c, size_t m, size_t k,
    size_t n, float *A, float *B, size_t L2, size_t L3);
void mtdgemmf32(float *a, float *b, float *restrict c, size_t m, size_t k,
    size_t n, size_t L2, size_t L3, size_t nt);
void printoffmf32(float *m, size_t mx, size_t my, size_t ox, size_t oy,
    size_t dx, size_t dy);
void printmf32(float *m, size_t mx, size_t my);
void iotaoffmf32(float v, float d, float *m, size_t mx, size_t my, size_t ox,
    size_t oy, size_t dx, size_t dy);
void iotamf32(float v, float d, float *m, size_t mx, size_t my);
int  eqmf32(float *a, float *b, size_t x, size_t y);
int  eqvf32(float *a, float *b, size_t y);
void chordmf32(float *mt, float *restrict m, size_t mx, size_t my);
void Tmf32(float *m, float *restrict mc, size_t mx, size_t my);
float *allocmf32(size_t m, size_t n);
float *allocvf32(size_t m);
void iotavf32(float v, float d, float *m, size_t my);
void iotaoffmf32(float v, float d, float *m, size_t my, size_t mx, size_t oy,
    size_t ox, size_t dy, size_t dx);
void printoffvf32(float *m, size_t my, size_t oy, size_t dy);
void printvf32(float *m, size_t my);
void printoffvTf32(float *m, size_t mx, size_t ox, size_t dx);
void printvTf32(float *m, size_t mx);

void axpyf64(double *a, double b, double *c, size_t m);
void dgerf64(double *a, double *b, double *restrict c, size_t m, size_t n);
void dgemvf64(double *a, double *b, double *restrict c, size_t m, size_t n);
void dgemmf64(double *a, double *b, double *restrict c, size_t m, size_t k,
    size_t n, double *A, double *B, size_t L2, size_t L3);
void mtdgemmf64(double *a, double *b, double *restrict c, size_t m, size_t k,
    size_t n, size_t L2, size_t L3, size_t nt);
void printoffmf64(double *m, size_t mx, size_t my, size_t ox, size_t oy,
    size_t dx, size_t dy);
void printmf64(double *m, size_t mx, size_t my);
void iotaoffmf64(double v, double d, double *m, size_t mx, size_t my, size_t ox,
    size_t oy, size_t dx, size_t dy);
void iotamf64(double v, double d, double *m, size_t mx, size_t my);
int  eqmf64(double *a, double *b, size_t x, size_t y);
int  eqvf64(double *a, double *b, size_t y);
void chordmf64(double *mt, double *restrict m, size_t mx, size_t my);
void Tmf64(double *m, double *restrict mc, size_t mx, size_t my);
double *allocmf64(size_t m, size_t n);
double *allocvf64(size_t m);
void iotavf64(double v, double d, double *m, size_t my);
void iotaoffmf64(double v, double d, double *m, size_t my, size_t mx, size_t oy,
    size_t ox, size_t dy, size_t dx);
void printoffvf64(double *m, size_t my, size_t oy, size_t dy);
void printvf64(double *m, size_t my);
void printoffvTf64(double *m, size_t mx, size_t ox, size_t dx);
void printvTf64(double *m, size_t mx);

#endif // MM_H
