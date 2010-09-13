CFLAGS=-Wall -Werror --std=c99 -I.

all: timecalc.o tctest
	@echo Done.

tctest: timecalc.o tctest.o
	$(CC) -o $@ $(CFLAGS) $^ 

%.o:%.c
	$(CC) -o $@ $(CFLAGS) -c $<

clean:
	rm -rf tctest *.o


# End file.
