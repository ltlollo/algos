// This is free and unencumbered software released into the public domain.
// For more information, see LICENSE

#ifndef MM_H
#define MM_H
#include <stdint.h>

void dgemmf32(float *a, float *b, float *restrict c, size_t m, size_t k, size_t n,
    float *A, float *B, size_t L2, size_t L3);
void printoffmf32(float *m, size_t mx, size_t my, size_t ox, size_t oy,
    size_t dx, size_t dy);
void printmf32(float *m, size_t mx, size_t my);
void iotaoffmf32(float v, float d, float *m, size_t mx, size_t my, size_t ox,
    size_t oy, size_t dx, size_t dy);
void iotamf32(float v, float d, float *m, size_t mx, size_t my);
int  eqmf32(float *a, float *b, size_t x, size_t y);
void chordmf32(float *mt, float *m, size_t mx, size_t my);
void Tmf32(float *m, float *restrict mc, size_t mx, size_t my);
float *mmallocf32(size_t m, size_t n);

void gemm64(double *a, double *b, double *restrict c, size_t m, size_t k, size_t n,
    double *A, double *B, size_t L2, size_t L3);
void printoffm64(double *m, size_t mx, size_t my, size_t ox, size_t oy,
    size_t dx, size_t dy);
void printm64(double *m, size_t mx, size_t my);
void iotaoffm64(double v, double d, double *m, size_t mx, size_t my, size_t ox,
    size_t oy, size_t dx, size_t dy);
void iotam64(double v, double d, double *m, size_t mx, size_t my);
int  eqm64(double *a, double *b, size_t x, size_t y);
void chordm64(double *mt, double *m, size_t mx, size_t my);
void Tmf64(double *m, double *restrict mc, size_t mx, size_t my);
double *mmallocf64(size_t m, size_t n);

#endif // MM_H
