TEMPLATE	= app

CONFIG		+= qt warn_on debug

SOURCES = main.cpp mainwindow.cpp settingsdialog.cpp nxhandler.cpp

HEADERS = mainwindow.h settingsdialog.h nxhandler.h

INCLUDEPATH	+= $(QTDIR)/include . ..

DEPENDPATH	+= $(QTDIR)/include

LIBPATH += ..

QMAKE_CXXFLAGS_DEBUG += $$system(pkg-config --cflags gtk+-2.0 hildon-libs)

QT = core xml

LIBS	= -lnxclientlib $$system(pkg-config --libs gtk+-2.0 hildon-libs)

TARGET		= maemonx
