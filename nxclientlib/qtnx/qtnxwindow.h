/***************************************************************************
                               qtnxwindow.h
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

#ifndef _QTNXWINDOW_H_
#define _QTNXWINDOW_H_

#include <QMainWindow>
#include <QMenuBar>
#include <QStatusBar>

#include "nxclientlib.h"
#include "nxdata.h"
#include "nxparsexml.h"

#include "qtnxsessions.h"
#include "qtnxsettings.h"

#include "ui_logindialog.h"
#include "ui_logwindow.h"

class QtNXWindow : public QMainWindow
{
	Q_OBJECT
	public:
		QtNXWindow();
		~QtNXWindow();
	public slots:
		void startConnect();
		void configure();
		void configureClosed();
		void loadResumeDialog(QList<NXResumeData>);
		void resumeNewPressed();
		void resumeResumePressed(QString);
		void noSessions();
		void sshContinue(QString);
		void updateStatusBar(QString);
		void failedLogin();
		void showLogWindow();
		void logStd(QString);
	private:
		Ui::LoginDialog ui_lg;
		Ui::LogWindow ui_lw;
		
		NXSessionData session;
		NXConfigData config;
		
		NXClientLib nxClient;
		
		QtNXSettings *settingsDialog;
		QtNXSessions *sessionsDialog;
		
		QDialog *logWindow;
		QMenu *fileMenu;
		QMenu *connectionMenu;
		QMenuBar *menuBar;
		QStatusBar *statusBar;
		QTextDocument *log;
		QWidget *loginDialog;
};

#endif
