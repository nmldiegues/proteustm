CC       := g++
CFLAGS   += -std=c++11 -g -w -pthread -mrtm -fpermissive
CFLAGS   += -O2
CFLAGS   += -I$(LIB) -I../../../backends/greentm/ -fgnu-tm -L../../../backends/greentm/gcc-abi/ -litm -I../../../rapl-power/ -I ../../../stms/norec/ -I ../../../stms/tinystm/include -I ../../../stms/tl2/ -I ../../../stms/swisstm/include/ -L../../../rapl-power/ -lrapl -L../../../stms/norec/ -L../../../stms/tinystm/lib -L../../../stms/tl2/ -L../../../stms/swisstm/lib/ -lstm -lnorec -ltl2 -lwlpdstm -ltcmalloc -lpthread
CPP      := g++
CPPFLAGS += $(CFLAGS)
LD       := g++
LIBS     += -lpthread
LDFLAGS  += $(CFLAGS)

# Remove these files when doing clean
OUTPUT +=

LIB := ../lib
