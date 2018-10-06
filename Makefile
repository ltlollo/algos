CFLAGS += -march=native -Wall -Wextra -pedantic -pthread
LDFLAGS += -Wl,--gc-sections
REL_CFLAGS = ${CFLAGS}  -s -Ofast -funroll-all-loops -minline-all-stringops \
	-ffunction-sections -fdata-sections -DNDEBUG
DBG_CFLAGS = ${CFLAGS} -O0 -g

test: mmtest
bench: mmbench
mmtest:  mmdbg; $(CC) ${LDFLAGS} ${DBG_CFLAGS} mm.o mmtest.c  -o mmtest;  ./$@
mmbench: mmrel; $(CC) ${LDFLAGS} ${REL_CFLAGS} mm.o mmbench.c -o mmbench; ./$@
%dbg:  ; $(CC) ${DBG_CFLAGS} -c $*.c
%rel:  ; $(CC) ${REL_CFLAGS} -c $*.c
clean: ; rm -f *.o mmtests
