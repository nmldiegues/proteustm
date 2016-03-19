# ==============================================================================
#
# Defines.common.mk
#
# ==============================================================================


CFLAGS += -DLIST_NO_DUPLICATES
CFLAGS += -DMAP_USE_AVLTREE
CFLAGS += -DSET_USE_RBTREE

PROG := yada
SRCS += \
	coordinate.cpp \
	element-stm.cpp \
	element-htm.cpp \
	mesh-stm.cpp \
	mesh-htm.cpp \
	region-stm.cpp \
	region-htm.cpp \
	yada.cpp \
	$(LIB)/avltree.c \
	$(LIB)/heap-stm.cpp \
	$(LIB)/heap-htm.cpp \
	$(LIB)/list-stm.cpp \
	$(LIB)/list-htm.cpp \
	$(LIB)/mt19937ar.c \
	$(LIB)/pair-stm.cpp \
	$(LIB)/pair-htm.cpp \
	$(LIB)/queue-stm.cpp \
	$(LIB)/queue-htm.cpp \
	$(LIB)/random.c \
	$(LIB)/rbtree-stm.cpp \
	$(LIB)/rbtree-htm.cpp \
	$(LIB)/thread.c \
	$(LIB)/vector.c \
#
OBJS := ${SRCS:.c=.o}


# ==============================================================================
#
# End of Defines.common.mk
#
# ==============================================================================
