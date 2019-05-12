#ifndef UTILS_H
#define UTILS_H

#define BENCH(x, ...) { x, ""#x, (__VA_ARGS__) }
#define TEST(x) { x, ""#x}

struct Bnc {
    void (*fn)(void *);
    const char *wh;
    void *p;
};

struct Tst {
    int (*fn)(void);
    const char *wh;
};

void bench(const char *, struct Bnc *);
size_t test(const char *, struct Tst *);

#endif // UTILS_H
