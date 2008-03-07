/***************************************************************************
                               nxwritexml.h
                             -------------------
    begin                : Wednesday August 9th 2006
    copyright            : (C) 2006 by George Wright
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

#ifndef _NXWRITEXML_H_
#define _NXWRITEXML_H_

#include "nxdata.h"

#include <QString>
#include <QFile>
#include <QTextStream>

class NXWriteXML
{
	public:
		NXWriteXML();
		~NXWriteXML();
		void write(QString);
		void setSessionData(NXConfigData data) { sessionData = data; };
		QString escape(QString);
	private:
		NXConfigData sessionData;
};

#endif
