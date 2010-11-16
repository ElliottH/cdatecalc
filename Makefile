
# Set DESTDIR if you want to build out of tree.
DESTDIR ?= .

CDC_C_SRCS := c/cdc.c
CDC_CPP_SRCS := cpp/cdcpp.cpp

C_HDRS=include/cdc/cdc.h
CPP_HDRS=include/cdc/cdcpp.h

OBJ_DIR=$(DESTDIR)/obj
LIB_DIR=$(DESTDIR)/lib
BIN_DIR=$(DESTDIR)/bin
INC=include

C_OBJ_DIR=$(OBJ_DIR)/c
CPP_OBJ_DIR=$(OBJ_DIR)/cpp

C_OBJS := $(CDC_C_SRCS:c/%.c=$(C_OBJ_DIR)/%.o)
CPP_OBJS := $(CDC_C_SRCS:c/%.c=$(CPP_OBJ_DIR)/%.o) $(CDC_CPP_SRCS:cpp/%.cpp=$(CPP_OBJ_DIR)/%.o)

CFLAGS=-Wall -Werror --std=c99 -I$(INC) -g -fPIC -DPIC
CXXFLAGS=-Wall -Werror -I$(INC) -g -fPIC -DPIC

LDFLAGS=-L$(LIB_DIR) 

all: dirs $(BIN_DIR)/cdctest $(LIB_DIR)/libcdc.so $(LIB_DIR)/libcdcpp.so $(BIN_DIR)/cdcpptest

$(BIN_DIR)/cdctest: $(LIB_DIR)/libcdc.so $(C_OBJ_DIR)/cdctest.o
	$(CC) -o $@ $(CFLAGS) $(C_OBJ_DIR)/cdctest.o $(LDFLAGS) -lcdc

$(BIN_DIR)/cdcpptest: $(LIB_DIR)/libcdc.so $(CPP_OBJ_DIR)/cdcpptest.o
	$(CXX) -o $@ $(CFLAGS) $(CPP_OBJ_DIR)/cdcpptest.o $(LDFLAGS) -lcdcpp 


# Add -DCOMPILE_AS_MAIN to make cdctest compile as the main
#  program. Otherwise it is a handy object file so you can
#  incorporate cdc's tests into your own unit test code.
$(OBJ_DIR)/c/cdctest.o: test/cdctest.c
	$(CC) -o $@ -DCOMPILE_AS_MAIN=1 $(CFLAGS) -c $<

$(OBJ_DIR)/cpp/cdcpptest.o: test/cdcpptest.cpp
	$(CXX) -o $@ -DCOMPILE_AS_MAIN=1 $(CXXFLAGS) -c $<

$(C_OBJ_DIR)/%.o: c/%.c
	$(CC) -o $@ $(CFLAGS) -c $<

$(CPP_OBJ_DIR)/%.o: cpp/%.cpp
	$(CXX) -o $@ $(CXXFLAGS) -c $<

$(CPP_OBJ_DIR)/%.o: c/%.c
	$(CXX) -o $@ $(CXXFLAGS) -c $<

$(LIB_DIR)/libcdc.so: $(C_OBJS)
	$(CC) -shared -o $@ $(LDFLAGS) $(C_OBJS)

$(LIB_DIR)/libcdcpp.so: $(CPP_OBJS)
	$(CC) -shared -o $@ $(LDFLAGS) $(CPP_OBJS)


$(CDC_C_SRCS) test/cdctest.c: $(C_HDRS)
$(CDC_CPP_SRCS) test/cdcpptest.cpp: $(CPP_HDRS) $(C_HDRS)

clean:
	rm -rf $(BIN_DIR) $(LIB_DIR) $(OBJ_DIR)

.PHONY: dirs 
dirs: 
	-mkdir -p $(LIB_DIR)
	-mkdir -p $(C_OBJ_DIR) $(CPP_OBJ_DIR)
	-mkdir -p $(BIN_DIR)

# End file.
