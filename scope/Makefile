

CFLAGS += -Wall -Wstrict-prototypes -O2

LIBS += -lplplotd -lcomedi -lm -lreadline

scope_objs = scope.o common.o

scope: $(scope_objs)
	$(CC) $(LDFLAGS) -o scope $(scope_objs) $(LIBS)

clean:
	-rm -f scope *.o


