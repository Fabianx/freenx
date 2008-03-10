TEMPLATE	= lib

CONFIG		= qt warn_on debug dll

HEADERS		= nxclientlib.h nxsession.h nxparsexml.h nxdata.h nxwritexml.h

SOURCES		= nxclientlib.cpp nxsession.cpp nxparsexml.cpp nxwritexml.cpp

INCLUDEPATH	+= $(QTDIR)/include .

DEPENDPATH	+= $(QTDIR)/include

QT		= core xml

TARGET		= nxclientlib
