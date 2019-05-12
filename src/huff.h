// This is free and unencumbered software released into the public domain.
// For more information, see LICENSE

#ifndef HUFF_H
#define HUFF_H

#include <stdint.h>
#include <stddef.h>

struct table {
    uint8_t size[256], sym[256][32];
};

void tnprint(struct table *, size_t);
void tinit(uint64_t *, struct table *);
int64_t tebufsize(struct table *, uint8_t *, size_t);
uint64_t tencode(struct table *, uint8_t *, size_t, uint8_t *);
void tdecode(struct table *, uint8_t *, size_t, uint8_t *, size_t);

#endif // HUFF_H
