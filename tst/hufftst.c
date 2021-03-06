// This is free and unencumbered software released into the public domain.
// For more information, see LICENSE

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "huff.h"
#include "tbh.h"

int
encode_decode(void) {
    uint64_t prob[256] = { 150, 100, 130, 120 };
    for (size_t i = 4; i < 256; i++) {
        prob[i] = 1;
    }
    struct table t;
    tinit(prob, &t);

    uint8_t buf[] = { 0, 1, 2, 3 };
    uint64_t tbs = tebufsize(&t, buf, sizeof(buf));
    uint8_t *eout = malloc(tbs);
    uint8_t dout[4];

    if (eout == NULL) {
        return 0;
    }
    tencode(&t, buf, sizeof(buf), eout);
    tdecode(&t, eout, tbs, dout, 4);

    return memcmp(dout, buf, 4) == 0;
}

int
main() {
    struct Tst tests[] = {
        TEST(encode_decode),
        TEST(NULL),
    };
    return test(__FILE__, tests);
}
