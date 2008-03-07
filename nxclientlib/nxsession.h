/***************************************************************************
                                 nxsession.h
                             -------------------
    begin                : Sat 22nd July 2006
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

#ifndef _NXSESSION_H_
#define _NXSESSION_H_

#include <QObject>
#include <QString>
#include <QStringList>
#include <QList>

#include <fcntl.h>
#include <unistd.h>

#include "nxdata.h"

// This class is used to parse the output from the nxssh session to the server

class NXSession : public QObject
{
	Q_OBJECT
	public:
		NXSession();
		~NXSession();
		
		QString parseSSH(QString);
		int parseResponse(QString);
		void setUsername(QString user) { nxUsername = user; };
		void setPassword(QString pass) { nxPassword = pass; };
		void parseResumeSessions(QStringList);

		void setResolution(int x, int y) { xRes.setNum(x); yRes.setNum(y); };
		void setDepth(int d) { depth.setNum(d); };
		void setRender(bool isRender) { if (isRender) renderSet = "+render"; };
		void setContinue(bool allow) { doSSH = allow; };
		void setSession(NXSessionData *);
		void setEncryption(bool enc) { encryption = enc; };
		void resetSession();

		void wipeSessions();
		
		QString generateCookie();
	signals:
		// Emitted when the initial public key authentication is successful
		void authenticated();
		void loginFailed();
		void finished();
		void sessionsSignal(QList<NXResumeData>);
		void noSessions();
	private:
		bool doSSH;
		bool suspendedSessions;
		bool sessionSet;
		bool encryption;
		
		void reset();
		void fillRand(unsigned char *, size_t);

		int stage;

		int devurand_fd;

		QString nxUsername;
		QString nxPassword;

		QString xRes;
		QString yRes;
		QString depth;
		QString renderSet;

		QString type;

		QStringList resumeSessions;

		QList<NXResumeData> runningSessions;
		NXSessionData *sessionData;

};

#endif
