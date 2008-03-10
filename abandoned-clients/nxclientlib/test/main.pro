TEMPLATE	= app

CONFIG		= qt warn_on debug dll

SOURCES = main.cpp

INCLUDEPATH	+= $(QTDIR)/include . ..

DEPENDPATH	+= $(QTDIR)/include

LIBPATH += ..

QT		= core

LIBS	= -lnxclientlib

TARGET		= test
