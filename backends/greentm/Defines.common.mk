CC       := g++
CFLAGS   += -std=c++11 -g -w -pthread -mrtm -fpermissive
CFLAGS   += -O2
CFLAGS   += -I$(LIB) -I../../../rapl-power/ -I ../../../stms/norec/ -I ../../../stms/tinystm/include -I ../../../stms/tl2/ -I ../../../stms/swisstm/include/
CPP      := g++
CPPFLAGS += $(CFLAGS)
LD       := g++
LIBS     += -lpthread

# Remove these files when doing clean
OUTPUT +=

LIB := ../lib
