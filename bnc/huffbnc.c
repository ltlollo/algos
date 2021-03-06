// This is free and unencumbered software released into the public domain.
// For more information, see LICENSE

#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "huff.h"
#include "tbh.h"

struct benchpar {
    size_t times;
    struct table *table;
    uint8_t *encodebuf, *decodebuf;
    size_t encodebufsz, decodebufsz;
};

void
encode(void *p) {
    struct benchpar *in = p;
    size_t times = in->times;
    size_t encodebufsz = in->encodebufsz;
    uint8_t *encodebuf = in->encodebuf;
    uint8_t *decodebuf = in->decodebuf;
    struct table *table = in->table;

    for (size_t i = 0; i < times; i++) {
        tencode(table, encodebuf, encodebufsz, decodebuf);
    }
}

void
decode(void *p) {
    struct benchpar *in = p;
    size_t times = in->times;
    size_t encodebufsz = in->encodebufsz;
    size_t decodebufsz = in->decodebufsz;
    uint8_t *encodebuf = in->encodebuf;
    uint8_t *decodebuf = in->decodebuf;
    struct table *table = in->table;

    for (size_t i = 0; i < times; i++) {
        tdecode(table, decodebuf, decodebufsz, encodebuf, encodebufsz);
    }
}

int
main() {
    uint64_t prob[256] = { 150, 100, 130, 120 };
    for (size_t i = 4; i < 256; i++) {
        prob[i] = 1;
    }
    struct table table;
    tinit(prob, &table);

    size_t encodebufsz = 0x1000000;
    uint8_t *encodebuf = calloc(1, encodebufsz);
    uint64_t decodebufsz = tebufsize(&table, encodebuf, encodebufsz);
    uint8_t *decodebuf = malloc(decodebufsz);

    printf("encode size: %ld\n", encodebufsz);
    printf("encode size: %ld\n", decodebufsz);
    tencode(&table, encodebuf, encodebufsz, decodebuf);

    struct Bnc benchs[] = {
        BENCH(encode, &(struct benchpar) {
            1<<5, &table, encodebuf, decodebuf, encodebufsz, decodebufsz
        }),
        BENCH(decode, &(struct benchpar) {
            1<<5, &table, encodebuf, decodebuf, encodebufsz, decodebufsz
        }),
        BENCH(NULL, NULL),
    };
    bench(__FILE__, benchs);
}
