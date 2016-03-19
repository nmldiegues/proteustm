CC      := gcc
CFLAGS  := -g -Wall

SRCS := \
	mt19937ar.c \
	random.c \
#
OBJS := ${SRCS:.c=.o}

RM := rm -f

.PHONY: clean
clean:
	$(RM) $(OBJS)