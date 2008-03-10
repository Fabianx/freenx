AM_CPPFLAGS = @PACKAGE_CFLAGS@ -DPACKAGE_DATA_DIR=\""$(datadir)"\" -DLOCALEDIR=\"$(localedir)\" -DPACKAGE_BIN_DIR=\""$(bindir)"\" $(DBUS_CFLAGS)
INCLUDES = -I../lib
bin_PROGRAMS = libtest notQttest

if WITH_NXCMD
  bin_PROGRAMS += nxcmd
endif

if DEBUG
  AM_CPPFLAGS += -DDEBUG
endif

libtest_SOURCES = libtest.cpp
libtest_LDADD = @PACKAGE_LIBS@ $(LIBINTL) -L../lib -lnxcl
notQttest_SOURCES = notQttest.cpp
notQttest_LDADD = @PACKAGE_LIBS@ $(LIBINTL) -L../lib -lnxcl
nxcmd_SOURCES = nxcmd.cpp
nxcmd_LDADD = @PACKAGE_LIBS@ $(LIBINTL) $(DBUS_LIBS)
#pkginclude_HEADERS = header.h

