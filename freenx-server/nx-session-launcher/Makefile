.PHONY: all install

CC=gcc
CFLAGS=-g -O2 -Wall -fPIC

SOURCES = nx-session-launcher-suid.c
PROGRAMS = nx-session-launcher-suid

ifneq ($(NX_VERSION),)
CFLAGS+=-DNXSERVER_COMMAND="\"$(PATH_BIN)/nx-session-launcher\""
endif

all: $(PROGRAMS)

clean:
	rm -f $(PROGRAMS)

