
COMEDIDIR=/home/ds/cvs/comedi
LINUXDIR=/home/ds/linux/linux-2.4.0-rthal3
RTAIDIR=/home/ds/rtai/rtai4-mf

CFLAGS := -O2 -Wall -Wstrict-prototypes \
	-D__KERNEL__ -DMODULE \
	-I$(COMEDIDIR)/include -I$(LINUXDIR)/include \
	-I$(RTAIDIR)/include

all:	it.o ledclock.o

it.o:	it.c

ledclock.o: ledclock.c

clean:
	-rm *.o
