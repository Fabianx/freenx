/***************************************************************************
                               nxclientlib.cpp
                             -------------------
    begin                : Sat 22nd July 2006
    modifications        : July 2007
    copyright            : (C) 2006 by George Wright
    modifications        : (C) 2007 Embedded Software Foundry Ltd. (U.K.)
                         :     Author: Sebastian James
                         : (C) 2008 Defuturo Ltd
                         :     Author: George Wright
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

#include "nxclientlib_i18n.h"
#include "nxclientlib.h"
#include "nxdata.h"

#include "../config.h"

#include <fstream>

extern "C" {
    #include <errno.h>
    #include <sys/types.h>
    #include <sys/stat.h>
    #include <unistd.h>
}

/*
 * On the location of nxproxy and nxssh binaries
 * --------------------------------------------- 
 * We expect them to be installed in PACKAGE_BIN_DIR (See
 * Makefile.am). So, if nxcl is installed in /usr/bin/nxcl then we
 * call /usr/bin/nxssh and /usr/bin/nxproxy, etc etc.
 */

using namespace std;
using namespace nxcl;

/*!
 * Implementation of the NXClientLibCallbacks class
 */
//@{
NXClientLibCallbacks::NXClientLibCallbacks()
{
}

NXClientLibCallbacks::~NXClientLibCallbacks()
{
}

void NXClientLibCallbacks::startedSignal (string name)
{
    this->parent->externalCallbacks->write
        (NXCL_PROCESS_STARTED, name + _(" process started"));
}

void NXClientLibCallbacks::processFinishedSignal (string name)
{
    this->parent->externalCallbacks->write
        (NXCL_PROCESS_EXITED, name + _(" process exited"));
    parent->setIsFinished (true);
}

void NXClientLibCallbacks::errorSignal (int error)
{
    string message;
    switch (error) {
        case NOTQPROCFAILEDTOSTART:
            message = _("The process failed to start");
            break;
        case NOTQPROCCRASHED:
            message = _("The process has crashed");
            break;
        case NOTQPROCTIMEDOUT:
            message = _("The process timed out");
            break;
        case NOTQPROCWRITEERR:
            message = _("There was an error writing to the process");
            break;
        case NOTQPROCREADERR:
            message = _("There was an error reading from the process");
            break;
        default:
            message = _("There was an unknown error with the process");
            break;
    }

    this->parent->externalCallbacks->error (message);
}

void NXClientLibCallbacks::readyReadStandardOutputSignal()
{
    this->parent->processParseStdout();
}

void NXClientLibCallbacks::readyReadStandardErrorSignal()
{
    this->parent->processParseStderr();
}

/*!
 * This gets called from within the NXSession object...
 */
void NXClientLibCallbacks::noSessionsSignal()
{
    this->parent->externalCallbacks->noSessionsSignal();
}

void NXClientLibCallbacks::loginFailedSignal()
{
    this->parent->loginFailed();
}

void NXClientLibCallbacks::readyForProxySignal()
{
    this->parent->readyproxy();
}

void NXClientLibCallbacks::authenticatedSignal()
{
    this->parent->doneAuth();
}

void NXClientLibCallbacks::sessionsSignal (list<NXResumeData> data)
{
    this->parent->externalCallbacks->resumeSessionsSignal (data);
}
//@}

/*!e
 * Implementation of the NXClientLib class
 */
//@{
NXClientLib::NXClientLib() :
    nxsshProcess(new notQProcess()),
    nxproxyProcess(new notQProcess()),
    x11Process(new notQProcess()),
    nxauthProcess(new notQProcess())
{
    this->isFinished = false;
    this->readyForProxy = false;
    this->sessionRunning = false;
    this->proxyData.encrypted = false;
    this->password = false;

    dbgln ("In NXClientLib constructor");

    /* Set up callback pointers */
    this->nxsshProcess->setCallbacks (&callbacks);
    this->nxproxyProcess->setCallbacks (&callbacks);
    this->x11Process->setCallbacks (&callbacks);
    this->nxauthProcess->setCallbacks (&callbacks);

    this->session.setCallbacks (&callbacks);
    this->callbacks.setParent (this);

    dbgln ("Returning from NXClientLib constructor");
}

NXClientLib::~NXClientLib()
{
    dbgln ("In NXClientLib destructor");
}

