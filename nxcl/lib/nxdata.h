/***************************************************************************
                                nxdata.h
                             -------------------
    begin                : Wednesday 9th August 2006
    modifications        : July 2007
    copyright            : (C) 2006 by George Wright
    modifications        : (C) 2007 Embedded Software Foundry Ltd. (U.K.)
                         :     Author: Sebastian James
    email                : seb@esfnet.co.uk, gwright@kde.org
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

#include <string>

/*!
 * Some definitions of numbers that we can send over to the frontend
 * client to tell it how we're getting along with the connection...
 */
#define NXCL_PROCESS_STARTED        1000001
#define NXCL_PROCESS_EXITED         1000002
#define NXCL_AUTH_FAILED            1000003
#define NXCL_AUTHENTICATING         1000004
#define NXCL_LOGIN_FAILED           1000005
#define NXCL_HOST_KEY_VERIFAILED    1000006
#define NXCL_INVOKE_PROXY           1000007
#define NXCL_STARTING               1000008
#define NXCL_FINISHED               1000009
#define NXCL_ALIVE                  1000010
#define NXCL_PROCESS_ERROR          1000011

using namespace std;

namespace nxcl {

	struct NXConfigData {
		string serverHost;
		int serverPort;
		string sessionUser;
		string sessionPass;
		string sessionName;
		string sessionType;
		int cache;
		int images;
		string linkType;
		bool render;
		string backingstore;
		int imageCompressionMethod;
		int imageCompressionLevel;
		string geometry;
		string keyboard;
		string kbtype;
		bool media;
		string agentServer;
		string agentUser;
		string agentPass;
		int cups;
		string key;
		bool encryption;
		bool fullscreen;
		bool virtualDesktop;
		string customCommand;
	};

	struct NXSessionData {
		string sessionName;
		string sessionType;
		int cache;
		int images;
		string linkType;
		bool render;
		string backingstore;
		int imageCompressionMethod;
		int imageCompressionLevel;
		string geometry;
		string keyboard;
		string kbtype;
		bool media;
		string agentServer;
		string agentUser;
		string agentPass;
		int cups;
		string id;
		string key;
		bool encryption;
		bool fullscreen;
		bool virtualDesktop;
		string customCommand;
		bool suspended;
		int xRes;
		int yRes;
		int depth;
		int display;
		bool terminate;
	};

	struct NXResumeData {
		int display;
		string sessionType;
		string sessionID;
		string options;
		int depth;
		string screen;
		string available;
		string sessionName;
	};

} // namespace
#endif
