// This is free and unencumbered software released into the public domain.
// For more information, see LICENSE

#include <stdio.h>
#include <time.h>

#define BENCH(x, ...) { x, ""#x, (__VA_ARGS__) }
#define TEST(x) { x, ""#x}

struct Par {
    size_t times;
};

struct Bnc {
    void (*fn)(void *);
    const char *wh;
    void *p;
};

struct Tst {
    int (*fn)(void);
    const char *wh;
};

void
bench(const char *str, struct Bnc *benchs) {
    float ts, tts = 0;
    struct timespec s, e;
    static char *fmt = "\t[%s]: time: %f ms, n: %ld iter, rate: %f ms/iter\n";

    printf("[%s]: bench\n", str);
    for (struct Bnc *bench = benchs; bench->fn; bench++) {
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &s);
        bench->fn(bench->p);
        struct Par *p = bench->p;
        clock_gettime(CLOCK_THREAD_CPUTIME_ID, &e);
        ts = (e.tv_sec - s.tv_sec) * 1e3 + (e.tv_nsec - s.tv_nsec) * 1e-6;
        printf(fmt, bench->wh, ts, p->times, ts / p->times);
        tts += ts;
    }
    printf("[%s]: %fms\n", str, tts);
}

size_t
test(const char *str, struct Tst *tests) {
    size_t res = 0, tot = 0;

    printf("[%s]: test\n", str);
    for (struct Tst *test = tests; test->fn; test++, tot++) {
        int t = test->fn();
        printf("\t[%s]: %s\n", test->wh, t ? "OK" : "FAIL");
        res += (t == 0);
    }
    printf("[%s]: %ld/%ld fail\n", str, res, tot);
    return res;
}
