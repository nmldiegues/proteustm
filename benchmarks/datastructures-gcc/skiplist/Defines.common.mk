PROG := skiplist

SRCS += \
	skiplist.cpp \
	$(LIB)/mt19937ar.c \
	$(LIB)/random.c \

#
OBJS := ${SRCS:.c=.o}
