
AUTOMAKE_OPTIONS = check-news dist-bzip2

ALWAYS_SUBDIRS = lib test

SUBDIRS = $(ALWAYS_SUBDIRS)

if WITH_NXCMD
  SUBDIRS += nxcl
endif

if WITH_DOXYGEN
  SUBDIRS += doc
endif

CLEANFILES = *~

MAINTAINERCLEANFILES = aclocal.m4 nxcl-*.tar.gz nxcl-*.tar.bz2 nxcl-*.diff

INCLUDES = -I./lib

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = nxcl.pc

EXTRA_DIST = nxcl.pc.in

