CFLAGS=-Wall -Werror --std=c99 -I. -g

all: cdc.o cdctest
	@echo Done.

cdctest: cdc.o cdctest.o
	$(CC) -o $@ $(CFLAGS) $^ 

# Add -DCOMPILE_AS_MAIN to make cdctest compile as the main
#  program. Otherwise it is a handy object file so you can
#  incorporate cdc's tests into your own unit test code.
cdctest.o: cdctest.c
	$(CC) -o $@ -DCOMPILE_AS_MAIN=1 $(CFLAGS) -c $<

%.o:%.c
	$(CC) -o $@ $(CFLAGS) -c $<

cdc.o: cdc.h
cdctest.o: cdc.h

clean:
	rm -rf cdctest *.o


# End file.
