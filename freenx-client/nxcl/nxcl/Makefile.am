AM_CPPFLAGS = @PACKAGE_CFLAGS@ -DPACKAGE_DATA_DIR=\""$(datadir)"\" -DLOCALEDIR=\"$(localedir)\" -DPACKAGE_BIN_DIR=\""$(bindir)"\" $(DBUS_CFLAGS)

if DEBUG
    AM_CPPFLAGS += -DDEBUG
endif

INCLUDES = -I../lib
bin_PROGRAMS = nxcl
nxcl_SOURCES = main.cpp nxcl.cpp
# This links to X11 so that nxcl can obtain the X server's actual screen size
nxcl_LDADD = @PACKAGE_LIBS@ $(LIBINTL) $(DBUS_LIBS) -L../lib -lnxcl -lX11
pkginclude_HEADERS = nxcl.h