void NXClientLib::invokeNXSSH (string publicKey, string serverHost,
        bool encryption, string key, int port)
{
    list<string> arguments;
    stringstream argtmp;
    proxyData.server = serverHost;

    dbgln("invokeNXSSH called");

    // We use same environment for the process as was used for the
    // parent, so remove nxsshProcess->setEnvironment();

    // Start to build the arguments for the nxssh command.
    // notQProcess requires that argv[0] contains the program name
    arguments.push_back ("nxssh");

    argtmp << "-nx";
    arguments.push_back (argtmp.str());

    argtmp.str("");
    argtmp << "-p" << port;
    arguments.push_back (argtmp.str());

    if (publicKey == "supplied") {

        this->keyFile = new notQTemporaryFile;
        this->keyFile->open();

        argtmp.str("");
        argtmp << "-i" << this->keyFile->fileName();
        arguments.push_back (argtmp.str());

        this->keyFile->write (key);			
        this->keyFile->close();

    } else {
        this->keyFile = NULL;
        argtmp.str("");
        argtmp << "-i" << publicKey;
        arguments.push_back (argtmp.str());
    }

    argtmp.str("");
    argtmp << "nx@" << serverHost;
    arguments.push_back (argtmp.str());

    // These options copied from the way Nomachine's client
    // specifies the nxssh command - they make good sense.
    arguments.push_back ("-x");
    arguments.push_back ("-2");
    arguments.push_back ("-oRhostsAuthentication no");
    arguments.push_back ("-oPasswordAuthentication no");
    arguments.push_back ("-oRSAAuthentication no");
    arguments.push_back ("-oRhostsRSAAuthentication no");
    arguments.push_back ("-oPubkeyAuthentication yes");

    if (encryption == true) {
        arguments.push_back("-B");
        session.setEncryption (true);
    } else {
        session.setEncryption (false);
    }

    // -E appears in the call to nxssh for Nomachine's nxclient
    // -version 3 but not 1.5. Is it there in 2?
    // nxssh -E gives this message when called:
    // NX> 285 Enabling skip of SSH config files
    // ...so there you have the meaning.
    arguments.push_back ("-E");

    // Find a path for the nxssh process using getPath()
    string nxsshPath = this->getPath ("nxssh");

    this->nxsshProcess->start(nxsshPath, arguments);

    if (this->nxsshProcess->waitForStarted() == false) {
        this->externalCallbacks->write
            (NXCL_PROCESS_ERROR, _("Error starting nxssh!"));
        this->isFinished = true;
    }
}

void NXClientLib::requestConfirmation (string msg)
{
    this->externalCallbacks->stdoutSignal
        (_("This is a placeholder method to deal with sending "
            "back a yes or a no answer. "
            "For now, we just set this->session.setContinue(true);"));
    this->session.setContinue(true);
}

void NXClientLib::reset()
{
    this->nxsshProcess->terminate();
    this->isFinished = false;
    this->proxyData.encrypted = false;
    this->password = false;	
    this->session.resetSession();
}

void NXClientLib::loginFailed()
{
    this->externalCallbacks->write
        (NXCL_LOGIN_FAILED, _("Got \"Login Failed\""));

    this->isFinished = true;
    this->nxsshProcess->terminate();
}

