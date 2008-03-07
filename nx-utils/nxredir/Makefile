all: libnxredir.so.0

CC=gcc
CFLAGS=-g -O2 -Wall

libnxredir.so.0: nxredir.o
	$(CC) -fPIC $(CFLAGS) -nostdlib -shared -o libnxredir.so.0 nxredir.o -ldl -lc

clean:
	rm -f libnxredir.so.0
	rm -f *.o

install: all
	cp -a libnxredir.so.0 $(DESTDIR)/usr/lib
	cp -a nxredir $(DESTDIR)/usr/bin
	ldconfig

.PHONY: clean install
