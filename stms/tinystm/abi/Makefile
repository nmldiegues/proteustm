
.PHONY:	all gcc

all:	gcc 

# Path to TinySTM
ROOT = ..

include $(ROOT)/Makefile.common

#TODO test if function are inline or need a specific header to force inlining
#TODO Flags may changed. I must find a way to keep up to date with current tiny. 

# DEFINES += -DDESIGN=WRITE_BACK_ETL
 DEFINES += -DDESIGN=WRITE_BACK_CTL
# DEFINES += -DDESIGN=WRITE_THROUGH

DEFINES += -DCM=CM_SUICIDE
# DEFINES += -DCM=CM_DELAY
# DEFINES += -DCM=CM_BACKOFF
# DEFINES += -DCM=CM_MODULAR

#DEFINES += -DEPOCH_GC
DEFINES += -UEPOCH_GC
# DEFINES += -DREAD_LOCKED_DATA

# DEFINES += -DLOCK_SHIFT_EXTRA=0
DEFINES += -DTM_STATISTICS
DEFINES += -UTM_STATISTICS2

DEFINES += -DIRREVOCABLE_ENABLED
# DEFINES += -UIRREVOCABLE_ENABLED

DEFINES += -UPTHREAD_WRAPPER

# Define how TLS is used in ABI (should be removed for next release)
DEFINES += -DTLS_COMPILER

DEFINES += -UHYBRID_ASF

DEF_ABI = $(subst -DEXPLICIT_TX_PARAMETER,,$(DEFINES))


##############################################################################
## GCC
##############################################################################
CPPFLAGS += -DTM_GCC -I.

# NOTES
#   lib.map enables to export only some functions
gcc: 	libitm.so libitm.a 

arch.o: arch.S
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEF_ABI) -c -o $@ $<

%.do: 	%.c 
	$(CC) -fPIC $(CPPFLAGS) $(CFLAGS) $(DEF_ABI) -c -o $@ $<

%.o: 	%.c 
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEF_ABI) -c -o $@ $<

%.s: 	%.c 
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEF_ABI) -fverbose-asm -S -o $@ $<

libitm.a: abi.o arch.o 
	$(AR) cru $@ $^

libitm.so: 	abi.do arch.o 
	$(CC) -fPIC $(CPPFLAGS) $(CFLAGS) -shared -Wl,--version-script,lib.map -o $@ $^

clean:
	rm -f *.o *.do libitm.a libitm.so