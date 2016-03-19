# ==============================================================================
#
# Defines.common.mk
#
# ==============================================================================


PROG := intruder

SRCS += \
	decoder-stm-v2.cpp \
	decoder-htm-v2.cpp \
	detector.cpp \
	dictionary.cpp \
	intruder.cpp \
	packet.cpp \
	preprocessor.cpp \
	stream-stm-v2.cpp \
	stream-htm-v2.cpp \
	$(LIB)/list-stm-v2.cpp \
	$(LIB)/list-htm-v2.cpp \
	$(LIB)/mt19937ar.c \
	$(LIB)/pair-stm-v2.cpp \
	$(LIB)/pair-htm-v2.cpp \
	$(LIB)/queue-stm-v2.cpp \
	$(LIB)/queue-htm-v2.cpp \
	$(LIB)/random.c \
	$(LIB)/rbtree-stm-v2.cpp \
	$(LIB)/rbtree-htm-v2.cpp \
	$(LIB)/thread.c \
	$(LIB)/vector.c \
#
OBJS := ${SRCS:.c=.o}

CFLAGS += -DMAP_USE_RBTREE


# ==============================================================================
#
# End of Defines.common.mk
#
# ==============================================================================