void NXClientLib::processParseStdout()
{
    string message = nxsshProcess->readAllStandardOutput();

    this->externalCallbacks->stdoutSignal (message);

    dbgln ("NXClientLib::processParseStdout() called");

    int response = 0;

    // Message 211 is sent if ssh is asking to continue with an unknown host
    if ((response = session.parseResponse(message)) == 211) {
        this->requestConfirmation (message);
    }

    dbgln ("NXClientLib::processParseStdout(): response = " << response);

    if (response == 100000) {
        // A program never started.
        this->isFinished = true;
        return;
    } else if (response > 100000) {
        dbgln ("A process crashed or exited");

        int pid = response - 100000;

        if (this->nxsshProcess->getPid() == pid) {
            this->nxsshProcess->setError(NOTQPROCCRASHED);

            this->externalCallbacks->error
                (_("nxsshProcess crashed or exited"));

            this->isFinished = true;
        } else if (this->nxproxyProcess->getPid() == pid) {
            this->nxproxyProcess->setError(NOTQPROCCRASHED);

            this->externalCallbacks->error
                (_("nxproxyProcess crashed or exited"));

            this->isFinished = true;
        } else {
            this->externalCallbacks->error
                (_("Warning: Don't know what crashed "
                   "(in processParseStdout())"));
        }
        return;
    }

    // If message 204 is picked, that's authentication failed.
    if (response == 204) {
        this->externalCallbacks->write (NXCL_AUTH_FAILED,
                _("Got \"Authentication Failed\" from nxssh.\n"
                    "Please check the certificate for the first SSL "
                    "authentication stage,\n"
                    "in which the \"nx\" user is authenticated."));
        this->isFinished = true;
        return;
    }

    // 147 is server capacity reached
    if (response == 147) {
        this->externalCallbacks->serverCapacitySignal();
        this->isFinished = true;
        return;
    }

    dbgln ("NXClientLib::processParseStdout(): The message is '"
            + message + "'(msg end)");

    dbgln ("...and response is " << response);

    notQtUtilities::ensureUnixNewlines (message);

    list<string> msglist;
    list<string>::iterator msgiter;

    notQtUtilities::splitString (message, '\n', msglist);

    for (msgiter = msglist.begin(); msgiter != msglist.end(); msgiter++) {
        dbgln ("NXClientLib::processParseStdout(): Processing the message '"
                + (*msgiter) + "'(end msg)");

        // On some connections this is sent via stdout instead of stderr?
        if (proxyData.encrypted && readyForProxy &&
                ((*msgiter).find("NX> 999 Bye")!=string::npos)) {

            // This is "NX> 299 Switching connection to: " in
            // version 1.5.0. This was changed in nxssh version
            // 2.0.0-8 (see the nxssh CHANGELOG).
            string switchCommand = "NX> 299 Switch connection to: ";

            stringstream ss;

            ss << "127.0.0.1:" << proxyData.port << " cookie: " <<
                proxyData.cookie << "\n";
            switchCommand += ss.str();

            this->write (switchCommand);
        } else if ((*msgiter).find
                ("NX> 287 Redirected I/O to channel descriptors") !=
                string::npos) {

            this->externalCallbacks->write
                (287, _("The session has been started successfully"));
            this->externalCallbacks->connectedSuccessfullySignal();
            this->sessionRunning = true;
        }

        if ((*msgiter).find("Password") != string::npos) {
            this->externalCallbacks->write
                (NXCL_AUTHENTICATING, _("Authenticating with NX server"));
            this->password = true;
        }

        if (!readyForProxy) {
            string msg = session.parseSSH (*msgiter);
            if (msg == "204\n" || msg == "147\n") {
                // Auth failed.
                dbgln ("NXClientLib::processParseStdout: Got auth failed"
                        " or capacity reached, calling this->parseSSH.");
                msg = this->parseSSH (*msgiter);
            }
            if (msg.size() > 0) {
                this->write (msg);
            }
        } else {
            this->write (this->parseSSH (*msgiter));
        }
    }
    return;
}

void NXClientLib::processParseStderr()
{
    string message = nxsshProcess->readAllStandardError();

    dbgln ("In NXClientLib::processParseStderr for message: '"
            + message + "'(msg end)");

    this->externalCallbacks->stderrSignal (message);

    // Now we need to split the message if necessary based on the
    // \n or \r characters
    notQtUtilities::ensureUnixNewlines (message);

    list<string> msglist;
    list<string>::iterator msgiter;
    notQtUtilities::splitString (message, '\n', msglist);

    for (msgiter=msglist.begin(); msgiter!=msglist.end(); msgiter++) {

        dbgln ("NXClientLib::processParseStderr: Processing the message '"
                + (*msgiter) + "'(end msg)");

        if (proxyData.encrypted && readyForProxy &&
                ((*msgiter).find("NX> 999 Bye") != string::npos)) {

            string switchCommand = "NX> 299 Switch connection to: ";
            stringstream ss;

            ss << "127.0.0.1:" << proxyData.port << " cookie: "
                << proxyData.cookie << "\n";

            switchCommand += ss.str();
            this->write(switchCommand);

        } else if ((*msgiter).find
                ("NX> 287 Redirected I/O to channel descriptors") !=
                string::npos) {

            this->externalCallbacks->write
                (287, _("The session has been started successfully"));
            this->externalCallbacks->connectedSuccessfullySignal();

        } else if ((*msgiter).find
                ("NX> 209 Remote host identification has changed") !=
                string::npos) {

            this->externalCallbacks->write(209, _("SSH Host Key Problem"));
            this->isFinished = true;

        } else if ((*msgiter).find
                ("NX> 280 Ignoring EOF on the monitored channel") !=
                string::npos) {

            this->externalCallbacks->write
                (280, _("Got \"NX> 280 Ignoring EOF on the monitored channel\""
                        " from nxssh..."));
            this->isFinished = true;

        } else if ((*msgiter).find
                ("Host key verification failed") != string::npos) {
            this->externalCallbacks->write
                (NXCL_HOST_KEY_VERIFAILED,
                 _("SSH host key verification failed"));
            this->isFinished = true;
        }
    }
}

