SOURCES=nxssh-4US.c
TARGET=nxssh

all: nxssh nxssh.exe

nxssh:
	$(CC) $(CFLAGS) $(SOURCES) -o $(TARGET)

nxssh.exe:
	i586-mingw32msvc-gcc $(CFLAGS) $(SOURCES) -o $(TARGET).exe

clean:
	rm -f $(TARGET) $(TARGET).exe
