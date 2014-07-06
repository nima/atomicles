BIN     := lock daemonize #syschk ia32emu
CC_OPTS := -DARCH_$(shell uname -m)

.PHONY: all
all: $(BIN)

.PHONY: lock
lock: ../bin/lock
../bin/lock: lock.c lock.h SharedMemory.c SharedMemory.h Semaphore.c Semaphore.h
	gcc ${CC_OPTS} -DSHMKEYPATH='"$(shell realpath $(@F).h)"' -c -Wall SharedMemory.c -o SharedMemory.o
	gcc ${CC_OPTS} -DSEMKEYPATH='"$(shell realpath $(@F).c)"' -c -Wall Semaphore.c -o Semaphore.o
	gcc ${CC_OPTS} -lm -Wall $< SharedMemory.o Semaphore.o -o $@

.PHONY: daemonize
daemonize: ../bin/daemonize
../bin/daemonize: daemonize.c daemonize.h SharedMemory.c SharedMemory.h Semaphore.c Semaphore.h
	gcc ${CC_OPTS} -DSHMKEYPATH='"$(shell realpath $(@F).h)"' -c -Wall SharedMemory.c -o SharedMemory.o
	gcc ${CC_OPTS} -DSEMKEYPATH='"$(shell realpath $(@F).c)"' -c -Wall Semaphore.c -o Semaphore.o
	gcc ${CC_OPTS} -lm -Wall $< SharedMemory.o Semaphore.o -o $@

#../bin/ia32emu: ia32emu.c registers.o bits.o SharedMemory.o Semaphore.o
#	gcc ${CC_OPTS} -lm -g -Wall $^ -o $@
#registers.o: registers.c registers.h
#	gcc ${CC_OPTS} -c -Wall $< -o $@
#bits.o: bits.c bits.h
#	gcc ${CC_OPTS} -c -Wall $< -o $@

#../bin/syschk: syschk.c SharedMemory.o Semaphore.o
#	gcc ${CC_OPTS} -g -Wall $^ -o $@

#../bin/mess: mess.l mess.y
#	@echo http://ds9a.nl/lex-yacc/cvs/lex-yacc-howto.html
#	flex $@.l
#	bison -d $@.y
#	gcc ${CC_OPTS} lex.yy.c y.tab.c -o $@

.PHONY: purge
purge: clean; -rm -f $(BIN:%=../bin/%)

.PHONY: clean
clean:; -rm -f *.o *.s *.py[co] a.out
