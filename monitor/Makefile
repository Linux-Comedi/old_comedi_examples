

CFLAGS += -I . -Wall
LIBS += -lcomedi -lm -lncurses


all: monitor

monitor-objs := monitor.o nc.o sv.o sci_print.o

monitor: $(monitor-objs)
	$(CC) -o $@ $(monitor-objs) $(LIBS)

clean:
	-rm -f *.o monitor
