TEMPLATE	= app

CONFIG		+= qt warn_on debug

FORMS = settingsdialog.ui logindialog.ui sessionsdialog.ui keydialog.ui logwindow.ui

SOURCES = main.cpp qtnxwindow.cpp qtnxsettings.cpp qtnxsessions.cpp

HEADERS = qtnxwindow.h qtnxsettings.h qtnxsessions.h

INCLUDEPATH	+= $(QTDIR)/include . ..

DEPENDPATH	+= $(QTDIR)/include

LIBPATH += ..

QT += ui xml

LIBS	= -lnxclientlib

TARGET		= qtnx
