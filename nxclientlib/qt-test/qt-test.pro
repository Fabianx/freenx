TEMPLATE	= app

CONFIG		+= qt warn_on debug

FORMS = mainwindow.ui

SOURCES = main.cpp qtnxwindow.cpp

HEADERS = qtnxwindow.h

INCLUDEPATH	+= $(QTDIR)/include . ..

DEPENDPATH	+= $(QTDIR)/include

LIBPATH += ..

QT += ui

LIBS	= -lnxclientlib

TARGET		= qtnx
