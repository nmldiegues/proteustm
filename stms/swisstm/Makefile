include Makefile.in

CPPFLAGS += -DUSE_STANDARD_IOSTREAM

OBJFILES = $(OBJ_DIR)/wlpdstm.o

####################################
# choose files to compile and link #
####################################

ifeq ($(STM), swisstm)
	OBJFILES += $(OBJ_DIR)/transaction.o $(OBJ_DIR)/privatization_tree.o
endif

.PHONY: clean

all: $(INCLUDE_OUT_FILE) $(LIB_DIR)/libwlpdstm.a

###############
# create dirs #
###############

OBJ_DIR_TS = $(OBJ_DIR)/.ts
LIB_DIR_TS = $(LIB_DIR)/.ts
INCLUDE_DIR_TS = $(INCLUDE_DIR)/.ts

$(OBJ_DIR_TS):
	mkdir -p $(OBJ_DIR)
	touch $@

$(LIB_DIR_TS):
	mkdir -p $(LIB_DIR)
	touch $@

$(INCLUDE_DIR_TS):
	mkdir -p $(INCLUDE_DIR)
	touch $@

#######################
# create include file #
#######################

$(INCLUDE_OUT_FILE): $(INCLUDE_DIR_TS)
	echo "#ifndef STM_H_" > $(INCLUDE_OUT_FILE)
	echo "#define STM_H_" >> $(INCLUDE_OUT_FILE)
	echo >> $(INCLUDE_OUT_FILE)
	echo | awk '{input = "$(LPDSTM_CPPFLAGS)";split(input, defs, " ");for(idx in defs) if(defs[idx] != "-D") print "#define " defs[idx]}' >> $(INCLUDE_OUT_FILE)
	cat $(INCLUDE_IN_FILE) >> $(INCLUDE_OUT_FILE)
	echo >> $(INCLUDE_OUT_FILE)
	echo "#endif" >> $(INCLUDE_OUT_FILE)

##################
# create library #
##################

# create lib
$(LIB_DIR)/libwlpdstm.a: $(LIB_DIR_TS) $(OBJFILES)
	$(AR) cru $@ $(OBJFILES)

# compile
#$(OBJ_DIR)/wlpdstm.o: $(OBJ_DIR) $(STM_API_DIR)/wlpdstm.cc $(STM_API_DIR)/wlpdstm.h
#	$(CPP) $(CPPFLAGS) $(STM_API_DIR)/wlpdstm.cc -c -o $@

$(OBJ_DIR)/wlpdstm.o: $(OBJ_DIR_TS) $(STM_API_DIR)/wlpdstm.cc $(STM_API_DIR)/wlpdstm.h
	$(CPP) $(CPPFLAGS) $(STM_API_DIR)/wlpdstm.cc -c -o $@

ifeq ($(STM), swisstm)
$(OBJ_DIR)/transaction.o: $(OBJ_DIR_TS) $(STM_SRC_DIR)/transaction.cc $(STM_SRC_DIR)/transaction.h
	$(CPP) $(CPPFLAGS) $(STM_SRC_DIR)/transaction.cc -c -o $@

$(OBJ_DIR)/privatization_tree.o: $(OBJ_DIR_TS) $(STM_SRC_DIR)/privatization_tree.cc $(STM_SRC_DIR)/privatization_tree.h
	$(CPP) $(CPPFLAGS) $(STM_SRC_DIR)/privatization_tree.cc -c -o $@
endif


################
# common tasks #
################

clean:
	rm -rf $(TARGET_DIR)
	rm -rf $(LIB_DIR)
	rm -rf $(INCLUDE_DIR)


