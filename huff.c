#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#if __AVX2__
#include <immintrin.h>
#endif

#define MIN(a, b) ((a) > (b) ? (b) : (a))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define align(n) __attribute__((aligned(n)))

struct table {
    uint8_t size[256];
    uint8_t sym[256][32];
};

struct state {
    uint64_t pos;
    uint8_t off;
};

void tnprint(struct table *, size_t);
void tinit(uint64_t *, struct table *);
int64_t tebufsize(struct table *, uint8_t *, size_t);
uint64_t tencode(struct table *, uint8_t *, size_t, uint8_t *);
void tdecode(struct table *, uint64_t *, uint8_t *, size_t, uint8_t *, size_t,
        struct state *);

static void shl256r(uint8_t *restrict, uint8_t *restrict);
static void shl256o(uint8_t *, uint8_t *);
static void gmerge(int16_t *restrict, int16_t *restrict);
static int mskcmp32(uint8_t *align(32), uint8_t *, uint8_t *align(32));

int
main() {
    uint64_t prob[256];
    for (size_t i = 0; i < 256; i++) {
        prob[i] = 1;
    }
    prob[1] = 100;
    prob[0] = 50;
    prob[2] = 30;
    prob[3] = 20;

    struct table t;
    tinit(prob, &t);
    tnprint(&t, 4);

    uint8_t buf[] = { 0, 1, 2, 3 };
    uint64_t tbs = tebufsize(&t, buf, sizeof(buf));
    uint8_t *eout = malloc(tbs);
    uint8_t dout[256];
    struct state state = { .pos = 0, .off = 0 };

    if (eout == NULL) {
        return 1;
    }
    tencode(&t, buf, sizeof(buf), eout);
    tdecode(&t, prob, eout, tbs, dout, sizeof(dout), &state);

    return 0;
}

static void
psort(uint64_t *prob, uint8_t *sym) {
    uint8_t bsym[2][256];
    uint64_t bprob[2][256];
    uint64_t mask = 1;

    /* 2-bucket radix sort sym[] and prob[], low prob first */
    for (size_t j = 0; j < 64; j++, mask <<= 1) {
        int bsize[2] = { 0, 0 };
        for (size_t i = 0; i < 256; i++) {
            int bnum = (prob[i] & mask) >> j;
            bprob[bnum][bsize[bnum]] = prob[i];
            bsym[bnum][bsize[bnum]++] = sym[i];
        }
        memcpy(prob, bprob[0], bsize[0] * sizeof(*prob));
        memcpy(prob + bsize[0], bprob[1], bsize[1] * sizeof(*prob));
        memcpy(sym, bsym[0], bsize[0] * sizeof(*sym));
        memcpy(sym + bsize[0], bsym[1], bsize[1] * sizeof(*sym));
    }
}

void
tinit(uint64_t *iprob, struct table *table) {
    uint8_t sym[256];
    uint64_t prob[256];
    int16_t buf[256][256];

    memset(table, 0, sizeof(*table));
    memcpy(prob, iprob, 256 * sizeof(*prob));

    for (size_t i = 0; i < 256; i++) {
        sym[i] = i;
    }
    psort(prob, sym);

    for (size_t i = 0; i < 256; i++) {
        buf[i][0] = sym[i];
        for (size_t j = 1; j < 256; j++) {
            buf[i][j] = -1;
        }
    }
    /* 256-bucket merge according to cumulate prob[] */
    for (size_t i = 1; i < 256; i++) {
        /* find merge candidates */
        int16_t from_group = sym[i - 1];
        int16_t to_group = sym[i];
        int16_t *groupa, *groupb;
        for (size_t i = 0; i < 256; i++) {
            for (size_t j = 0; j < 256 && buf[i][j] != -1; j++) {
                if (buf[i][j] == from_group) {
                    groupa = buf[i];
                    i = 256;
                    break;
                }
            }
        }
        for (size_t i = 0; i < 256; i++) {
            for (size_t j = 0; j < 256 && buf[i][j] != -1; j++) {
                if (buf[i][j] == to_group) {
                    groupb = buf[i];
                    i = 256;
                    break;
                }
            }
        }
        /* low prob candidates follow the 1 branch */
        for (int16_t *it = groupa; *it != -1; it++) {
            table->size[*it]++;
            shl256o(table->sym[*it], table->sym[*it]);
            table->sym[*it][0] |= 1;
        }
        /* high prob candidates follow the 0 branch */
        for (int16_t *it = groupb; *it != -1; it++) {
            table->size[*it]++;
            shl256o(table->sym[*it], table->sym[*it]);
        }
        /* merge buckets */
        gmerge(MIN(groupa, groupb), MAX(groupa, groupb));
        prob[i] += prob[i - 1];
        /* reorder remaning prob[] and sym[] */
        for (size_t j = 256 - 1; j != i; j--) {
            if (prob[i] > prob[j]) {
                uint64_t ptmp = prob[i];
                uint8_t stmp = sym[i];
                memmove(prob + i, prob + i + 1, (j - i) * sizeof(*prob));
                memmove(sym + i, sym + i + 1, (j - i) * sizeof(*sym));
                prob[j] = ptmp;
                sym[j] = stmp;
                break;
            }
        }
    }
}

