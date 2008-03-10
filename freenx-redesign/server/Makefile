# Copyright 2007 Google Inc.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
#
# Author: alriddoch@google.com (Alistair Riddoch)

.PHONY: all install dist

SUDO_PROMPT = Your password for sudo access:

PACKAGE = freenx
VERSION = 0.0.0
CC = gcc
CFLAGS = -Wall -g

INSTALL = install
TAR = tar
PREFIX = /usr/freenx
BINDIR = $(PREFIX)/bin
LIBDIR = $(PREFIX)/lib
PARSERLIBDIR = $(LIBDIR)/nxparser
ETCDIR = $(PREFIX)/etc
VARDIR = $(PREFIX)/var
SESSDBDIR = $(VARDIR)/sessdb
TMPNXDIR = /tmp/nx
COMPATABILITY = compatibility/nxagent \
                compatibility/nxnode \
                compatibility/nxserver \
                compatibility/nxserver-login \
                compatibility/nxserver-login-helper \
                compatibility/nxserver-login-hook \
                compatibility/nxserver-login-userhook \
                compatibility/nxservice \
                compatibility/nxshadow \
                compatibility/nxstart
UTILITY = utility/nxloadconfig \
          utility/nxloadconfig-helper.sh \
          utility/nxlog \
          utility/log4sh \
          utility/nxagent-helper
SOURCES = utility/nxserver-suid.c
PROGRAMS = utility/nxserver-suid
BINARIES = $(COMPATABILITY) $(UTILITY) $(PROGRAMS)
LIBRARIES = lib/nxloadconfig.py \
            lib/nxlog.py \
            lib/nxsession.py
PARSER_LIB = lib/nxparser/__init__.py \
             lib/nxparser/base.py \
             lib/nxparser/node.py \
             lib/nxparser/server.py

all: $(PROGRAMS)

ARCHIVE=$(PWD)/$(PACKAGE)-$(VERSION).tar.gz
DISTDIR=$${TMPDIR-/tmp}/$(PACKAGE)-$(VERSION)
dist:
	$(RM) -r $(DISTDIR)
	$(INSTALL) -d $(DISTDIR)
	$(INSTALL) -m 644 Makefile $(DISTDIR)
	$(INSTALL) -d $(DISTDIR)/utility
	$(INSTALL) -m 755 $(UTILITY) $(DISTDIR)/utility
	$(INSTALL) -d $(DISTDIR)/compatability
	$(INSTALL) -m 755 $(COMPATABILITY) $(DISTDIR)/compatability
	$(INSTALL) -m 644 $(SOURCES) $(DISTDIR)/utility
	$(INSTALL) -d $(DISTDIR)/lib
	$(INSTALL) -m 644 $(LIBRARIES) $(DISTDIR)/lib
	$(INSTALL) -d $(DISTDIR)/lib/nxparser
	$(INSTALL) -m 644 $(PARSER_LIB) $(DISTDIR)/lib/nxparser
	(cd $${TMPDIR-/tmp} && $(TAR) chozf $(ARCHIVE) $(PACKAGE)-$(VERSION))
	$(RM) -r $(DISTDIR)

install: utility/nxserver-suid
	@sudo -p "$(SUDO_PROMPT)" true
	sudo $(INSTALL) -o $(LOGNAME) -d -m 755 $(BINDIR)
	sudo $(INSTALL) -o nx -d -m 755 $(VARDIR)
	sudo $(INSTALL) -o nx -d -m 700 $(SESSDBDIR)
	$(INSTALL) -m 755 $(BINARIES) $(BINDIR)
	$(INSTALL) -d -m 755 $(LIBDIR)
	$(INSTALL) -m 644 $(LIBRARIES) $(LIBDIR)
	$(INSTALL) -d -m 755 $(PARSERLIBDIR)
	$(INSTALL) -m 644 $(PARSER_LIB) $(PARSERLIBDIR)
	$(INSTALL) -d -m 755 $(ETCDIR)
	echo -e "PATH_BIN=\"$(BINDIR)\"\nPATH=\"$$PATH:$(BINDIR)\"\nLOG_LEVEL=DEBUG" > $(ETCDIR)/general.conf
	echo -e "LOG_LEVEL=NOTICE" > $(ETCDIR)/nxstart.conf
	echo -e "COMMAND_START_GNOME='/usr/bin/dbus-launch --exit-with-session gnome-session'\n\
	COMMAND_START_KDE='/usr/bin/dbus-launch --exit-with-session startkde'" > $(ETCDIR)/nxagent-helper.conf
	chmod 644 $(ETCDIR)/general.conf
	sudo chown nx $(BINDIR)/nxserver-suid
	sudo chmod 4755 $(BINDIR)/nxserver-suid
	[ ! -e $(TMPNXDIR] && sudo mkdir $(TMPNXDIR)
	sudo chown root:root $(TMPNXDIR)
	sudo chmod 1777 $(TMPNXDIR)
