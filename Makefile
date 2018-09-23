CFLAGS += -march=native -Ofast

tests: mm
	gcc ${CFLAGS} mm.o mm_tests.c -o mm_tests

mm:
	gcc ${CFLAGS} -c mm.c

clean:
	rm -f *.o mm_tests
