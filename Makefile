
# Set DESTDIR if you want to build out of tree.
DESTDIR ?= .

# Set SRCDIR if for some inexplicable reason your source is somewhere else.
SRCDIR ?= .


CDC_C_SRCS := c/cdc.c
CDC_CPP_SRCS := cpp/cdcpp.cpp

C_HDRS=include/cdc/cdc.h
CPP_HDRS=include/cdc/cdcpp.h

C_OBJS := $(CDC_C_SRCS:c/%.c=obj/c/%.o)
CPP_OBJS := $(CDC_C_SRCS:c/%.c=obj/cpp/%.o) $(CDC_CPP_SRCS:cpp/%.cpp=obj/cpp/%.o)

OBJ=$(DESTDIR)/obj
LIB=$(DESTDIR)/lib
BIN=$(DESTDIR)/bin
INC=include

C_OBJ=$(OBJ)/c
CPP_OBJ=$(OBJ)/cpp

CFLAGS=-Wall -Werror --std=c99 -I$(INC) -g -fPIC -DPIC
CXXFLAGS=-Wall -Werror -I$(INC) -g -fPIC -DPIC

LDFLAGS=-L$(LIB) 

all: dirs bin/cdctest $(LIB)/libcdc.so $(LIB)/libcdcpp.so bin/cdcpptest

bin/cdctest: $(LIB)/libcdc.so $(OBJ)/c/cdctest.o
	$(CC) -o $@ $(CFLAGS) $(OBJ)/c/cdctest.o $(LDFLAGS) -lcdc

bin/cdcpptest: $(LIB)/libcdc.so $(OBJ)/cpp/cdcpptest.o
	$(CXX) -o $@ $(CFLAGS) $(OBJ)/cpp/cdcpptest.o $(LDFLAGS) -lcdcpp 


# Add -DCOMPILE_AS_MAIN to make cdctest compile as the main
#  program. Otherwise it is a handy object file so you can
#  incorporate cdc's tests into your own unit test code.
$(OBJ)/c/cdctest.o: test/cdctest.c
	$(CC) -o $@ -DCOMPILE_AS_MAIN=1 $(CFLAGS) -c $<

$(OBJ)/cpp/cdcpptest.o: test/cdcpptest.cpp
	$(CXX) -o $@ -DCOMPILE_AS_MAIN=1 $(CXXFLAGS) -c $<

$(C_OBJ)/%.o: c/%.c
	$(CC) -o $@ $(CFLAGS) -c $<

$(CPP_OBJ)/%.o: cpp/%.cpp
	$(CXX) -o $@ $(CXXFLAGS) -c $<

$(CPP_OBJ)/%.o: c/%.c
	$(CXX) -o $@ $(CXXFLAGS) -c $<

$(LIB)/libcdc.so: $(C_OBJS)
	$(CC) -shared -o $@ $(LDFLAGS) $(C_OBJS)

$(LIB)/libcdcpp.so: $(CPP_OBJS)
	$(CC) -shared -o $@ $(LDFLAGS) $(CPP_OBJS)


$(CDC_C_SRCS) test/cdctest.c: $(C_HDRS)
$(CDC_CPP_SRCS) test/cdcpptest.cpp: $(CPP_HDRS) $(C_HDRS)

clean:
	rm -rf bin lib obj 

.PHONY: dirs 

dirs: 
	-mkdir -p $(LIB)
	-mkdir -p $(OBJ)/c $(OBJ)/cpp
	-mkdir -p $(BIN)



# End file.
