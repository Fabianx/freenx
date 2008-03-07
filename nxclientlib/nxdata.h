/***************************************************************************
                                nxdata.h
                             -------------------
    begin                : Wednesday 9th August 2006
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

#ifndef _NXDATA_H_
#define _NXDATA_H_

#include <QString>

struct NXConfigData {
	QString serverHost;
	int serverPort;
	QString sessionName;
	QString sessionType;
	int cache;
	int images;
	QString linkType;
	bool render;
	QString backingstore;
	int imageCompressionMethod;
	int imageCompressionLevel;
	QString geometry;
	QString keyboard;
	QString kbtype;
	bool media;
	QString agentServer;
	QString agentUser;
	QString agentPass;
	int cups;
	QString key;
	bool encryption;
	bool fullscreen;
	QString customCommand;
};

struct NXSessionData {
	QString sessionName;
	QString sessionType;
	int cache;
	int images;
	QString linkType;
	bool render;
	QString backingstore;
	int imageCompressionMethod;
	int imageCompressionLevel;
	QString geometry;
	QString keyboard;
	QString kbtype;
	bool media;
	QString agentServer;
	QString agentUser;
	QString agentPass;
	int cups;
	QString id;
	QString key;
	bool suspended;
	bool fullscreen;
	QString customCommand;
};

struct NXResumeData {
	int display;
	QString sessionType;
	QString sessionID;
	QString options;
	int depth;
	QString screen;
	QString available;
	QString sessionName;
};

#endif
