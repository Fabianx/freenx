/***************************************************************************
                               nxsession.h
                             -------------------
    begin                : Wed 26th July 2006
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
 
// Enumerated type defining the stages through which the client goes when connecting
enum { HELLO_NXCLIENT, ACKNOWLEDGE, SHELL_MODE, AUTH_MODE, LOGIN, LIST_SESSIONS, PARSESESSIONS, STARTSESSION, FINISHED };

/*
	0 HELLO NXCLIENT
	1 Acknowledgement from server
	2 SET SHELL_MODE SHELL
	3 SET AUTH_MODE PASSWORD
	4 login
	5 listsession
	6 resumesession/startsession
*/

#define CLIENT_VERSION "1.5.0"

#include <iostream>

using namespace std;

#include "nxsession.h"

NXSession::NXSession()
{
	devurand_fd = -1;
	stage = 0;
	sessionSet = false;
}

NXSession::~NXSession()
{
}

void NXSession::resetSession()
{
	stage = 0;
	sessionSet = false;
}

QString NXSession::parseSSH(QString message)
{
	int response = parseResponse(message);

	QString returnMessage;
	
	if (response == 211) {
		if (doSSH == true) {
			returnMessage = "yes";
			doSSH = false;
		} else
			returnMessage = "no";
	}
	
	switch (stage) {
		case HELLO_NXCLIENT:
			if (message.contains("HELLO NXSERVER - Version")) {
				emit authenticated();
				returnMessage = "hello NXCLIENT - Version ";
				returnMessage.append(CLIENT_VERSION);
				stage++;
			}
			break;
		case ACKNOWLEDGE:
			if (response == 105)
				stage++;
			break;
		case SHELL_MODE:
			if (response == 105) {
				returnMessage = "SET SHELL_MODE SHELL";
				stage++;
			}
			break;
		case AUTH_MODE:
			if (response == 105) {
				returnMessage = "SET AUTH_MODE PASSWORD";
				stage++;
			}
			break;
		case LOGIN:
			switch (response) {
				case 105:
					returnMessage = "login";
					break;
				case 101:
					returnMessage = nxUsername;
					break;
				case 102:
					returnMessage = nxPassword;
					break;
				case 103:
					stage++;
					break;
				case 404:
					emit loginFailed();
				}
			break;
		case LIST_SESSIONS:
			if (response == 105) {
				// Get a list of the available sessions on the server
				returnMessage = "listsession --user=\"" + nxUsername +
				"\" --status=\"suspended,running\" --geometry=\"" + xRes + "x" + yRes + renderSet + "\" --type=\"" + type + "\"";
				stage++;
			}
			break;
		case PARSESESSIONS:
			if (response != 148)
				resumeSessions << message;
			if (response == 148)
				parseResumeSessions(resumeSessions);
			break;
		case STARTSESSION:
			if (response == 105 && sessionSet) {
				int media = 0;
				int render = 0;
				QString fullscreen = "";
				if (sessionData->media)
					media = 1;
				if (sessionData->render)
					render = 1;
				if (sessionData->fullscreen) {
					sessionData->geometry = "fullscreen";
					fullscreen = "+fullscreen";
				}

				if (sessionData->suspended) {
					// These are the session parameters that NoMachine's client sends for resume
					returnMessage = "restoresession --id=\"" + sessionData->id +
					"\" --session=\"" + sessionData->sessionName +
					"\" --type=\"" + sessionData->sessionType +
					"\" --cache=\"" + QString::number(sessionData->cache) +
					"M\" --images=\"" + QString::number(sessionData->images) +
					"M\" --cookie=\"" + generateCookie() +
					"\" --link=\"" + sessionData->linkType +
					"\" --kbtype=\"" + sessionData->kbtype +
					"\" --nodelay=\"1\" --encryption=\"" + QString::number(encryption) +
					"\" --backingstore=\"" + sessionData->backingstore +
					"\" --geometry=\"" + sessionData->geometry +
					"\" --media=\"" + QString::number(media) +
					"\" --agent_server=\"" + sessionData->agentServer +
					"\" --agent_user=\"" + sessionData->agentUser +
					"\" --agent_password=\"" + sessionData->agentPass + "\"";
					stage++;
				} else {
					returnMessage = "startsession --session=\"" + sessionData->sessionName +
					"\" --type=\"" + sessionData->sessionType +
					"\" --cache=\"" + QString::number(sessionData->cache) +
					"M\" --images=\"" + QString::number(sessionData->images) +
					"M\" --cookie=\"" + generateCookie() +
					"\" --link=\"" + sessionData->linkType +
					"\" --render=\"" + QString::number(render) +
					"\" --encryption=\"" + QString::number(encryption) +
					"\" --backingstore=\"" + sessionData->backingstore +
					"\" --imagecompressionmethod=\"" + QString::number(sessionData->imageCompressionMethod) +
					"\" --geometry=\"" + sessionData->geometry + 
					"\" --screeninfo=\"" + xRes + "x" + yRes + "x" + depth + renderSet + fullscreen +
					"\" --keyboard=\"" + sessionData->keyboard +
					"\" --kbtype=\"" + sessionData->kbtype +
					"\" --media=\"" + QString::number(media) +
					"\" --agent_server=\"" + sessionData->agentServer +
					"\" --agent_user=\"" + sessionData->agentUser +
					"\" --agent_password=\"" + sessionData->agentPass + "\"";
					if (sessionData->sessionType == "unix-application")
						returnMessage.append(" --application=\"" + sessionData->customCommand + "\"");
					stage++;
				}
			}
			break;
		case FINISHED:
			emit finished();
	}

	if (!returnMessage.isEmpty()) {
		returnMessage.append("\n");
		return returnMessage;
	} else
		return 0;
}

