
.PHONY:	all gcc

include ../Makefile.flags

all:	gcc 

CPPFLAGS += -fPIC -I. $(LIBS) $(HTM_RETRIES) $(RETRY_POLICY) $(STARTING_MODE) $(NO_ADAPTIVITY) ${CONTROLLER_SLEEP} ${KPI_TRACKING} ${SMOOTHING} ${CONSECUTIVE_THRESHOLD}  ${ANOMALY_THRESHOLD}

# NOTES
#   lib.map enables to export only some functions
gcc: 	libitm.so libitm.a 

arch.o: arch.S
	$(CC) -O2 $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

%.do: 	%.c 
	$(CC) -O2 -fPIC $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

%.o: 	%.c 
	$(CC) -O2 $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

%.s: 	%.c 
	$(CC) -O2 $(CPPFLAGS) $(CFLAGS) -fverbose-asm -S -o $@ $<

libitm.a: abi.o arch.o 
	$(AR) cru $@ $^

libitm.so: 	abi.do arch.o 
	$(CC) -O2 $(CPPFLAGS) $(CFLAGS) -shared -Wl,--version-script,lib.map -o $@ $^

clean:
	rm -f *.o *.do libitm.a libitm.so
