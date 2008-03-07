CLEANFILES = *~
AM_CFLAGS = @PACKAGE_CFLAGS@ -DPACKAGE_DATA_DIR=\""$(datadir)"\" -DLOCALEDIR=\"$(localedir)\"
lib_LTLIBRARIES = libcallbacks_nx.la
libcallbacks_nx_la_SOURCES = callbacks_nx.c
libcallbacks_nx_la_LIBADD = @PACKAGE_LIBS@ $(LIBINTL)
libcallbacks_nx_la_LDFLAGS = -version-info 1:0:0
pkginclude_HEADERS = callbacks_nx.h