void NXClientLib::write (string data)
{
    if (data.size() == 0) { return; }

    dbgln ("Writing '" << data << "' to nxssh process.");

    this->nxsshProcess->writeIn(data);

    if (password) {
        data = "********";
        password = false;
    }

    // Output this to the user via a signal - this is data going in to nxssh.
    this->externalCallbacks->stdinSignal (data);
}

void NXClientLib::doneAuth()
{
    if (this->keyFile != NULL) {
        this->keyFile->remove();
        delete this->keyFile;
    }
    return;
}

void NXClientLib::allowSSHConnect (bool auth)
{
    session.setContinue (auth);
}

void NXClientLib::setSessionData (NXSessionData *nxSessionData)
{
    session.setSessionData (nxSessionData);
    string a = "NX> 105";
    string d = session.parseSSH(a);
    if (d.size()>0) {
        this->write(d);
    }
}

void NXClientLib::runSession ()
{
    session.runSession();
    string a = "NX> 105";
    string d = session.parseSSH(a);
    if (d.size()>0) {
        this->write(d);
    }
}

string NXClientLib::parseSSH (string message)
{
    string rMessage;
    string::size_type pos;
    rMessage = "";

    dbgln ("NXClientLib::parseSSH called for message '" + message + "'");

    if ((pos = message.find("NX> 700 Session id: ")) != string::npos) {
        this->externalCallbacks->write (700, _("Got a session ID"));
        proxyData.id = message.substr(pos+20, message.length()-pos);

    } else if ((pos = message.find("NX> 705 Session display: ")) != string::npos) {
        stringstream portss;
        int portnum;
        portss << message.substr(pos+25, message.length()-pos);
        portss >> portnum;		
        proxyData.display = portnum;
        proxyData.port = portnum + 4000;

    } else if
        ((pos = message.find("NX> 706 Agent cookie: ")) != string::npos) {

        proxyData.cookie = message.substr(pos+22, message.length()-pos);
        this->externalCallbacks->write (706, _("Got an agent cookie"));

    } else if
        ((pos = message.find("NX> 702 Proxy IP: ")) != string::npos) {

        proxyData.proxyIP = message.substr(pos+18, message.length()-pos);
        this->externalCallbacks->write (702, _("Got a proxy IP"));

    } else if
        (message.find("NX> 707 SSL tunneling: 1") != string::npos) {

        this->externalCallbacks->write
            (702, _("All data will be SSL tunnelled"));

        proxyData.encrypted = true;

    } else if (message.find("NX> 147 Server capacity") != string::npos) {

        this->externalCallbacks->write
            (147, _("Got \"Server Capacity Reached\" from nxssh."));

        this->externalCallbacks->serverCapacitySignal();
        this->isFinished = true;

    } else if
        (message.find ("NX> 204 Authentication failed.") != string::npos) {

        this->externalCallbacks->write
            (204, _("NX SSH Authentication Failed, finishing"));
        this->isFinished = true;
    }

    if (message.find("NX> 710 Session status: running") != string::npos) {

        this->externalCallbacks->write
            (710, _("Session status is \"running\""));
        invokeProxy();
        session.wipeSessions();
        rMessage = "bye\n";
    }

    return rMessage;
}

