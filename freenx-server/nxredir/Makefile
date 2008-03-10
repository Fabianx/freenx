all: libnxredir.so.0

CC=gcc
CFLAGS=-g -O2 -Wall -fPIC

libnxredir.so.0: nxredir.o
	$(CC) -fPIC $(CFLAGS) -nostdlib -shared -o libnxredir.so.0 nxredir.o -ldl -lc

clean:
	rm -f libnxredir.so.0
	rm -f *.o

install: all
	install -m755 libnxredir.so.0 $(DESTDIR)/usr/lib
	install -m755 nxredir $(DESTDIR)/usr/bin
	install -m755 nxsmb $(DESTDIR)/usr/lib/cups/backend/
	ldconfig

.PHONY: clean install
