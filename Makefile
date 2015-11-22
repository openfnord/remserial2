all: remserial2

REMOBJ=remserial2.o stty.o
remserial2: $(REMOBJ)
	$(CC) $(LDFLAGS) -o remserial2 $(REMOBJ)

clean:
	rm -f remserial2 *.o
