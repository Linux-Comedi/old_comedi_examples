


CFLAGS=-Wall -O2 -I .
LDFLAGS=-L../lib/ -lcomedi -lncurses
#CC=egcs

CFLAGS += -DPCL_DEV=\"/dev/comedi0\" -D_GNU_SOURCE
#CFLAGS += -DJCD_DEV=\"/dev/comedi0\"


BINS=bake sample
OBJS_BAKE=tc.o nc.o pcld789.o bake_fe.o
OBJS_NEWBAKE=bake.o tc.o pcld789.o
OBJS_SAMPLE=sample.o tc.o nc.o pcld789.o bigfont.o default8x16.o default8x9.o

all: $(BINS)

bake: $(OBJS_BAKE)
	$(CC) -o bake $(OBJS_BAKE) $(LDFLAGS) -lm

newbake: $(OBJS_NEWBAKE)
	$(CC) -o newbake $(OBJS_NEWBAKE) $(LDFLAGS) -lm

sample: $(OBJS_SAMPLE)
	$(CC) -o sample $(OBJS_SAMPLE) $(LDFLAGS) -lm

install:
	cp bake $(HOME)/bin
	cp sample $(HOME)/bin

clean:
	-rm *.o $(BINS)

