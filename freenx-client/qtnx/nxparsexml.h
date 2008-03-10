/***************************************************************************
                                nxparsexml.h
                            -------------------
        begin                : Friday August 4th 2006
        copyright            : (C) 2006 by George Wright
                             : (C) 2007 Defuturo Ltd
        email                : gwright@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#ifndef _NXPARSEXML_H_
#define _NXPARSEXML_H_

#include <QXmlDefaultHandler>

#include "nxdata.h"
#include "nxsession.h"

using namespace nxcl;

class NXParseXML : public QObject, public QXmlDefaultHandler
{
    Q_OBJECT
    public:
        NXParseXML();
        ~NXParseXML();
        bool startElement(const QString &namespaceURI, const QString
                &localName, const QString &qName, const QXmlAttributes 
                &attributes);
        bool endElement(const QString &namespaceURI, const QString
                &localName, const QString &qName);
        void setData(NXConfigData *data) { sessionData = data; };
    private:
        NXConfigData *sessionData;
        int group;
};

#endif