static void
gmerge(int16_t *restrict a, int16_t *restrict b) {
    while (*a != -1) a++;
    while (*b != -1) *a++ = *b++;
}

int64_t
tebufsize(struct table *table, uint8_t *buf, size_t isize) {
    uint64_t size[9] = { 0, }, *csize = size, res = 0, rst = 0;

    for (uint64_t i = 0; i < isize; i++) {
        *csize += table->size[buf[i]];
        if ((*csize += table->size[buf[i]]) > 0x7fffffffffffffff) {
            csize++;
        }
    }
    for (size_t i = 0; i < 8; i++) {
        res += size[i] / 8, rst += size[i] % 8;
    }
    if (size[8]) {
        return -1;
    }
    return res + rst / 8 + !!(rst % 8) + 32;
}

uint64_t
tencode(struct table *table, uint8_t *buf, size_t isize, uint8_t *out) {
    uint8_t sym[256][8][32];

    for (size_t i = 0; i < 256; i++) {
        memcpy(sym[i][0], table->sym[i], 32);
        for (size_t j = 1; j < 8; j++) {
            shl256r(sym[i][j], sym[i][j - 1]);
        }
    }
    uint8_t off = 0;
    uint64_t pos = 0;
    for (size_t i = 0; i < isize; i++) {
        uint8_t ch = buf[i], *it = sym[ch][off];
        for (size_t j = 0; j < 32; j++) {
            out[pos + j] |= it[j];
        }
        pos += (table->size[ch] + off) / 8;
        off = (off + table->size[ch]) & 7;
    }
    return pos + !!off;
}

void
tdecode(struct table *table, uint64_t *iprob, uint8_t *ibuf, size_t isize,
        uint8_t *obuf, size_t osize, struct state *state) {
    union {
        struct aux {
            uint8_t data[32];
            uint8_t mask[32];
        } sym[256][8];
        char __[sizeof(struct aux) * 256 * 8 + 1];
    } mm;
    uint8_t sym[256];
    uint64_t prob[256];
    uint8_t reprsym[256][32];
    uint8_t reprsize[256];

    for (size_t i = 0; i < 256; i++) {
        prob[i] = (~0ull) - iprob[i];
    }
    for (size_t i = 0; i < 256; i++) {
        sym[i] = i;
    }
    psort(prob, sym);

    for (size_t i = 0; i < 256; i++) {
        memcpy(reprsym + i, table->sym + sym[i], sizeof(reprsym[i]));
    }
    for (size_t i = 0; i < 256; i++) {
        memcpy(reprsize + i, table->size + sym[i], sizeof(reprsize[i]));
    }
    /* prepare symbol offset table and mask */
    for (size_t i = 0; i < 256; i++) {
        memcpy(mm.sym[i][0].data, reprsym[i], 32);
        size_t j;
        for (j = 0; j < 32; j++) {
            mm.sym[i][0].mask[j] = 0;
        }
        for (j = 0; j < reprsize[i] / 8; j++) {
            mm.sym[i][0].mask[j] = 0xff;
        }
        uint8_t rst = (reprsize[i] % 8);
        mm.sym[i][0].mask[j] =  ~((0xffu >> rst) << rst);

        for (size_t j = 1; j < 8; j++) {
            shl256r(mm.sym[i][j].data, mm.sym[i][j - 1].data);
            shl256r(mm.sym[i][j].mask, mm.sym[i][j - 1].mask);
        }
    }
    uint8_t off = state->off;
    size_t pos = state->pos;
    uint8_t *oit = obuf, *oend = obuf + osize;
    
    do {
        for (size_t i = 0; i < 256; i++) {
            int res = mskcmp32(mm.sym[i][off].data, ibuf + pos,
                mm.sym[i][off].mask
            );
            if (res) {
                *oit++ = sym[i];
                pos += reprsize[i] / 8;
                off += reprsize[i] % 8;
                break;
            } else {
                continue;
            }
        }
        if (oit >= oend) {
            *state = (struct state){ .pos = pos, .off = off, };
            return;
        }
        if (pos + 32 == isize) {
            return;
        }
    } while (1);
}

