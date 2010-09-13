CFLAGS=-Wall -Werror --std=c99 -I. -g

all: cdatecalc.o cdctest
	@echo Done.

cdctest: cdatecalc.o cdctest.o
	$(CC) -o $@ $(CFLAGS) $^ 

%.o:%.c
	$(CC) -o $@ $(CFLAGS) -c $<

clean:
	rm -rf cdctest *.o


# End file.