void NXClientLib::invokeProxy()
{
    this->externalCallbacks->write
        (NXCL_INVOKE_PROXY, _("Starting NX session"));

#if NXCL_CYGWIN
    NXSessionData* sessionData = getSession()->getSessionData();

    string res;

    if (sessionData->geometry == "fullscreen") {
        stringstream resolution;

        ostringstream dimensionX, dimensionY;
        dimensionX << sessionData->xRes;
        dimensionY << sessionData->yRes;

        resolution << dimensionX.str() << "x" << dimensionY.str();
        res = resolution.str();
    } else {
        res = sessionData->geometry;
    }

    startX11(res, "");
#endif

#if NXCL_DARWIN
    // Let's run open -a X11 to fire up X
    list<string> x11Arguments;

    x11Arguments.push_back("open");
    x11Arguments.push_back("-a");
    x11Arguments.push_back("X11");

    string openPath = this->getPath("open");
    
    this->x11Process->start(openPath, x11Arguments);

    this->x11Probe = true;
    
    if (this->x11Process->waitForStarted() == false) {
        this->externalCallbacks->write
            (NXCL_PROCESS_ERROR, _("Error starting X11!"));
        this->isFinished = true;
    }
    
    this->x11Probe = false;

    // Horrendous hack - must fix
    for (int i = 0; i < 32768; i++) {};
#endif

    int e;
    char * home;

    home = getenv ("HOME");

    stringstream ss;
    ss << home;

    string nxdir = ss.str();

    nxdir += "/.nx";

    // Create the .nx directory first.
    if (mkdir (nxdir.c_str(), 0770)) {
        e = errno;

        if (e != EEXIST) {
            // We don't mind .nx already
            // existing, though if there is a
            // _file_ called $HOME/.nx, we'll
            // get errors later.
            this->externalCallbacks->error
                (_("Problem creating .nx directory"));
        }
    }

    // Now the per session directory
    nxdir += "/S-" + proxyData.id;

    if (mkdir (nxdir.c_str(), 0770)) {
        e = errno;

        if (e != EEXIST) {
            // We don't mind .nx already
            this->externalCallbacks->error
                (_("Problem creating Session directory"));
        }
    }

    string x11Display = "";

#if NXCL_DARWIN || NXCL_CYGWIN
    x11Display = ",display=:0.0";
#endif

    stringstream data;
 
    if (proxyData.encrypted) {
        data << "nx/nx" << x11Display << ",session=session,encryption=1,cookie="
            << proxyData.cookie
            << ",id=" << proxyData.id << ",listen=" 
            << proxyData.port << ":" << proxyData.display << "\n";
        // may also need shmem=1,shpix=1,font=1,product=...

    } else {
        // Not tested yet
        data << "nx/nx" << x11Display << ",session=session,cookie=" << proxyData.cookie
            << ",id=" << proxyData.id
            // << ",connect=" << proxyData.server << ":" << proxyData.display
            << ",listen=" << proxyData.port << ":" << proxyData.display
            << "\n";
    }

    // Filename is nxdir plus "/options"
    nxdir += "/options";
    std::ofstream options;
    options.open (nxdir.c_str(), std::fstream::out);
    options << data.str();
    options.close();

    // Build arguments for the call to the nxproxy command
    list<string> arguments;
    arguments.push_back("nxproxy"); // argv[0] has to be the program name
    arguments.push_back("-S");
    ss.str("");
    ss << "options=" << nxdir;
    ss << ":" << proxyData.display;
    arguments.push_back(ss.str());	

    // Find a path for the nxproxy process using getPath()
    string nxproxyPath = this->getPath ("nxproxy");
    this->nxproxyProcess->start(nxproxyPath, arguments);

    if (this->nxproxyProcess->waitForStarted() == false) {
        this->externalCallbacks->write
            (NXCL_PROCESS_ERROR, _("Error starting nxproxy!"));
        this->isFinished = true;
    }
}

