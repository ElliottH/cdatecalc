CFLAGS=-Wall -Werror --std=c99 -I. -g

all: cdc.o cdctest
	@echo Done.

cdctest: cdc.o cdctest.o
	$(CC) -o $@ $(CFLAGS) $^ 

%.o:%.c
	$(CC) -o $@ $(CFLAGS) -c $<

cdc.o: cdc.h
cdctest.o: cdc.h

clean:
	rm -rf cdctest *.o


# End file.
