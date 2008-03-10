/***************************************************************************
                               nxclientlib.cpp
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

#include "nxclientlib.h"

#include <QDir>

#define NXSSH_BIN "nxssh"
#define NXPROXY_BIN "nxproxy"

// Default NoMachine certificate for FALLBACK
QByteArray cert("-----BEGIN DSA PRIVATE KEY-----\nMIIBuwIBAAKBgQCXv9AzQXjxvXWC1qu3CdEqskX9YomTfyG865gb4D02ZwWuRU/9\nC3I9/bEWLdaWgJYXIcFJsMCIkmWjjeSZyTmeoypI1iLifTHUxn3b7WNWi8AzKcVF\naBsBGiljsop9NiD1mEpA0G+nHHrhvTXz7pUvYrsrXcdMyM6rxqn77nbbnwIVALCi\nxFdHZADw5KAVZI7r6QatEkqLAoGBAI4L1TQGFkq5xQ/nIIciW8setAAIyrcWdK/z\n5/ZPeELdq70KDJxoLf81NL/8uIc4PoNyTRJjtT3R4f8Az1TsZWeh2+ReCEJxDWgG\nfbk2YhRqoQTtXPFsI4qvzBWct42WonWqyyb1bPBHk+JmXFscJu5yFQ+JUVNsENpY\n+Gkz3HqTAoGANlgcCuA4wrC+3Cic9CFkqiwO/Rn1vk8dvGuEQqFJ6f6LVfPfRTfa\nQU7TGVLk2CzY4dasrwxJ1f6FsT8DHTNGnxELPKRuLstGrFY/PR7KeafeFZDf+fJ3\nmbX5nxrld3wi5titTnX+8s4IKv29HJguPvOK/SI7cjzA+SqNfD7qEo8CFDIm1xRf\n8xAPsSKs6yZ6j1FNklfu\n-----END DSA PRIVATE KEY-----");

NXClientLib::NXClientLib(QObject *parent) : QObject(parent)
{
	isFinished = false;
	proxyData.encrypted = false;
	password = false;
	
	connect(&session, SIGNAL(authenticated()), this, SLOT(doneAuth()));
	connect(&session, SIGNAL(loginFailed()), this, SLOT(failedLogin()));
	connect(&session, SIGNAL(finished()), this, SLOT(finished()));
	connect(&session, SIGNAL(sessionsSignal(QList<NXResumeData>)), this, SLOT(suspendedSessions(QList<NXResumeData>)));
	connect(&session, SIGNAL(noSessions()), this, SLOT(noSuspendedSessions()));
	
	connect(&nxproxyProcess, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(reset()));
}

NXClientLib::~NXClientLib()
{
//	nxsshProcess.terminate();
}

void NXClientLib::invokeNXSSH(QString publicKey, QString serverHost, bool encryption, QByteArray key, int port)
{
	QStringList arguments;
	proxyData.server = serverHost;
	
	if (publicKey == "default") {
		usingHardcodedKey = true;
	}

	if (publicKey == "default" || publicKey == "supplied") {
		if (publicKey == "default")
			cerr << tr("WARNING: Using hardcoded NoMachine public key.").toStdString() << endl;
		
		keyFile = new QTemporaryFile;
		keyFile->open();
		
		arguments << "-nx" << "-i" << keyFile->fileName();
		
		if (publicKey == "default")
			keyFile->write(cert);
		else
			keyFile->write(key);
			
		keyFile->close();
	} else {
		arguments << "-i" << publicKey;
	}
	
	if (encryption == true) {
		arguments << "-B";
		session.setEncryption(true);
	} else
		session.setEncryption(false);
	
	connect(&nxsshProcess, SIGNAL(started()), this, SLOT(processStarted()));
	connect(&nxsshProcess, SIGNAL(error(QProcess::ProcessError)), this, SLOT(processError(QProcess::ProcessError)));
	connect(&nxsshProcess, SIGNAL(readyReadStandardOutput()), this, SLOT(processParseStdout()));
	connect(&nxsshProcess, SIGNAL(readyReadStandardError()), this, SLOT(processParseStderr()));

	nxsshProcess.setEnvironment(nxsshProcess.systemEnvironment());

	arguments << QString("-p %1").arg(port) << QString("nx@" + serverHost);
	nxsshProcess.start(NXSSH_BIN, arguments);
}

void NXClientLib::processStarted()
{
	emit callbackWrite(tr("Started nxssh process"));
}

void NXClientLib::processError(QProcess::ProcessError error)
{
	QString message;
	switch (error) {
		case QProcess::FailedToStart:
			message = tr("The process failed to start");
			break;
		case QProcess::Crashed:
			message = tr("The process has crashed");
			break;
		case QProcess::Timedout:
			message = tr("The process timed out");
			break;
		case QProcess::WriteError:
			message = tr("There was an error writing to the process");
			break;
		case QProcess::ReadError:
			message = tr("There was an error reading from the process");
			break;
		case QProcess::UnknownError:
			message = tr("There was an unknown error with the process");
			break;
	}
	
	emit callbackWrite(message);
}

void NXClientLib::reset()
{
	nxsshProcess.terminate();
	
	isFinished = false;
	proxyData.encrypted = false;
	password = false;
	
	session.resetSession();
}

void NXClientLib::failedLogin()
{
	emit loginFailed();
	nxsshProcess.terminate();
}

void NXClientLib::processParseStdout()
{
	QString message = nxsshProcess.readAllStandardOutput().data();
	
	// Message 211 is sent if ssh is asking to continue with an unknown host
	if (session.parseResponse(message) == 211) {
		emit sshRequestConfirmation(message);
	}
	
	cout << message.toStdString();

	emit stdout(message);
	
	QStringList messages = splitString(message);
	QStringList::const_iterator i;

	// On some connections this is sent via stdout instead of stderr?
	if (proxyData.encrypted && isFinished && message.contains("NX> 999 Bye")) {
		QString returnMessage;
		returnMessage = "NX> 299 Switching connection to: ";
		returnMessage += proxyData.proxyIP + ":" + QString::number(proxyData.port) + " cookie: " + proxyData.cookie + "\n";
		write(returnMessage);
	} else if (message.contains("NX> 287 Redirected I/O to channel descriptors"))
		emit callbackWrite(tr("The session has been started successfully"));

	for (i = messages.constBegin(); i != messages.constEnd(); ++i) {
		if ((*i).contains("Password")) {
			emit callbackWrite(tr("Authenticating with NX server"));
			password = true;
		}
		if (!isFinished)
			write(session.parseSSH(*i));
		else
			write(parseSSH(*i));
	}
}

QStringList NXClientLib::splitString(QString message)
{
	QStringList lines;
	
	// Split the string message into several different strings separated by '\n'
	for (int i = 0;;i++) {
		if (message.section('\n', i, i).isEmpty() && message.section('\n', i+1, i+1).isEmpty() && message.section('\n', i+2, i+2).isEmpty()) {
			break;
		} else
			lines << message.section('\n', i, i);
	}
	
	return lines;
}

void NXClientLib::processParseStderr()
{
	QString message = nxsshProcess.readAllStandardError().data();
	
	cerr << message.toStdString();

	if (proxyData.encrypted && isFinished && message.contains("NX> 999 Bye")) {
		QString returnMessage;
		returnMessage = "NX> 299 Switching connection to: ";
		returnMessage += proxyData.proxyIP + ":" + QString::number(proxyData.port) + " cookie: " + proxyData.cookie + "\n";
		write(returnMessage);
	} else if (message.contains("NX> 287 Redirected I/O to channel descriptors"))
		emit callbackWrite(tr("The session has been started successfully"));

	emit stderr(message);
}

void NXClientLib::write(QString data)
{
	nxsshProcess.write(data.toAscii());
	
	if (password) {
		data = "********";
		password = false;
	}

	emit stdin(data);

	cout << data.toStdString();
}

void NXClientLib::doneAuth()
{
	if (usingHardcodedKey)
		delete keyFile;
}

void NXClientLib::allowSSHConnect(bool auth)
{
	session.setContinue(auth);
}

void NXClientLib::setSession(NXSessionData *nxSession)
{
	session.setSession(nxSession);
	write(session.parseSSH("NX> 105"));
}

QString NXClientLib::parseSSH(QString message)
{
	QString returnMessage = 0;
	
	if (message.contains("NX> 700 Session id: ")) {
		proxyData.id = message.right(message.length() - 20);
	} else if (message.contains("NX> 705 Session display: ")) {
		proxyData.display = message.right(message.length() - 24).toInt();
		proxyData.port = proxyData.display + 4000;
	} else if (message.contains("NX> 706 Agent cookie: ")) {
		proxyData.cookie = message.right(message.length() - 22);
	} else if (message.contains("NX> 702 Proxy IP: ")) {
		proxyData.proxyIP = message.right(message.length() - 18);
	} else if (message.contains("NX> 707 SSL tunneling: 1")) {
		proxyData.encrypted = true;
	}

	if (message.contains("NX> 710 Session status: running")) {
		invokeProxy();
		session.wipeSessions();
		returnMessage = "bye\n";
	}
	
	return returnMessage;
}

void NXClientLib::invokeProxy()
{
	emit callbackWrite(tr("Starting NX session"));
	
	QFile options;
	QDir nxdir;
	
	nxdir.mkpath(QDir::homePath() + "/.nx/S-" + proxyData.id);
	options.setFileName(QDir::homePath() + "/.nx/S-" + proxyData.id + "/options");

	QString data;
	if (proxyData.encrypted)
		data = "nx,session=session,cookie=" + proxyData.cookie + ",root=" + QDir::homePath() + "/.nx,id=" + proxyData.id + ",listen=" + QString::number(proxyData.port) + ":" + QString::number(proxyData.display) + "\n";
	else
		data = "nx,session=session,cookie=" + proxyData.cookie + ",root=" + QDir::homePath() + "/.nx,id=" + proxyData.id + ",connect=" + proxyData.server + ":" + QString::number(proxyData.display) + "\n";
	
	options.open(QIODevice::WriteOnly);
	options.write(data.toAscii());
	options.close();

	QStringList arguments;
	nxproxyProcess.setEnvironment(nxproxyProcess.systemEnvironment());

	arguments << "-S" << "options=" + options.fileName() + ":" + QString::number(proxyData.display);
	
	nxproxyProcess.start(NXPROXY_BIN, arguments);

	if (!nxproxyProcess.waitForStarted())
		emit callbackWrite(tr("Session failed to start"));
}

void NXClientLib::noSuspendedSessions()
{
	emit noSessions();
}
