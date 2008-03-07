# Currently, if prefix is /usr then these files go in /usr/share/ but it would be better to place them in /usr/share/nxlaunch/
EXTRA_DIST = nxlaunch.gladep nxconfig.gladep kde-nx-session.png gnome-nx-session.png unknown-nx-session.png rdp-nx-session.png vnc-nx-session.png nx-icon.png
dist_data_DATA = nxlaunch.glade nxconfig.glade
AM_CFLAGS = @PACKAGE_CFLAGS@ -DPACKAGE_DATA_DIR=\""$(datadir)"\" -DLOCALEDIR=\"$(localedir)\" -DPACKAGE_BIN_DIR=\""$(bindir)"\"
bin_PROGRAMS = nxlaunch
nxlaunch_SOURCES =  main.c nxlaunch.c
nxlaunch_LDADD = @PACKAGE_LIBS@ $(LIBINTL) -L../lib/.libs -lcallbacks_nx
pkginclude_HEADERS = nxlaunch.h
nxlaunch_LDFLAGS = -export-dynamic # Required so that glade can autoconnect signals