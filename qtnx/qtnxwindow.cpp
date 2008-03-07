/***************************************************************************
                               qtnxwindow.cpp
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

#include "qtnxwindow.h"

#include <QApplication>
#include <QDesktopWidget>
#include <QDir>
#include <QFile>
#include <QMessageBox>

#ifdef Q_WS_MAC
    #include <CoreFoundation/CFBundle.h>
#endif

using namespace nxcl;
using namespace std;

QtNXWindow::QtNXWindow() :
    QMainWindow(),
    m_NXClient(new NXClientLib()),
    sessionsDialog(NULL),
    processProbe(new QTimer())
{
#ifdef Q_CYGWIN_WIN
    qApp->setStyle("plastique");
#endif
    setWindowTitle("QtNX");

    initialiseClient();

    setupUI();

    QDir dir(QDir::homePath() + "/.qtnx", "*.nxml");

    for (unsigned int i = 0; i < dir.count(); i++) {
        QString conn=dir[i];
        ui_lg.session->addItem(conn.left(conn.length() - 5));
    }

    ui_lg.session->addItem(tr("Create new session"));

    parseXML();

    connect(ui_lg.connectButton, SIGNAL(pressed()), this, SLOT(startConnect()));
    connect(ui_lg.password, SIGNAL(returnPressed()), this, SLOT(startConnect()));

    connect(ui_lg.configureButton, SIGNAL(pressed()), this, SLOT(configure()));

    connect(ui_lg.session, SIGNAL(currentIndexChanged(QString)), this, SLOT(updateLinkType(QString)));

    connect(processProbe, SIGNAL(timeout()), this, SLOT(handleTimeout()));

    connect(&callback, SIGNAL(logging(QString)), this, SLOT(handleLogging(QString)));

    connect(&callback, SIGNAL(status(QString)), this,
            SLOT(handleStatus(QString)));

    connect(&callback, SIGNAL(suspendedSessions(QList<NXResumeData>)), this,
            SLOT(handleSuspendedSessions(QList<NXResumeData>)));

    connect(&callback, SIGNAL(noSessions()), this, SLOT(handleNoSessions()));

    connect(&callback, SIGNAL(progress(int, QString)), this,
            SLOT(handleProgress(int, QString)));

    connect(&callback, SIGNAL(atCapacity()), this, SLOT(handleAtCapacity()));

    connect(&callback, SIGNAL(connectedSuccessfully()), this, SLOT(quit()));
}

void QtNXWindow::updateLinkType(QString sessionName)
{
    parseXML();
}

QtNXWindow::~QtNXWindow()
{
    delete m_NXClient;
}

void QtNXWindow::quit()
{
    if (closeWindowAction->isChecked())
        qApp->quit();
}
void QtNXWindow::initialiseClient()
{
    m_NXClient->setExternalCallbacks(&callback);

#ifdef Q_WS_MAC
    // We ship nxproxy and nxssh in qtnx.app/Contents/MacOS
    CFURLRef appUrlRef = CFBundleCopyBundleURL(CFBundleGetMainBundle());
    CFStringRef macPath = CFURLCopyFileSystemPath(appUrlRef,
            kCFURLPOSIXPathStyle);

    const char *pathPtr = CFStringGetCStringPtr(macPath,
            CFStringGetSystemEncoding());

    CFRelease(appUrlRef);
    CFRelease(macPath);

    binaryPath = pathPtr;
    binaryPath.append("/Contents/MacOS");

    m_NXClient->setCustomPath(binaryPath.toStdString());

    setenv("NX_SYSTEM", binaryPath.toStdString().c_str(), 1);
#endif

#ifdef Q_CYGWIN_WIN
    QString filePath = qApp->applicationDirPath();

    // On Windows QtNX will ship with the NX system components in NX/ relative to the
    // QtNX binary
    filePath.append("/NX");
    setenv("NX_SYSTEM", filePath.toStdString().c_str(), 1);
    filePath.append("/bin");
    m_NXClient->setCustomPath(filePath.toStdString());
#endif
}

void QtNXWindow::setupUI()
{
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

    fileMenu->addAction(tr("Quit"),
            qApp,
            SLOT(quit()),
            QKeySequence(tr("CTRL+Q")));

    connectionMenu->addAction(tr("Show log window"),
            this,
            SLOT(showLogWindow()),
            QKeySequence(tr("CTRL+L")));

    connectionMenu->addAction(tr("Connect..."),
            this,
            SLOT(startConnect()));

    connectionMenu->addSeparator();

    closeWindowAction = connectionMenu->addAction(
            tr("Close QtNX when NX starts"));

    closeWindowAction->setCheckable(true);
    closeWindowAction->setChecked(true);
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
    QMessageBox::critical(this, tr("Authentication failure"),
            tr("You have supplied an incorrect username or password for this " \
               "NX server."), QMessageBox::Ok, QMessageBox::NoButton,
            QMessageBox::NoButton);

    statusBar->showMessage(tr("Login failed"));

    reinitialiseClient();
}

void QtNXWindow::handleAtCapacity()
{
    QMessageBox::critical(this, tr("Server at capacity"),
            tr("This NX server is running at capacity."), QMessageBox::Ok, QMessageBox::NoButton,
            QMessageBox::NoButton);

    statusBar->showMessage(tr("Login failed"));

    reinitialiseClient();
}

void QtNXWindow::reinitialiseClient()
{
    delete m_NXClient;

    m_NXClient = new NXClientLib();
    initialiseClient();
}

void QtNXWindow::sshContinue(QString message)
{
    /*TODO: this is not functionally available in nxcl

    int reply = QMessageBox::question(this,
            tr("SSH Request"),
            message,
            QMessageBox::Yes,
            QMessageBox::No,
            QMessageBox::NoButton);

    if (reply == QMessageBox::Yes)
        m_NXClient->allowSSHConnect(true);
    else
        m_NXClient->allowSSHConnect(false);
    */
}

