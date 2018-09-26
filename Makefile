CFLAGS += -march=native
REL_CFLAGS = ${CFLAGS} -Ofast -s -funroll-all-loops 
DBG_CFLAGS = ${CFLAGS} -O0 -g

test: mmtest

bench: mmbench

mmtest: mmdbg
	gcc ${DBG_CFLAGS} mm.o mmtest.c -o mmtests
	./mmtests

mmbench: mmrel
	gcc ${REL_CFLAGS} mm.o mmbench.c -o mmbench
	./mmbench

mmdbg:
	gcc ${DBG_CFLAGS} -c mm.c

mmrel:
	gcc ${REL_CFLAGS} -c mm.c



clean:
	rm -f *.o mmtests
