all: remserial2

REMOBJ=remserial2.o stty.o
remserial2: $(REMOBJ)
	$(CC) $(LDFLAGS) -o remserial2 $(REMOBJ)
clean:
	rm -f remserial2 *.o

#HOW TO BUILD A 32 BIT VERSION ON A AMD64 64bit Linux?
# 1st: export CC="gcc -m32 -static"
# 2nd: make
