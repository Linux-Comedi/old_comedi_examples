
LINUXDIR=/home/ds/linux/linux-2.4.16-rthal5
RTAIDIR=/home/ds/rtai/rtai4

CFLAGS := -O2 -Wall -Wstrict-prototypes \
	-D__KERNEL__ -DMODULE \
	-I. \
	-I$(LINUXDIR)/include \
	-I$(RTAIDIR)/include

all:	it.o ledclock.o

it.o:	it.c

ledclock.o: ledclock.c

clean:
	-rm *.o
