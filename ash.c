#ifndef ASH
#define ASH

#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#define KEY_EMPTY (~0u)

struct Store {
    struct Store *next;
    size_t curr;
    size_t size;
    char data[];
};

struct Key {
    uint32_t size;
    uint32_t hash;
    void *mm;
};

struct KeyPos {
    struct Key key;
    uint32_t pos;
    uint8_t present;
};

struct HashMapMem {
    uint32_t size;
    uint8_t ord;
    uint16_t vsize;
    void *keys;
    void *values;
    struct Store *store;
};

struct Store *store_init(size_t);
void store_free(struct Store *);
void *store_add(struct Store **, void *, size_t);

int hmap_init(struct HashMapMem *, uint32_t, uint16_t, struct Store *);
void hmap_free(struct HashMapMem *);
int hmap_insert(struct HashMapMem *, void *, int32_t, void *);
int hmap_at(struct HashMapMem *, void *, int32_t, struct KeyPos *);
int hmap_insert_at(struct HashMapMem *, struct KeyPos *, void *);
void *hmap_get(struct HashMapMem *, struct KeyPos *);

struct Store *
store_init(size_t size) {
    struct Store *data;
    if ((data = malloc(sizeof (struct Store) + size)) == NULL) {
        return data;
    }
    data->next = NULL;
    data->curr = 0;
    data->size = size;
    return data;
}

void
store_free(struct Store *s) {
    struct Store *next;
    while (s) {
        next = s->next;
        free(s);
        s = next;
    }
}

void *
store_add(struct Store **sptr, void *mm, size_t mmsz) {
    struct Store *s = *sptr;
    
    if (s->curr + mmsz > s->size) {
        size_t size = s->size + s->size / 2 + mmsz;
        struct Store *data = malloc(sizeof (struct Store) + size);
        if (data == NULL) {
            return NULL;
        }
        data->next = NULL;
        data->curr = 0;
        data->size = size;
        s->next = data;
        *sptr = s = data;
    }
    void *res = s->data + s->curr;
    memcpy(res, mm, mmsz);
    s->curr += mmsz;
    return res;
}

static ssize_t
hmap_ord(uint32_t size) {
    for (int i = 4; i < 32; i++) {
        if (size < (1u << i)) {
            return i;
        };
    }
    return 32;
}

int
hmap_init(struct HashMapMem *hmm
    , uint32_t size
    , uint16_t vsize
    , struct Store *store
) {
    ssize_t ord = hmap_ord(size);
    hmm->values = NULL;

    if ((hmm->keys = malloc((1 << ord) * sizeof (struct Key))) == NULL) {
        return -1;
    }
    if (vsize && (hmm->values = malloc((1 << ord) * vsize)) == NULL) {
        free(hmm->keys);
        return -1;
    }
    size_t ksz = (1u << ord) * sizeof (struct Key);
    uint64_t *curr = hmm->keys, *end = hmm->keys + ksz;
    while (curr < end) {
        curr[0] = curr[1] = ~0ul;
        curr += 2;
    }
    hmm->ord = ord;
    hmm->vsize = vsize;
    hmm->size = 0;
    hmm->store = store;
    return 0;
}

void
hmap_free(struct HashMapMem *hmm) {
    free(hmm->keys);
    free(hmm->values);
}

static uint32_t
hmap_hash(void *mm, int32_t size, uint8_t ord) {
    uint32_t hash = 0;
    uint32_t *ucurr = mm;
    uint32_t rest = size % sizeof (uint32_t);
    uint32_t *uend = mm + size - rest;

    while (ucurr < uend) {
        hash += *ucurr++;
    }
    uint8_t * ccurr = (void *)ucurr;

    switch (rest) {
        default: break;
        case 3: hash += (ccurr[2]) << 0x10; __attribute__((fallthrough));
        case 2: hash += (ccurr[1]) << 0x08; __attribute__((fallthrough));
        case 1: hash += (ccurr[0]) << 0x00; __attribute__((fallthrough));
    }
    if (ord < 5) {
        hash += hash >> 4;
    }
    return hash;
}

