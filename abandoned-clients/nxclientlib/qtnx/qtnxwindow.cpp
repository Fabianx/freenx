/***************************************************************************
                               qtnxwindow.cpp
                             -------------------
    begin                : Thursday August 3rd 2006
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

#include "qtnxwindow.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QX11Info>

QtNXWindow::QtNXWindow() : QMainWindow()
{
	sessionsDialog = 0;

	logWindow = new QDialog(0);
	ui_lw.setupUi(logWindow);

	log = new QTextDocument();
	ui_lw.output->setDocument(log);
	
	loginDialog = new QWidget(this);
	menuBar = new QMenuBar(this);
	statusBar = new QStatusBar(this);

	fileMenu = new QMenu(tr("&File"));
	connectionMenu = new QMenu(tr("Conn&ection"));
	
	ui_lg.setupUi(loginDialog);
	setCentralWidget(loginDialog);
	setStatusBar(statusBar);
	setMenuBar(menuBar);

	statusBar->showMessage(tr("Ready"));
	statusBar->setSizeGripEnabled(false);
	
	menuBar->addMenu(fileMenu);
	menuBar->addMenu(connectionMenu);

	fileMenu->addAction(tr("Quit"), qApp, SLOT(quit()), QKeySequence(tr("CTRL+Q")));

	connectionMenu->addAction(tr("Show log window"), this, SLOT(showLogWindow()), QKeySequence(tr("CTRL+L")));
	
	connectionMenu->addAction(tr("Connect..."), this, SLOT(startConnect()));

	QDir dir(QDir::homePath()+"/.qtnx","*.nxml");
	for (unsigned int i=0;i<dir.count();i++) {
		QString conn=dir[i];
		ui_lg.session->addItem(conn.left(conn.length()-5));
	}
	ui_lg.session->addItem(tr("Create new session"));


	connect(ui_lg.connectButton, SIGNAL(pressed()), this, SLOT(startConnect()));
	connect(ui_lg.configureButton, SIGNAL(pressed()), this, SLOT(configure()));
}

QtNXWindow::~QtNXWindow()
{
}

void QtNXWindow::showLogWindow()
{
	if (logWindow->isHidden())
		logWindow->show();
	else
		logWindow->hide();
}

void QtNXWindow::failedLogin()
{
	QMessageBox::critical(this, tr("Authentication failure"), tr("You have supplied an incorrect username or password for this NX server."), QMessageBox::Ok, QMessageBox::NoButton, QMessageBox::NoButton);
	statusBar->showMessage(tr("Login failed"));
}
void QtNXWindow::sshContinue(QString message)
{
	int reply = QMessageBox::question(this, tr("SSH Request"), message, QMessageBox::Yes, QMessageBox::No, QMessageBox::NoButton);
	if (reply == QMessageBox::Yes)
		nxClient.allowSSHConnect(true);
	else
		nxClient.allowSSHConnect(false);

}

void QtNXWindow::startConnect()
{
	QByteArray key;
	QDesktopWidget dw;
	QX11Info info;

	NXParseXML handler;
	handler.setData(&config);

	QFile file(QDir::homePath() + "/.qtnx/" + ui_lg.session->currentText() + ".nxml");
	QXmlInputSource inputSource(&file);

	QXmlSimpleReader reader;
	reader.setContentHandler(&handler);
	reader.setErrorHandler(&handler);
	reader.parse(inputSource);

	session.sessionName = config.sessionName;
	session.sessionType = config.sessionType;
	session.cache = config.cache;
	session.images = config.images;
	session.linkType = config.linkType;
	session.render = config.render;
	session.backingstore = "when_requested";
	session.imageCompressionMethod = config.imageCompressionMethod;
	session.imageCompressionLevel = config.imageCompressionLevel;
	session.geometry = config.geometry;
	session.keyboard = "defkeymap";
	session.kbtype = "pc102/defkeymap";
	session.media = config.media;
	session.agentServer = config.agentServer;
	session.agentUser = config.agentUser;
	session.agentPass = config.agentPass;
	session.cups = config.cups;
	session.fullscreen = config.fullscreen;

	if (!config.key.isEmpty()) {
		key = config.key.toAscii();
		session.key = "supplied";
	} else
		session.key = "default";

	if (config.sessionType == "unix-application")
		session.customCommand = config.customCommand;

	if (config.encryption == false) {
		if (session.key == "supplied")
			nxClient.invokeNXSSH(session.key, config.serverHost, false, key, config.serverPort);
		else if (session.key == "default")
			nxClient.invokeNXSSH(session.key, config.serverHost, false, 0, config.serverPort);
	} else {
		if (session.key == "supplied")
			nxClient.invokeNXSSH(session.key, config.serverHost, true, key, config.serverPort);
		else if (session.key == "default")
			nxClient.invokeNXSSH(session.key, config.serverHost, true, 0, config.serverPort);
	}
	
	nxClient.setUsername(ui_lg.username->text());
	nxClient.setPassword(ui_lg.password->text());
	nxClient.setResolution(dw.screenGeometry(this).width(), dw.screenGeometry(this).height());
	nxClient.setDepth(info.depth());
	
	connect(&nxClient, SIGNAL(resumeSessions(QList<NXResumeData>)), this, SLOT(loadResumeDialog(QList<NXResumeData>)));
	connect(&nxClient, SIGNAL(noSessions()), this, SLOT(noSessions()));
	connect(&nxClient, SIGNAL(sshRequestConfirmation(QString)), this, SLOT(sshContinue(QString)));
	connect(&nxClient, SIGNAL(callbackWrite(QString)), this, SLOT(updateStatusBar(QString)));
	connect(&nxClient, SIGNAL(loginFailed()), this, SLOT(failedLogin()));
	connect(&nxClient, SIGNAL(stdout(QString)), this, SLOT(logStd(QString)));
	connect(&nxClient, SIGNAL(stderr(QString)), this, SLOT(logStd(QString)));
	connect(&nxClient, SIGNAL(stdin(QString)), this, SLOT(logStd(QString)));
	
	//nxClient.setSession(&session);
}

void QtNXWindow::updateStatusBar(QString message)
{
	statusBar->showMessage(message);
}

void QtNXWindow::configure()
{
	if (ui_lg.session->currentText() == tr("Create new session"))
		settingsDialog = new QtNXSettings("");
	else
		settingsDialog = new QtNXSettings(ui_lg.session->currentText());

	connect(settingsDialog, SIGNAL(closing()), this, SLOT(configureClosed()));

	settingsDialog->show();
}

void QtNXWindow::configureClosed()
{
	while (ui_lg.session->count() != 0) {
		ui_lg.session->removeItem(0);
	}

	QDir dir(QDir::homePath()+"/.qtnx","*.nxml");
	for (unsigned int i=0;i<dir.count();i++) {
		QString conn=dir[i];
		ui_lg.session->addItem(conn.left(conn.length()-5));
	}
	ui_lg.session->addItem(tr("Create new session"));
}

void QtNXWindow::loadResumeDialog(QList<NXResumeData> data)
{
	delete sessionsDialog;
	sessionsDialog = new QtNXSessions(data);
	sessionsDialog->show();

	connect(sessionsDialog, SIGNAL(newPressed()), this, SLOT(resumeNewPressed()));
	connect(sessionsDialog, SIGNAL(resumePressed(QString)), this, SLOT(resumeResumePressed(QString)));
}

void QtNXWindow::resumeNewPressed()
{
	nxClient.setSession(&session);
}

void QtNXWindow::resumeResumePressed(QString id)
{
	session.id = id;
	session.suspended = true;
	nxClient.setSession(&session);
}

void QtNXWindow::noSessions()
{
	session.suspended = false;
	nxClient.setSession(&session);
}

void QtNXWindow::logStd(QString message)
{
	log->setPlainText(log->toPlainText() + message);
}