void
tnprint(struct table *table, size_t size) {
    uint8_t *tsize = (void *)table->size;
    uint8_t *tsym = (void *)table->sym;
    char str[32 * 8], *buf = str;
    /* non-cyclic 8 length binary De Bruijn sequence (see: A169674)*/
    static char repr[256 + 7] = {
        "000000001000000110000010100000111000010010000101100001101000011110001"
        "000100110001010100010111000110010001101100011101000111110010010100100"
        "111001010110010110100101111001100110101001101110011101100111101001111"
        "11010101011101011011010111110110111101110111111110000000"
    };
    static uint8_t off[256] = {
        "\x00\x01\x02\x09\x03\x11\x0a\x19\x04\x21\x12\x29\x0b\x31\x1a\x39\x05"
        "\x41\x22\x45\x13\x4d\x2a\x55\x0c\x5d\x32\x65\x1b\x6d\x3a\x75\x06\x26"
        "\x42\x62\x23\x7d\x46\x85\x14\x80\x4e\x8d\x2b\x95\x56\x9d\x0d\x49\x5e"
        "\xa5\x33\xa9\x66\xb1\x1c\x88\x6e\xb9\x3b\xc1\x76\xc9\x07\x17\x27\x37"
        "\x43\x53\x63\x73\x24\x83\x7e\x9b\x47\xaf\x86\xc7\x15\x51\x81\xad\x4f"
        "\xd1\x8e\xd3\x2c\x90\x96\xdb\x57\xd5\x9e\xe3\x0e\x2e\x4a\x6a\x5f\x92"
        "\xa6\xbe\x34\x98\xaa\xe0\x67\xdd\xb2\xeb\x1d\x59\x89\xb5\x6f\xd7\xba"
        "\xf3\x3c\xa0\xc2\xee\x77\xe5\xca\xf7\xff\x08\x10\x18\x20\x28\x30\x38"
        "\x40\x44\x4c\x54\x5c\x64\x6c\x74\x25\x61\x7c\x84\x7f\x8c\x94\x9c\x48"
        "\xa4\xa8\xb0\x87\xb8\xc0\xc8\x16\x36\x52\x72\x82\x9a\xae\xc6\x50\xac"
        "\xd0\xd2\x8f\xda\xd4\xe2\x2d\x69\x91\xbd\x97\xdf\xdc\xea\x58\xb4\xd6"
        "\xf2\x9f\xed\xe4\xf6\xfe\x0f\x1f\x2f\x3f\x4b\x5b\x6b\x60\x7b\x8b\x93"
        "\xa3\xa7\xb7\xbf\x35\x71\x99\xc5\xab\xcf\xd9\xe1\x68\xbc\xde\xe9\xb3"
        "\xf1\xec\xf5\xfd\x1e\x3e\x5a\x7a\x8a\xa2\xb6\x70\xc4\xce\xd8\xbb\xe8"
        "\xf0\xf4\xfc\x3d\x79\xa1\xc3\xcd\xe7\xef\xfb\x78\xcc\xe6\xfa\xcb\xf9"
        "\xf8"
    };
    for (size_t i = 0; i < size; i++, buf = str) {
        uint8_t *beg = tsym + i * 32;
        uint8_t s = tsize[i];
        uint8_t soff = off[beg[s / 8]] + 8 - s % 8;
        for (int i = 0; i < s % 8; i++) {
            *buf++ = repr[soff + i];
        }
        for (size_t j = 0; j < s / 8; j++) {
            uint8_t soff = off[beg[s / 8 - j]];
            for (int i = 0; i < 8; i++) {
                *buf++ = repr[soff + i];
            }
        }
        *buf = '\0';
        printf("%s\n", str);
    }
}

static void
shl256r(uint8_t *restrict dst, uint8_t *restrict src) {
    for (uint8_t rst0 = 0, k = 0; k < 32; k++) {
        dst[k] = rst0 | (src[k] << 1);
        rst0 = src[k] >> 7;
    }
}

static void
shl256o(uint8_t *dst, uint8_t *src) {
    for (uint8_t rst0 = 0, k = 0, tmp; k < 32; k++) {
        tmp = src[k];
        dst[k] = rst0 | (tmp << 1);
        rst0 = tmp >> 7;
    }
}

#if __AVX2__
static int
mskcmp32(uint8_t *align(32) a, uint8_t *b, uint8_t *align(32) msk) {
    __m256i ra = _mm256_load_si256((void *)a);
    __m256i rb = _mm256_loadu_si256((void *)b);
    __m256i rm = _mm256_load_si256((void *)msk);
    __m256i cmp = _mm256_cmpeq_epi8(ra, _mm256_and_si256(rm, rb));
    return _mm256_movemask_epi8(cmp) == ~0;
}
#else
static int
mskcmp32(uint8_t *align(32) a, uint8_t *b, uint8_t *align(32) msk) {
    int res = 1;

    for (size_t j = 0; j < 32 && res; j++) {
        res &= a[j] == (msk[j] & b[j]);
    }
    return res;
}
#endif
