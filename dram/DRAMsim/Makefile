CC = g++
ifeq ($(MACHTYPE), i386)
OFLAGS = -O6 -mcpu=i686 -malign-double -funroll-all-loops\
		 -ffast-math -fno-exceptions -m32
endif
ifeq ($(MACHTYPE), x86_64)
OFLAGS = -m32
endif
CFLAGS = -g -Wall -DMEM_TEST -fbounds-check $(OFLAGS)

CFILES = mem-biu.c mem-stat.c mem-dram.c mem-statemachine.c mem-address.c mem-dram-helper.c mem-commandissuetest.c mem-fileio.c mem-transactions.c mem-bundle.c mem-amb-buffer.c mem-refresh.c mem-issuecommands.c mem-dram-power.c mem-test.c 

OBJ_FILES = $(CFILES:.c=.o)

DRAMsim:$(OBJ_FILES)
	$(CC) $(CFLAGS) $(OFLAGS) -o DRAMsim $^ -lm 

mem-fileio.o: mem-fileio.c mem-tokens.h mem-system.h

mem-dram.o: mem-dram.c mem-system.h mem-biu.h

mem-dram-helper.o: mem-dram-helper.c mem-system.h mem-biu.h

mem-dram-power.o: mem-dram-power.c mem-system.h 

mem-commandissuetest.o: mem-commandissuetest.c mem-system.h mem-biu.h

mem-statemachine.o: mem-commandissuetest.c mem-system.h 

mem-bundle.o: mem-bundle.c mem-system.h mem-biu.h

mem-biu.o: mem-biu.c mem-system.h mem-biu.h

mem-stat.o: mem-stat.c mem-system.h mem-biu.h

mem-refresh.o: mem-refresh.c mem-system.h mem-biu.h 

mem-issuecommands.o: mem-issuecommands.c mem-system.h 

mem-address.o: mem-address.c mem-system.h 

mem-test.o: mem-test.c mem-system.h mem-biu.h

mem-transactions.o: mem-transactions.c mem-system.h mem-biu.h

clean:
	\rm -f *.o
	\rm -f DRAMsim
	\rm -f *~

tar:
	cd .. ; \
	tar cvf DRAMsim.tar DRAMsim ; \
	gzip DRAMsim.tar

