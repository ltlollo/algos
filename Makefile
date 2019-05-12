CFLAGS += -march=native -Wall -Wextra -pedantic -pthread
LDFLAGS += -Wl,--gc-sections
REL_CFLAGS = ${CFLAGS}  -s -Ofast -funroll-all-loops -minline-all-stringops \
	-ffunction-sections -fdata-sections -DNDEBUG
DBG_CFLAGS = ${CFLAGS} -O0 -g

test: 	mmtst cryptst hufftst
bench: 	mmbnc crypbnc huffbnc
.PHONY: clean test bench

%inc: src/%*.c
	echo "#include \"src/$*.c\"\nint main(){}"|$(CC) ${CFLAGS} -xc - -o/dev/null
%tst: %dbg %inc tbhdbg tst/%*.c
	$(CC) ${LDFLAGS} ${DBG_CFLAGS} ./$*.o ./tbh.o tst/$@.c -Isrc -o ./$@
	./$@
%bnc: %rel tbhrel bnc/%*.c
	$(CC) ${LDFLAGS} ${REL_CFLAGS} ./$*.o ./tbh.o bnc/$@.c -Isrc -o ./$@
	./$@
%dbg: src/%*.c
	$(CC) ${DBG_CFLAGS} -c src/$*.c -o ./$*.o
%rel: src/%*.c
	$(CC) ${REL_CFLAGS} -c src/$*.c -o ./$*.o
clean:
	rm -f *.o ?*tst ?*bnc
