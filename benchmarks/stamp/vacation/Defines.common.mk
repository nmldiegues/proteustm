# ==============================================================================
#
# Defines.common.mk
#
# ==============================================================================


CFLAGS += -DLIST_NO_DUPLICATES
CFLAGS += -DMAP_USE_RBTREE

PROG := vacation

SRCS += \
	client.cpp \
	customer-stm.cpp \
	customer-htm.cpp \
	manager-stm.cpp \
	manager-htm.cpp \
	reservation-stm.cpp \
	reservation-htm.cpp \
	vacation.cpp \
	$(LIB)/list-stm.cpp \
	$(LIB)/list-htm.cpp \
	$(LIB)/pair-stm.cpp \
	$(LIB)/pair-htm.cpp \
	$(LIB)/mt19937ar.c \
	$(LIB)/random.c \
	$(LIB)/rbtree-stm.cpp \
	$(LIB)/rbtree-htm.cpp \
	$(LIB)/thread.c \
#
OBJS := ${SRCS:.c=.o}


# ==============================================================================
#
# End of Defines.common.mk
#
# ==============================================================================