void NXClientLib::startX11 (string resolution, string name)
{
#if NXCL_CYGWIN
    // Invoke NXWin.exe on Windows machines

    // See if XAUTHORITY path is set

    stringstream xauthority;

    xauthority << getenv("HOME") << "/.Xauthority";

    // Now we set the environment variable
    setenv("XAUTHORITY", xauthority.str().c_str(), 1);

    // Now we actually start NXWin.exe
    list<string> nxwinArguments;

    // Arguments taken from 2X
    nxwinArguments.push_back("NXWin");

    nxwinArguments.push_back("-nowinkill");
    nxwinArguments.push_back("-clipboard");
    nxwinArguments.push_back("-noloadxkb");
    nxwinArguments.push_back("-agent");
    nxwinArguments.push_back("-hide");
    nxwinArguments.push_back("-noreset");
    nxwinArguments.push_back("-nolisten");
    nxwinArguments.push_back("tcp");

    // TODO: If rootless, append "-multiwindow" and "-hide" but only
    // hide if not restoring

    // Now we set up the font paths. By default this is $NX_SYSTEM/usr/X11R6/...

    stringstream fontPath;

    fontPath << getenv("NX_SYSTEM") << "/X11R6/lib/X11/fonts/base,"
        << getenv("NX_SYSTEM") << "/X11R6/lib/X11/fonts/TTF";

    nxwinArguments.push_back("-fp");
    nxwinArguments.push_back(fontPath.str());

    nxwinArguments.push_back("-name");
    nxwinArguments.push_back("NXWin");
    nxwinArguments.push_back(":0");

    nxwinArguments.push_back("-screen");
    nxwinArguments.push_back("0");

    nxwinArguments.push_back(resolution);

    nxwinArguments.push_back(":0");
    nxwinArguments.push_back("-nokeyhook");

    string nxwinPath = this->getPath("nxwin");

    this->x11Process->start(nxwinPath, nxwinArguments);

    this->x11Probe = true;

    if (this->x11Process->waitForStarted() == false) {
        this->externalCallbacks->write
            (NXCL_PROCESS_ERROR, _("Error starting nxwin!"));
        this->isFinished = true;
    }

    this->x11Probe = false;

    list<string> nxauthArguments;

    char hostname[256];

    gethostname(hostname, 256);

    string cookie = getSession()->generateCookie();
    cookie.resize(32);

    stringstream domain;

    domain << hostname << "/unix:0";

    // These arguments taken from the 2X GPL client
    // We're going to assume that nxauth is in PATH
    nxauthArguments.push_back("nxauth");
    nxauthArguments.push_back("-i");
    nxauthArguments.push_back("-f");
    nxauthArguments.push_back(xauthority.str());
    nxauthArguments.push_back("add");
    nxauthArguments.push_back(domain.str());
    nxauthArguments.push_back("MIT-MAGIC-COOKIE-1");
    nxauthArguments.push_back(cookie);

    this->nxauthProcess->setCallbacks (&callbacks);

    string nxauthPath = this->getPath("nxauth");

    this->nxauthProcess->start(nxauthPath, nxauthArguments);

    if (this->nxauthProcess->waitForStarted() == false) {
        this->externalCallbacks->write
            (NXCL_PROCESS_ERROR, _("Error starting nxauth!"));
        this->isFinished = true;
    }
#endif
}

bool NXClientLib::chooseResumable (int n)
{
    return (this->session.chooseResumable(n));
}

bool NXClientLib::terminateSession (int n)
{
    return (this->session.terminateSession(n));
}

string NXClientLib::getPath (string prog)
{
    string path;
    struct stat * buf;

    buf = static_cast<struct stat*>(malloc (sizeof (struct stat)));

    if (!buf) {
        // Malloc error.
        return prog;
    }

    // We'll check the custom search path first
    stringstream pathTest;
    pathTest << customPath << "/" << prog;

    memset (buf, 0, sizeof(struct stat));
    stat (pathTest.str().c_str(), buf);

    if (S_ISREG (buf->st_mode) || S_ISLNK (buf->st_mode)) {
        // Found in custom path
        free(buf);
        return pathTest.str();
    }

    path = PACKAGE_BIN_DIR"/" + prog;
    memset (buf, 0, sizeof(struct stat));
    stat (path.c_str(), buf);

    if (S_ISREG (buf->st_mode) || S_ISLNK (buf->st_mode)) {
        // Found prog in PACKAGE_BIN_DIR
    } else {
        path = "/usr/local/bin/" + prog;
        memset (buf, 0, sizeof(struct stat));
        stat (path.c_str(), buf);

        if (S_ISREG (buf->st_mode) || S_ISLNK (buf->st_mode)) {
            // Found prog in /usr/local/bin
        } else {
            path = "/usr/bin/" + prog;
            memset (buf, 0, sizeof(struct stat));
            stat (path.c_str(), buf);

            if (S_ISREG (buf->st_mode) ||
                    S_ISLNK (buf->st_mode)) {
                // Found prog in /usr/bin

            } else {
                path = "/usr/NX/bin/" + prog;
                memset (buf, 0, sizeof(struct stat));
                stat (path.c_str(), buf);

                if (S_ISREG (buf->st_mode) || 
                        S_ISLNK (buf->st_mode)) {

                } else {
                    path = "/bin/" + prog;
                    memset (buf, 0, sizeof(struct stat));
                    stat (path.c_str(), buf);
                    if (S_ISREG (buf->st_mode) || 
                            S_ISLNK (buf->st_mode)) {
                        // Found prog in /bin
                    } else {
                        // Just return the
                        // prog name.
                        path = prog;
                    }
                }
            }
        }
    }

    free (buf);
    return path;
}
//@}