void QtNXWindow::startConnect()
{
    string key = "";

    if (ui_lg.session->currentText() == tr("Create new session")) {
        QMessageBox::critical(this, tr("Unconfigured"),
                tr("Please select a session profile to connect with, or configure one"), 
                QMessageBox::Ok, QMessageBox::NoButton, QMessageBox::NoButton);
        return;
    }

    parseXML();

    if (config.sessionType == "unix-application")
        session.customCommand = config.customCommand;

    setDefaultData();

    if (ui_lg.link->currentText() == tr("Modem"))
        session.linkType = "modem";
    else if (ui_lg.link->currentText() == tr("ISDN"))
        session.linkType = "isdn";
    else if (ui_lg.link->currentText() == tr("ADSL"))
        session.linkType = "adsl";
    else if (ui_lg.link->currentText() == tr("WAN"))
        session.linkType = "wan";
    else if (ui_lg.link->currentText() == tr("LAN"))
        session.linkType = "lan";

    if (!config.key.empty()) {
        key = config.key;
        session.key = "supplied";
    } else
        session.key = "default";

    if (config.sessionType == "unix-application")
        session.customCommand = config.customCommand;

    string username = ui_lg.username->text().toStdString();
    string password = ui_lg.password->text().toStdString();

    m_NXClient->setSessionData(&session);

    m_NXClient->setUsername(username);
    m_NXClient->setPassword(password);
    m_NXClient->setResolution(getWidth(), getHeight());

    m_NXClient->setDepth(getDepth());

    QString keyPath = "id.key";

#ifdef Q_WS_MAC
    keyPath = binaryPath + "/id.key";
#endif

    m_NXClient->invokeNXSSH(keyPath.toStdString(), config.serverHost, config.encryption, "",
            config.serverPort);

    processProbe->start(30);
}

void QtNXWindow::setDefaultData()
{
    // These seem to be a fairly sane set of defaults for session data
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
    session.encryption = true;
    session.virtualDesktop = false;
    session.terminate = false;
}

int QtNXWindow::getDepth()
{
    return qApp->desktop()->depth();
}

int QtNXWindow::getWidth()
{
    return qApp->desktop()->screenGeometry(this).width();
}

int QtNXWindow::getHeight()
{
    return qApp->desktop()->screenGeometry(this).height();
}

void QtNXWindow::handleTimeout()
{
    notQProcess* p = m_NXClient->getNXSSHProcess();

    if ((m_NXClient->getIsFinished()) == false) {
        if (m_NXClient->getReadyForProxy() == false) {
            p->probeProcess();
        } else if (m_NXClient->needX11Probe()) {
            p->probeProcess();
            p = m_NXClient->getX11Process();
            p->probeProcess();
        } else {
            p->probeProcess();
            p = m_NXClient->getNXProxyProcess();
            p->probeProcess();
        }
        if (!m_NXClient->getSessionRunning())
            processProbe->start(30);
        else {
            processProbe->start(2000);
        }
    } else {
        processProbe->stop();
    }
}

