

CFLAGS += -I .
LIBS += -lcomedi -lm


all: monitor

monitor-objs := monitor.o

monitor: $(monitor-objs)
	$(CC) -o $@ $< $(LIBS)

clean:
	-rm -f *.o monitor
