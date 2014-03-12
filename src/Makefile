BIN     := daemonize lock
#BIN     += syschk ia32emu
CC_OPTS := -DARCH_$(shell uname -m)

all: $(BIN)

lock: lock.c SharedMemory.o Semaphore.o
	gcc ${CC_OPTS} -Wall $^ -o $@

daemonize: daemonize.c daemonize.h SharedMemory.o Semaphore.o
	gcc ${CC_OPTS} -lm -Wall $^ -o $@


ia32emu: ia32emu.c registers.o bits.o SharedMemory.o Semaphore.o
	gcc ${CC_OPTS} -lm -g -Wall $^ -o $@
registers.o: registers.c registers.h
	gcc ${CC_OPTS} -c -Wall $< -o $@
bits.o: bits.c bits.h
	gcc ${CC_OPTS} -c -Wall $< -o $@

syschk: syschk.c SharedMemory.o Semaphore.o
	gcc ${CC_OPTS} -g -Wall $^ -o $@



SharedMemory.o: SharedMemory.c SharedMemory.h Semaphore.o
	gcc ${CC_OPTS} -c -Wall $< -o $@

Semaphore.o: Semaphore.c Semaphore.h
	gcc ${CC_OPTS} -c -Wall $< -o $@


clean:
	-rm -f *.o *.s *.py[co]
	-rm -f $(BIN) a.out

mess: mess.l mess.y
	@echo http://ds9a.nl/lex-yacc/cvs/lex-yacc-howto.html
	flex $@.l
	bison -d $@.y
	gcc ${CC_OPTS} lex.yy.c y.tab.c -o $@

.PHONY: all clean
