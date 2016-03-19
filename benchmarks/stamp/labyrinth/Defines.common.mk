# ==============================================================================
#
# Defines.common.mk
#
# ==============================================================================


LIBS += -lm

PROG := labyrinth

SRCS += \
	coordinate.cpp \
	grid.cpp \
	labyrinth.cpp \
	maze.cpp \
	router.cpp \
	$(LIB)/list-stm.cpp \
	$(LIB)/list-htm.cpp \
	$(LIB)/mt19937ar.c \
	$(LIB)/pair-stm.cpp \
	$(LIB)/pair-htm.cpp \
	$(LIB)/queue-stm.cpp \
	$(LIB)/queue-htm.cpp \
	$(LIB)/random.c \
	$(LIB)/thread.c \
	$(LIB)/vector.c \
#
OBJS := ${SRCS:.c=.o}

CFLAGS += -DUSE_EARLY_RELEASE


# ==============================================================================
#
# End of Defines.common.mk
#
# ==============================================================================
