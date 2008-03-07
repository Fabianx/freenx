/***************************************************************************
                                nxhandler.cpp
                             -------------------
    begin                : Saturday 17th February 2007
    copyright            : (C) 2007 by George Wright
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

/* The main function which creates all the GUI components */

#include "nxhandler.h"

NXHandler::NXHandler()
{
}

NXHandler::~NXHandler()
{
}

void NXHandler::startConnect() {}
//X {
//X     QDesktopWidget dw;
//X     QX11Info info;
//X 
//X     NXParseXML handler;
//X     handler.setData(&config);
//X 
//X     QFile file(QDir::homePath() + "/.qtnx/" + ui_lg.session->currentText() + ".nxml");
//X     QXmlInputSource inputSource(&file);
//X 
//X     QXmlSimpleReader reader;
//X     reader.setContentHandler(&handler);
//X     reader.setErrorHandler(&handler);
//X     reader.parse(inputSource);
//X 
//X     session.sessionName = config.sessionName;
//X     session.sessionType = config.sessionType;
//X     session.cache = config.cache;
//X     session.images = config.images;
//X     session.linkType = config.linkType;
//X     session.render = config.render;
//X     session.backingstore = "when_requested";
//X     session.imageCompressionMethod = config.imageCompressionMethod;
//X     session.imageCompressionLevel = config.imageCompressionLevel;
//X     session.geometry = config.geometry;
//X     session.keyboard = "defkeymap";
//X     session.kbtype = "pc102/defkeymap";
//X     session.media = config.media;
//X     session.agentServer = config.agentServer;
//X     session.agentUser = config.agentUser;
//X     session.agentPass = config.agentPass;
//X     session.cups = config.cups;
//X     session.fullscreen = config.fullscreen;
//X 
//X     if (!config.key.isEmpty()) {
//X         key = config.key.toAscii();
//X         session.key = "supplied";
//X     } else
//X         session.key = "default";
//X 
//X     if (config.sessionType == "unix-application")
//X         session.customCommand = config.customCommand;
//X 
//X     if (config.encryption == false) {
//X         if (session.key == "supplied")
//X             nxClient.invokeNXSSH(session.key, config.serverHost, false, key, config.serverPort);
//X         else if (session.key == "default")
//X             nxClient.invokeNXSSH(session.key, config.serverHost, false, 0, config.serverPort);
//X     } else {
//X         if (session.key == "supplied")
//X             nxClient.invokeNXSSH(session.key, config.serverHost, true, key, config.serverPort);
//X         else if (session.key == "default")
//X             nxClient.invokeNXSSH(session.key, config.serverHost, true, 0, config.serverPort);
//X     }
//X 
//X     nxClient.setUsername(ui_lg.username->text());
//X     nxClient.setPassword(ui_lg.password->text());
//X     nxClient.setResolution(dw.screenGeometry(this).width(), dw.screenGeometry(this).height());
//X     nxClient.setDepth(info.depth());
//X 
//X     connect(&nxClient, SIGNAL(resumeSessions(QList<NXResumeData>)), this, SLOT(loadResumeDialog(QList<NXResumeData>)));
//X     connect(&nxClient, SIGNAL(noSessions()), this, SLOT(noSessions()));
//X     connect(&nxClient, SIGNAL(sshRequestConfirmation(QString)), this, SLOT(sshContinue(QString)));
//X     connect(&nxClient, SIGNAL(callbackWrite(QString)), this, SLOT(updateStatusBar(QString)));
//X     connect(&nxClient, SIGNAL(loginFailed()), this, SLOT(failedLogin()));
//X     connect(&nxClient, SIGNAL(stdout(QString)), this, SLOT(logStd(QString)));
//X     connect(&nxClient, SIGNAL(stderr(QString)), this, SLOT(logStd(QString)));
//X     connect(&nxClient, SIGNAL(stdin(QString)), this, SLOT(logStd(QString)));
//X 
//X     //nxClient.setSession(&session);
//X }