static int
hmap_bumpord(struct HashMapMem *hmm) {
    if (hmm->ord == 32) {
        errno = ENOMEM;
        return -1;
    }
    uint8_t ord = hmm->ord + 1;
    size_t ksz = (1u << ord) * sizeof (struct Key);
    size_t vsz = (1u << ord) * hmm->vsize;
    struct Key *keys;
    void *values;

    if ((keys = malloc(ksz)) == NULL) {
        return -1;
    }
    if (vsz && (values = malloc(vsz)) == NULL) {
        free(keys);
        return -1;
    }
    uint64_t *curr = (void *)keys, *end = (void *)keys + ksz;
    while (curr < end) {
        curr[0] = curr[1] = ~0ul;
        curr += 2;
    }
    struct Key *kbeg = hmm->keys, *kcurr = hmm->keys;
    struct Key *kend = hmm->keys + (1 << hmm->ord) * sizeof (struct Key); 

    for (; kcurr < kend; kcurr++) {
        if (kcurr->size == KEY_EMPTY) {
            continue;
        }
        uint32_t hash = hmap_hash(kcurr->mm, kcurr->size, ord);

        for (size_t i = 0; i < 1u << ord; i++) {
            uint32_t pos = (i + hash) & ((1u << ord) - 1);
            struct Key *k = keys + pos;
            void *value = hmm->values + (kcurr - kbeg) * hmm->vsize;
            __builtin_prefetch(value, 0, 0);
            __builtin_prefetch(values + pos * hmm->vsize, 1, 0);
            if (k->size == KEY_EMPTY) {
                k->size = kcurr->size;
                k->hash = hash;
                k->mm = kcurr->mm;
                memcpy(value, values + pos * hmm->vsize, hmm->vsize); 
                break;
            }
        }
    }
    free(hmm->keys);
    free(hmm->values);
    hmm->ord = ord;
    hmm->keys = keys;
    hmm->values = values;
    return 0;
}

static int
mmeq(void *restrict a, void *restrict b, size_t sz) {
    char *restrict f = a, *restrict s = b;

    for (size_t i = 0; i < sz; i++) {
        if (f[i] != s[i]) {
            return 0;
        }
    }
    return 1;
}

int
hmap_insert(struct HashMapMem *hmm, void *k, int32_t ksz, void *val) {
    int res = 0;
    uint8_t ord = hmm->ord;
    uint32_t alloc = 1 << ord;
    uint16_t vsize = hmm->vsize;

    if (hmm->size + 1 > alloc - alloc / 8 && (res = hmap_bumpord(hmm))) {
        return res;
    }
    uint32_t hash = hmap_hash(k, ksz, ord);
    struct Key *kbeg = hmm->keys, *kcurr;

    for (size_t i = 0; i < 1u << ord; i++) {
        uint32_t pos = (i + hash) & ((1u << ord) - 1);
        kcurr = kbeg + pos;
        if (kcurr->size == KEY_EMPTY) {
            kcurr->size = ksz;
            kcurr->hash = hash;
            if (hmm->store && (k = store_add(&hmm->store, k, ksz)) == NULL) {
                return -1;
            }
            kcurr->mm = k;
            memcpy(hmm->values + pos * vsize, val, vsize);
            break;
        } else if (kcurr->size != (uint32_t)ksz
            || kcurr->hash != hash
            || !mmeq(kcurr->mm, k, ksz)
        ) {
            continue;
        }
        memcpy(hmm->values + pos * vsize, val, vsize);
        break;
    }
    hmm->size++;
    return res;
}

int
hmap_at(struct HashMapMem *hmm, void *k, int32_t ksz, struct KeyPos *kp) {
    uint8_t ord = hmm->ord;
    uint32_t hash = hmap_hash(k, ksz, ord);
    struct Key *kbeg = hmm->keys, *kcurr;

    for (size_t i = 0; i < 1u << ord; i++) {
        uint32_t pos = (i + hash) & ((1u << ord) - 1);
        kcurr = kbeg + pos;
        __builtin_prefetch(hmm->values + pos * hmm->vsize, 0, 3);
        if (kcurr->size == KEY_EMPTY) {
            break;
        } else if (kcurr->size != (uint32_t)ksz
            || kcurr->hash != hash
            || !mmeq(kcurr->mm, k, ksz)
        ) {
            continue;
        }
        break;
    }
    if (kcurr->size == KEY_EMPTY) {
        if (hmm->store && (k = store_add(&hmm->store, k, ksz)) == NULL) {
            return -1;
        }
    } else {
        k = kcurr->mm;
    }
    *kp = (struct KeyPos) {
        .key = { .hash = hash, .mm = k, .size = ksz, },
        .pos = kcurr - kbeg,
        .present = kcurr->size != KEY_EMPTY,
    };
    return 0;
}

int
hmap_insert_at(struct HashMapMem *hmm, struct KeyPos *kp, void *val) {
    int res = 0;
    struct Key *kbeg = hmm->keys;
    uint32_t pos = kp->pos;
    struct Key *k = kbeg + pos;
    uint8_t ord = hmm->ord;
    uint32_t alloc = 1 << ord;
    uint16_t vsize = hmm->vsize;

    *k = kp->key;
    memcpy(hmm->values + pos * vsize, val, vsize);
    if (hmm->size + 1 > alloc - alloc / 8) {
        res = hmap_bumpord(hmm);
    }
    return res;
}

void *
hmap_get(struct HashMapMem *hmm, struct KeyPos *kp) {
    return hmm->values + kp->pos * hmm->vsize;
}

#endif
