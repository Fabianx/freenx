.PHONY: all clean distclean

ifneq ($(NX_VERSION),)
CFLAGS+=-DNXSERVER_COMMAND="\"$(PATH_BIN)/nxserver\""
CFLAGS+=-DNXNODE_COMMAND="\"$(PATH_BIN)/nxnode\""
CFLAGS+=-DNXBIN_DIRECTORY="\"$(PATH_BIN)\""
endif

all: nxserver-suid

clean distclean:
	rm -f *.o nxserver-suid
