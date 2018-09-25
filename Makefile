CFLAGS += -march=native
REL_CFLAGS = ${CFLAGS} -Ofast -s -funroll-all-loops 
DBG_CFLAGS += ${CFLAGS} -O0 -g

tests: mm
	gcc ${DBG_CFLAGS} mm.o mm_tests.c -o mm_tests

mm:
	gcc ${DBG_CFLAGS} -c mm.c

release:
	gcc ${REL_CFLAGS} -c mm.c

clean:
	rm -f *.o mm_tests
