/***************************************************************************
                                qtnxwindow.h
                            -------------------
        begin                : Thursday August 3rd 2006
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

#ifndef _QTNXWINDOW_H_
#define _QTNXWINDOW_H_

#include <QMainWindow>
#include <QMenuBar>
#include <QStatusBar>
#include <QTimer>

#include "nxclientlib.h"
#include "nxdata.h"
#include "nxparsexml.h"

#include "qtnxsessions.h"
#include "qtnxsettings.h"

#include "ui_logindialog.h"
#include "ui_logwindow.h"

using namespace std;
using namespace nxcl;

class QtNXCallback :
    public QObject, 
    public NXClientLibExternalCallbacks
{
    Q_OBJECT
    public:
        QtNXCallback() {};
        ~QtNXCallback() {};

        // Library callbacks
        void write(string msg)
          { emit status(QString::fromStdString(msg)); }

        void write(int num, string msg)
          { emit progress(num, QString::fromStdString(msg)); }

        void error(string msg)
          { emit error(QString::fromStdString(msg)); }

        void debug(string msg)
          { emit debug(QString::fromStdString(msg)); }

        void stdoutSignal(string msg)
          { emit logging("stdout> " + QString::fromStdString(msg)); }

        void stderrSignal(string msg)
          { emit logging("stderr> " + QString::fromStdString(msg)); }

        void stdinSignal(string msg)
          { emit logging("stdin>  " + QString::fromStdString(msg)); }

        void resumeSessionsSignal(list<NXResumeData> sessions)
          { emit suspendedSessions(QList<NXResumeData>::
                  fromStdList(sessions)); }

        void noSessionsSignal()
          { emit noSessions(); }

        void serverCapacitySignal()
          { emit atCapacity(); }

        void connectedSuccessfullySignal()
          { emit connectedSuccessfully(); }

    signals:
        void logging(QString);
        void status(QString);
        void progress(int, QString);
        void debug(QString);
        void error(QString);
        void connectedSuccessfully();

        void suspendedSessions(QList<NXResumeData>);
        void noSessions();
        void atCapacity();
};

class QtNXWindow : public QMainWindow
{
    Q_OBJECT
    public:
        QtNXWindow();
        ~QtNXWindow();

    public slots:
        void startConnect();
        void configure();
        void configureClosed(QString);
        void updateLinkType(QString);

        // Callback handlers
        void handleSuspendedSessions(QList<NXResumeData>);
        void handleNoSessions();
        void handleLogging(QString);
        void handleTimeout();
        void handleProgress(int, QString);
        void handleStatus(QString);
        void handleAtCapacity();

        void resumeNewPressed();
        void resumeResumePressed(QString);
        void sshContinue(QString);
        void failedLogin();
        void showLogWindow();
        void quit();
    private:

        // Decided to split up the code
        void setupUI();
        void setDefaultData();
        void initialiseClient();
        void parseXML();
        void reinitialiseClient();

        int getWidth();
        int getHeight();
        int getDepth();

        Ui::LoginDialog ui_lg;
        Ui::LogWindow ui_lw;

        NXSessionData session;
        NXConfigData config;

        NXClientLib *m_NXClient;

        QtNXSettings *settingsDialog;
        QtNXSessions *sessionsDialog;

        QAction *closeWindowAction;
        QDialog *logWindow;
        QMenu *fileMenu;
        QMenu *connectionMenu;
        QMenuBar *menuBar;
        QStatusBar *statusBar;
        QTextDocument *log;
        QTimer *processProbe;
        QWidget *loginDialog;

        QtNXCallback callback;

        QString binaryPath;
};
#endif
