

CFLAGS += -O2 -Wall -Wstrict-prototypes -g
CFLAGS += -I.

LDFLAGS += 


simultaneous_start: simultaneous_start.o common.o
	$(CC) $(LDFLAGS) -o $@ simultaneous_start.o common.o \
		-lcomedi

clean:
	rm -f simultaneous_start *.o

