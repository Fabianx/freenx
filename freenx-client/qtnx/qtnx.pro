TEMPLATE	= app

CONFIG		+= static qt warn_on release

FORMS = settingsdialog.ui logindialog.ui sessionsdialog.ui keydialog.ui logwindow.ui

SOURCES = main.cpp qtnxwindow.cpp qtnxsettings.cpp qtnxsessions.cpp nxparsexml.cpp nxwritexml.cpp

HEADERS = qtnxwindow.h qtnxsettings.h qtnxsessions.h nxparsexml.h nxwritexml.h

INCLUDEPATH	+= $(QTDIR)/include

!macx {
    QMAKE_CXXFLAGS += $$system(pkg-config --cflags nxcl)

    LIBS += $$system(pkg-config --libs nxcl)
}

macx {
    QMAKE_CXXFLAGS += -I/usr/local/include/nxcl
    LIBS += -lnxcl
}

DEPENDPATH	+= $(QTDIR)/include


QT += gui xml

TARGET		= qtnx
target.path = $$[QT_INSTALL_BINS]
INSTALLS += target
