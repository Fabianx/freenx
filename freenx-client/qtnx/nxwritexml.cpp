/***************************************************************************
                                nxwritexml.cpp
                            -------------------
        begin                : Wednesday August 9th 2006
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

#include "nxwritexml.h"

NXWriteXML::NXWriteXML()
{
}

NXWriteXML::~NXWriteXML()
{
}

void NXWriteXML::write(QString fileName)
{
    QFile file(fileName);
    file.open(QFile::WriteOnly);

    QTextStream xml(&file);
    xml.setCodec("UTF-8");

    xml << "<!DOCTYPE NXClientLibSettings>\n";
    xml << "<NXClientLibSettings>\n";

    xml << "<option key=\"Connection Name\" value=\"" <<
        escape(QString::fromStdString(sessionData.sessionName)) <<
        "\"></option>\n";

    xml << "<option key=\"Server Hostname\" value=\"" <<
        escape(QString::fromStdString(sessionData.serverHost)) <<
        "\"></option>\n";

    xml << "<option key=\"Server Port\" value=\"" <<
        escape(QString::number(sessionData.serverPort)) <<
        "\"></option>\n";

    xml << "<option key=\"Session Type\" value=\"" <<
        escape(QString::fromStdString(sessionData.sessionType)) <<
        "\"></option>\n";

    xml << "<option key=\"Custom Session Command\" value=\"" <<
        escape(QString::fromStdString(sessionData.customCommand)) <<
        "\"></option>\n";

    xml << "<option key=\"Disk Cache\" value=\"" <<
        escape(QString::number(sessionData.cache)) <<
        "\"></option>\n";

    xml << "<option key=\"Image Cache\" value=\"" <<
        escape(QString::number(sessionData.images)) <<
        "\"></option>\n";

    xml << "<option key=\"Link Type\" value=\"" <<
        escape(QString::fromStdString(sessionData.linkType)) <<
        "\"></option>\n";

    if (sessionData.render == true)
        xml << "<option key=\"Use Render Extension\" value=\"True\">" <<
            "</option>\n";
    else
        xml << "<option key=\"Use Render Extension\" value=\"False\">" <<
            "</option>\n";

    if (sessionData.imageCompressionMethod == -1)
        xml << "<option key=\"Image Compression Method\" value=\"JPEG\">" <<
            "</option>\n";
    else if (sessionData.imageCompressionMethod == 2)
        xml << "<option key=\"Image Compression Method\" value=\"PNG\">" <<
            "</option>\n";
    else if (sessionData.imageCompressionMethod == 0)
        xml << "<option key=\"Image Compression Method\" value=\"Raw X11\">" <<
            "</option>\n";

    xml << "<option key=\"JPEG Compression Level\" value=\"" <<
        escape(QString::number(sessionData.imageCompressionLevel)) <<
        "\"></option>\n";

    xml << "<option key=\"Desktop Geometry\" value=\"" <<
        escape(QString::fromStdString(sessionData.geometry)) <<
        "\"></option>\n";

    xml << "<option key=\"Keyboard Layout\" value=\"" <<
        escape(QString::fromStdString(sessionData.keyboard)) <<
        "\"></option>\n";

    xml << "<option key=\"Keyboard Type\" value=\"" <<
        escape(QString::fromStdString(sessionData.kbtype)) <<
        "\"></option>\n";

    if (sessionData.media == true)
        xml << "<option key=\"Media\" value=\"True\"></option>\n";
    else
        xml << "<option key=\"Media\" value=\"False\"></option>\n";

    xml << "<option key=\"Agent Server\" value=\"" <<
        escape(QString::fromStdString(sessionData.agentServer)) <<
        "\"></option>\n";

    xml << "<option key=\"Agent User\" value=\"" <<
        escape(QString::fromStdString(sessionData.agentUser)) <<
        "\"></option>\n";

    xml << "<option key=\"CUPS Port\" value=\"" <<
        escape(QString::number(sessionData.cups)) <<
        "\"></option>\n";

    xml << "<option key=\"Authentication Key\" value=\"" <<
        escape(QString::fromStdString(sessionData.key)) <<
        "\"></option>\n";

    if (sessionData.encryption == true)
        xml << "<option key=\"Use SSL Tunnelling\" value=\"True\">" <<
            "</option>\n";
    else
        xml << "<option key=\"Use SSL Tunnelling\" value=\"False\">" <<
            "</option>\n";

    if (sessionData.fullscreen == true)
        xml << "<option key=\"Enable Fullscreen Desktop\" value=\"True\">" <<
            "</option>\n";
    else
        xml << "<option key=\"Enable Fullscreen Desktop\" value=\"False\">" <<
            "</option>\n";

    xml << "</NXClientLibSettings>\n";

    file.close();
}

QString NXWriteXML::escape(QString plain)
{
    QString formatted;
    formatted = plain.replace('<', "&lt;");
    formatted = plain.replace('>', "&rt;");
    formatted = plain.replace('&', "&amp;");
    return formatted;
}
