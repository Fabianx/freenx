all: libnxredir.so.0

CC=gcc
CFLAGS=-g -O2 -Wall -fPIC
LIBNAME=libnxredir.so.0

libnxredir.so.0: nxredir.o
	$(CC) -fPIC $(CFLAGS) -nostdlib -shared -o $(LIBNAME) nxredir.o -ldl -lc

clean:
	rm -f $(LIBNAME)
	rm -f *.o

install: all
	# install and possibly patch paths gotten by nxloadconfig
	if [ -n "$$NX_VERSION" ]; \
	then \
		install -m755 $(LIBNAME) $(DESTDIR)/$$PATH_LIB/ ;\
		install -m755 nxredir $(DESTDIR)/$$PATH_BIN/ ;\
		install -m755 nxsmb $(DESTDIR)/$$CUPS_BACKEND/ ;\
		perl -pi -e "s,PATH_LIB=.*,PATH_LIB=\"$$PATH_LIB\",g" $(DESTDIR)/$$PATH_BIN/nxredir ;\
		perl -pi -e "s,PATH_LIB=.*,PATH_LIB=\"$$PATH_LIB\",g" $(DESTDIR)/$$CUPS_BACKEND/nxsmb ;\
		perl -pi -e "s,CUPS_BACKEND=.*,CUPS_BACKEND=\"$$CUPS_BACKEND\",g" $(DESTDIR)/$$CUPS_BACKEND/nxsmb ;\
	else \
		install -m755 libnxredir.so.0 $(DESTDIR)/usr/lib ;\
		install -m755 nxredir $(DESTDIR)/usr/bin ;\
		install -m755 nxsmb $(DESTDIR)/usr/lib/cups/backend/ ;\
	fi

.PHONY: clean install
