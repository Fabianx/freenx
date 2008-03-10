AM_CFLAGS = -g -O2 -Wall
INCLUDES = ${GTK_CFLAGS}

bin_PROGRAMS = thinnx

thinnx_SOURCES = thinnx.c
thinnx_LDADD = ${GTK_LIBS}

# same as the used by thinstation ones, should be changed to
# ${datadir} sometime soon?
icondir = ${sysconfdir}/icons
icon_DATA = thinnx.xpm error.xpm

confdir = ${sysconfdir}/thinnx
conf_DATA = thinnx.conf.example