void NXSession::setSession(NXSessionData *session)
{
	sessionData = session;
	sessionSet = true;
}

int NXSession::parseResponse(QString message)
{
	int idx1, idx2, response;
	// Find out the server response number
	// This will only be present in strings which contain "NX>"
	if (message.contains("NX>")) {
		idx1 = message.indexOf("NX>") + 4;
		idx2 = message.indexOf(" ", idx1);
		response = message.mid(idx1, idx2-idx1).toInt();
	} else {
		response = 0;
	}

	return response;
}

void NXSession::parseResumeSessions(QStringList rawdata)
{
	int at, i;
	QStringList sessions;

	for (i = 0; i < rawdata.size(); ++i) {
		if (rawdata.at(i).contains("-------") && !rawdata.at(i).isEmpty()) {
			at = i;
		}
	}
	
	for (i = at+1; i < rawdata.size(); ++i) {
		if (!rawdata.at(i).contains("NX> 148") && !rawdata.at(i).isEmpty())
			sessions << rawdata.at(i);
	}
	
	QList<QStringList> rawsessions;
	
	for (i = 0; i < sessions.size(); ++i)
		rawsessions.append(sessions.at(i).simplified().split(' '));

	NXResumeData resData;
	
	for (i = 0; i < rawsessions.size(); ++i) {
		resData.display = rawsessions.at(i).at(0).toInt();
		resData.sessionType = rawsessions.at(i).at(1);
		resData.sessionID = rawsessions.at(i).at(2);
		resData.options = rawsessions.at(i).at(3);
		resData.depth = rawsessions.at(i).at(4).toInt();
		resData.screen = rawsessions.at(i).at(5);
		resData.available = rawsessions.at(i).at(6);
		resData.sessionName = rawsessions.at(i).at(7);
		runningSessions.append(resData);
	}

	if (runningSessions.size() != 0) {
		suspendedSessions = true;
		emit sessionsSignal(runningSessions);
	} else {
		emit noSessions();
	}
	
	stage++;
}

void NXSession::wipeSessions()
{
	while (!runningSessions.isEmpty()) {
		runningSessions.removeFirst();
	}
}

QString NXSession::generateCookie()
{
	unsigned long long int int1, int2;
	QString cookie;
	
	devurand_fd = open("/dev/urandom", O_RDONLY);

	fillRand((unsigned char*)&int1, sizeof(int1));
	fillRand((unsigned char*)&int2, sizeof(int2));
	cookie = QString("%1%2").arg(int1, 0, 16).arg(int2, 0, 16);

	return cookie;
}

void NXSession::fillRand(unsigned char *buf, size_t nbytes) {
	ssize_t r;
	unsigned char *where = buf;

	while (nbytes) {
		while ((r = read(devurand_fd, where, nbytes)) == -1)
		where  += r;
		nbytes -= r;
	}
}
