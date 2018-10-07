CFLAGS += -march=native -Wall -Wextra -pedantic -pthread
LDFLAGS += -Wl,--gc-sections
REL_CFLAGS = ${CFLAGS}  -s -Ofast -funroll-all-loops -minline-all-stringops \
	-ffunction-sections -fdata-sections -DNDEBUG
DBG_CFLAGS = ${CFLAGS} -O0 -g

test: 	mmtst cryptst
bench: 	mmbch crypbch
.PHONY: clean test bench

%tst: %dbg 	; $(CC) ${LDFLAGS} ${DBG_CFLAGS} $*.o $@.c -o $@; ./$@
%bch: %rel 	; $(CC) ${LDFLAGS} ${REL_CFLAGS} $*.o $@.c -o $@; ./$@
%dbg: %*.c 	; $(CC) ${DBG_CFLAGS} -c $*.c
%rel: %*.c	; $(CC) ${REL_CFLAGS} -c $*.c
clean: 		; rm -f *.o *tst *bch
