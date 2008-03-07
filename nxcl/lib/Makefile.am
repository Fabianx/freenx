CLEANFILES = *~
AM_CPPFLAGS = @PACKAGE_CFLAGS@  -DPACKAGE_BIN_DIR=\""$(bindir)"\"

if DEBUG
    AM_CPPFLAGS += -DDEBUG
endif

INCLUDES = -I$(top_srcdir) -I$(top_builddir)
lib_LTLIBRARIES = libnxcl.la
libnxcl_la_SOURCES = notQt.cpp nxsession.cpp nxclientlib.cpp
libnxcl_la_LDFLAGS = -version-info 1:0:0
pkginclude_HEADERS = notQt.h nxsession.h nxclientlib.h nxdata.h nxclientlib_i18n.h
