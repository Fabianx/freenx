/***************************************************************************
                              nxparsexml.cpp
                            -------------------
        begin                : Friday August 4th 2006
        copyright            : (C) 2006 by George Wright
                               (C) 2007 Defuturo Ltd
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

#include <iostream>
#include "nxparsexml.h"

using namespace std;
using namespace nxcl;

NXParseXML::NXParseXML()
{
}

NXParseXML::~NXParseXML()
{
}

bool NXParseXML::startElement(const QString &namespaceURI, const QString &localName, const QString &qName, const QXmlAttributes &attributes)
{
    if (attributes.value("key") == "Connection Name") {
        sessionData->sessionName = attributes.value("value").toStdString();
    }

    if (attributes.value("key") == "Server Hostname") {
        sessionData->serverHost = attributes.value("value").toStdString();
    }

    if (attributes.value("key") == "Server Port") {
        sessionData->serverPort = attributes.value("value").toInt();
    }

    if (attributes.value("key") == "Session Type") {
        sessionData->sessionType = attributes.value("value").toStdString();
    }

    if (attributes.value("key") == "Custom Session Command") {
        sessionData->customCommand = attributes.value("value").toStdString();
    }

    if (attributes.value("key") == "Disk Cache") {
        sessionData->cache = attributes.value("value").toInt();
    }

    if (attributes.value("key") == "Image Cache") {
        sessionData->images = attributes.value("value").toInt();
    }

    if (attributes.value("key") == "Link Type") {
        sessionData->linkType = attributes.value("value").toStdString();
    }

    if (attributes.value("key") == "Use Render Extension") {
        if (attributes.value("value") == "True")
            sessionData->render = true;
        else
            sessionData->render = false;
    }

    if (attributes.value("key") == "Image Compression Method") {
        if (attributes.value("value") == "JPEG")
            sessionData->imageCompressionMethod = -1;
        else if (attributes.value("value") == "PNG")
            sessionData->imageCompressionMethod = 2;
        else if (attributes.value("value") == "Raw X11")
            sessionData->imageCompressionMethod = 0;
    }

    if (attributes.value("key") == "JPEG Compression Level") {
        sessionData->imageCompressionLevel = attributes.value("value").toInt();
    }

    if (attributes.value("key") == "Desktop Geometry") {
        sessionData->geometry = attributes.value("value").toStdString();
    }

    if (attributes.value("key") == "Keyboard Layout") {
        sessionData->keyboard = attributes.value("value").toStdString();
    }

    if (attributes.value("key") == "Keyboard Type") {
        sessionData->kbtype = attributes.value("value").toStdString();
    }

    if (attributes.value("key") == "Media") {
        if (attributes.value("value") == "True")
            sessionData->media = true;
        else
            sessionData->media = false;
    }

    if (attributes.value("key") == "Agent Server") {
        sessionData->agentServer = attributes.value("value").toStdString();
    }

    if (attributes.value("key") == "Agent User") {
        sessionData->agentUser = attributes.value("value").toStdString();
    }

    if (attributes.value("key") == "CUPS Port") {
        sessionData->cups = attributes.value("value").toInt();
    }

    if (attributes.value("key") == "Authentication Key") {
        sessionData->key = attributes.value("value").toStdString();
    }

    if (attributes.value("key") == "Use SSL Tunnelling") {
        if (attributes.value("value") == "True")
            sessionData->encryption = true;
        else
            sessionData->encryption = false;
    }

    if (attributes.value("key") == "Enable Fullscreen Desktop") {
        if (attributes.value("value") == "True")
            sessionData->fullscreen = true;
        else
            sessionData->fullscreen = false;
    }

    return true;
}

bool NXParseXML::endElement(const QString &namespaceURI, const QString &localName, const QString &qName)
{
    return true;
}