void QtNXWindow::parseXML()
{
    NXParseXML handler;
    handler.setData(&config);

    QFile file(QDir::homePath() + "/.qtnx/" +
            ui_lg.session->currentText() + ".nxml");

    QXmlInputSource inputSource(&file);

    QXmlSimpleReader reader;
    reader.setContentHandler(&handler);
    reader.setErrorHandler(&handler);
    reader.parse(inputSource);

    if (config.linkType == "modem")
        ui_lg.link->setCurrentIndex(ui_lg.link->findText(tr("Modem")));
    else if (config.linkType == "isdn")
        ui_lg.link->setCurrentIndex(ui_lg.link->findText(tr("ISDN")));
    else if (config.linkType == "adsl")
        ui_lg.link->setCurrentIndex(ui_lg.link->findText(tr("ADSL")));
    else if (config.linkType == "wan")
        ui_lg.link->setCurrentIndex(ui_lg.link->findText(tr("WAN")));
    else if (config.linkType == "lan")
        ui_lg.link->setCurrentIndex(ui_lg.link->findText(tr("LAN")));
}

void QtNXWindow::configure()
{
    if (ui_lg.session->currentText() == tr("Create new session"))
        settingsDialog = new QtNXSettings("");
    else
        settingsDialog = new QtNXSettings(ui_lg.session->currentText());

    connect(settingsDialog, SIGNAL(closing(QString)), this, SLOT(configureClosed(QString)));

    settingsDialog->show();
}

void QtNXWindow::configureClosed(QString sessionName)
{
    while (ui_lg.session->count() != 0) {
        ui_lg.session->removeItem(0);
    }

    QDir dir(QDir::homePath() + "/.qtnx", "*.nxml");
    for (unsigned int i = 0; i < dir.count(); i++) {
        QString conn=dir[i];
        ui_lg.session->addItem(conn.left(conn.length() - 5));
    }

    ui_lg.session->addItem(tr("Create new session"));

    ui_lg.session->setCurrentIndex(
            ui_lg.session->findText(sessionName));
}

void QtNXWindow::resumeNewPressed()
{
    m_NXClient->setSessionData(&session);
    m_NXClient->runSession();
}

void QtNXWindow::resumeResumePressed(QString id)
{
    session.id = id.toStdString();
    session.suspended = true;
    m_NXClient->setSessionData(&session);
    m_NXClient->runSession();
}

void QtNXWindow::handleStatus(QString message)
{
    handleLogging(message);
    statusBar->showMessage(message);
}

void QtNXWindow::handleSuspendedSessions(QList<NXResumeData> data)
{
    delete sessionsDialog;
    sessionsDialog = new QtNXSessions(data);
    sessionsDialog->show();

    connect(sessionsDialog, SIGNAL(newPressed()), this, SLOT(resumeNewPressed()));
    connect(sessionsDialog, SIGNAL(resumePressed(QString)),
            this, SLOT(resumeResumePressed(QString)));
}

void QtNXWindow::handleNoSessions()
{
    session.suspended = false;
    m_NXClient->setSessionData(&session);
    m_NXClient->runSession();
}

void QtNXWindow::handleLogging(QString message)
{
    if (message.right(1) != "\n")
        cout << message.toStdString() << endl;
    else
        cout << message.toStdString();

    ui_lw.output->append(message);
}

void QtNXWindow::handleProgress(int id, QString message)
{
    if (!message.isEmpty())
        message = ": " + message;

    switch (id) {
        case NXCL_PROCESS_STARTED:
            handleStatus(tr("Process started"));
            break;
        case NXCL_PROCESS_EXITED:
            handleStatus(tr("Process exited"));
            break;
        case NXCL_AUTH_FAILED:
            handleStatus(tr("Invalid authentication key"));
            break;
        case NXCL_AUTHENTICATING:
            handleStatus(tr("Authenticating client"));
            break;
        case NXCL_LOGIN_FAILED:
            handleStatus(tr("Invalid username or password"));
            failedLogin();
            break;
        case NXCL_HOST_KEY_VERIFAILED:
            handleStatus(tr("Host key verification failed"));
            break;
        case NXCL_INVOKE_PROXY:
            handleStatus(tr("Starting NX proxy"));
            break;
        case NXCL_STARTING:
            handleStatus(tr("Starting session"));
            break;
        case NXCL_FINISHED:
            handleStatus(tr("Finished connecting"));
            break;
        case NXCL_ALIVE:
            handleStatus(tr("NX session active"));
            break;
        case NXCL_PROCESS_ERROR:
            handleStatus(tr("Process error"));
            break;
        default:
            break;
    }
}

