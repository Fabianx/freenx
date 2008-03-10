/***************************************************************************
                               nxclientlib.h
                             -------------------
    begin                : Sat 22nd July 2006
    remove Qt dependency : Started June 2007
    modifications        : June-July 2007
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

#ifndef _NXCLIENTLIB_H_
#define _NXCLIENTLIB_H_

#include <iostream>
#include "nxsession.h"
#include <list>
#include "notQt.h"


using namespace std;

namespace nxcl {

    struct ProxyData {
        string id;
        int    display;
        string cookie;
        string proxyIP;
        bool   encrypted;
        int    port;
        string server;
    };

    /*!
     * Callbacks which are to be defined by the client code of
     * NXClientLib objects. In the case of nxcl, that means the
     * code in the class Nxcl in nxcl.cpp.
     */
    class NXClientLibExternalCallbacks
    {
        public:
            NXClientLibExternalCallbacks () {}
            virtual ~NXClientLibExternalCallbacks () {}
            virtual void write (string msg) {}
            virtual void write (int num, string msg) {}
            virtual void error (string msg) {}
            virtual void debug (string msg) {}
            virtual void stdoutSignal (string msg) {}
            virtual void stderrSignal (string msg) {}
            virtual void stdinSignal (string msg) {}
            virtual void resumeSessionsSignal (list<NXResumeData>) {}
            virtual void noSessionsSignal (void) {}
            virtual void serverCapacitySignal (void) {}
            virtual void connectedSuccessfullySignal (void) {}
    };

    /*!
     * Have to derive NXClientLib from an abstract base class,
     * NXClientLibBase, so that NXClientLibCallbacks can
     * hold a pointer to an NXClientLib object.
     *
     * The functions that are declared in NXClientLibBase are the
     * ones that we want to call via this->parent in
     * NXClientLibCallbacks. They're the ones that are called from
     * within objects of other classes (such as this->session
     * (NXSession) or this->nxsshProcess (notQProcess).
     */
    class NXClientLibBase 
    {
        public:
            NXClientLibBase() {}
            virtual ~NXClientLibBase() {}

            virtual void setIsFinished (bool status) {}
            virtual void processParseStdout (void) {}
            virtual void processParseStderr (void) {}
            virtual void loginFailed (void) {}
            virtual void readyproxy (void) {}
            virtual void doneAuth (void) {}

            /*!
             * External callbacks pointer is held in NXClientLibBase
             * because NXClientLibProcessCallbacks::parent is of
             * type NXClientLibBase and in NXClientLibProcessCallbacks we
             * refer to this->parent->externalCallbacks->write()
             */
            NXClientLibExternalCallbacks * externalCallbacks;
    };

    /*!
     * Callbacks class. This derives from several other base
     * callbacks classes, defining the behaviour of the callbacks.
     */
    class NXClientLibCallbacks : public notQProcessCallbacks,
        public NXSessionCallbacks
    {
        public:
            NXClientLibCallbacks();
            ~NXClientLibCallbacks();

            /*!
             * The callback signals
             */
            //@{
            /*!
             * From notQProcess:
             */
            //@{
            void startedSignal (string name);
            void errorSignal (int error);
            void processFinishedSignal (string name);
            void readyReadStandardOutputSignal (void);
            void readyReadStandardErrorSignal (void);
            //@}
            /*!
             * From NXSession:
             */
            //@{
            void noSessionsSignal (void);
            void loginFailedSignal (void);
            void readyForProxySignal (void);
            void authenticatedSignal (void);
            void sessionsSignal (list<NXResumeData>);
            //@}
            //@}

            /*!
             * Accessor function to set a pointer to the parent NXCLientLib
             * object.
             */
            void setParent (NXClientLibBase * p) { this->parent = p; }
        private:
            NXClientLibBase * parent;
    };

    class NXClientLib : public NXClientLibBase
    {
        public:
            NXClientLib();
            ~NXClientLib();

            /*!
             * Set up data and then call this->nxsshProcess.start().
             * 
             * \param publicKey is the path to the ssh public key
             * file to authenticate with.  Pass "supplied" to use
             * a new key, which you should then supply as the
             * parameter key.
             *
             * \param serverHost is the hostname of the NX server to
             * connect to
             *
             * \param encryption is whether to use an encrypted NX
             * session
             *
             * \param key ssh key to use for authentication of the
             * nx user if publicKey is "supplied".
             *
             * \param port TCP port to use for the ssh connection.
             */
            void invokeNXSSH (string publicKey = "supplied",
                    string serverHost = "",
                    bool encryption = true,
                    string key = "",
                    int port = 22);

            /*!
             * Overloaded to give callback data on write.
             * 
             * Writes data to this->nxsshProcess stdin and also
             * out to the user via stdoutCallback
             */
            void write (string data);

            /*!
             * Sets a custom binary search path
             */
            void setCustomPath(string path)
            {
                this->customPath = path;
            }
            
            /*!
             * Passes auth to this->session.setContinue()
             */
            void allowSSHConnect (bool auth);

            /*!
             * Set up data and then call this->nxproxyProcess.start()
             */
            void invokeProxy (void);

            /*!
             * Parse a line of output from
             * this->nxsshProcess. This is called when the proxy
             * has started, or if NX authentication
             * failed. Otherwise, this->session.parseSSH() is
             * used.
             */
            string parseSSH (string message);

            /*!
             * Read through the nx session file, and if we find a
             * message saying "Session: Terminating session at
             * 'some date'" we need to set isFinished to true.
             */
            //void checkSession (void);

            /*!
             * Re-set the contents of this->session.sessionData
             * with the nth choice.
             *
             * \return true if the nth session is resumable, false
             * if not, or if there is no nth session.
             */
            bool chooseResumable (int n); 

            /*!
             * Re-set the contents of this->session.sessionData
             * with the nth choice such that a terminate session
             * message will be sent to the nxserver
             *
             * \return true if the nth session is terminatable, false
             * if not, or if there is no nth session.
             */
            bool terminateSession (int n);

            void runSession (void);

            void startX11 (string resolution, string name);

            bool needX11Probe (void)
            {
                return x11Probe;
            }
            
            // public slots:
            //@{
            void doneAuth (void);
            void loginFailed (void);
            
            void finished (void)
            {
                dbgln ("Finishing up on signal"); this->isFinished = true;
            }

            void readyproxy (void)
            {
                dbgln ("ready for nxproxy"); this->readyForProxy = true;
            }

            void reset (void);
            void processParseStdout (void);
            void processParseStderr (void);

            /*!
             * SSH requests confirmation to go ahead with
             * connecting (e.g. if you haven't connected to the
             * host before)
             */
            void requestConfirmation (string msg);
            //@}

            // Accessors
            //@{
            /*!
             *  Set the username for NX to log in with
             */
            void setUsername (string& user)
            {
                this->nxuser = user;
                this->session.setUsername (this->nxuser);
            }

            /*!
             *  Set the password for NX to log in with
             */
            void setPassword (string& pass)
            {
                this->nxpass = pass;
                this->session.setPassword (this->nxpass);
            }

            void setResolution (int x, int y)
            {
                this->session.setResolution(x, y);
            }

            void setDepth (int depth)
            {
                this->session.setDepth(depth);
            }

            void setRender (bool render)
            {
                this->session.setRender(render);
            }

            void setSessionData (NXSessionData *);

            notQProcess* getNXSSHProcess (void)
            {
                return this->nxsshProcess;
            }

            notQProcess* getNXProxyProcess (void)
            {
                return this->nxproxyProcess;
            }

            notQProcess* getX11Process (void)
            {
                return this->x11Process;
            }

            notQProcess* getNXAuthProcess (void)
            {
                return this->nxauthProcess;
            }

            bool getIsFinished (void)
            {
                return this->isFinished;
            }

            bool getReadyForProxy (void)
            {
                return this->readyForProxy;
            }

            NXSession* getSession (void)
            {
                return &this->session;
            }

            void setIsFinished (bool status)
            {
                this->isFinished = status;
            }

            void setExternalCallbacks (NXClientLibExternalCallbacks * cb)
            {
                this->externalCallbacks = cb;
            }

            bool getSessionRunning (void)
            {
                return this->sessionRunning;
            }
            //@}

        private:
            /*!
             * Try a number of different paths to try to find the
             * program prog's full path.
             *
             * \param prog The program to find, likely to be nxssh
             * or nxproxy.
             *
             * \return The full path; e.g. /usr/bin/nxssh
             */
            string getPath (string prog);

            /*!
             * Custom search path
             */
            string customPath;

            bool x11Probe;
            /*!
             * Set true when the program is ready to end, e.g if
             * authentication failed, nxssh failed to start amoung
             * other reasons.
             */
            bool isFinished;
            /*!
             * Set true when nxssh is ready to launch the nxproxy process.
             */
            bool readyForProxy;
            /*!
             * Set true when the NX session is under way. This
             * means we can reduce the polling frequency right
             * down to a level which won't impact on power
             * consumption.
             */
            bool sessionRunning;
            /*!
             * Have we said we need to enter a password?
             */
            bool password;

            // FIXME: I hold the actual data, and a pointer to the
            // data here. I tried to get rid of the pointer, and
            // modify main.cpp in ../nxcl and that didn't work
            // properly - I'm not sure why. I suppose I could get
            // rid of the objects here, and then call
            // pNxsshProcess = new notQProcess; in the
            // constructor...
            /*!
             * The nxssh process object
             */
            notQProcess* nxsshProcess;
            /*!
             * The nxproxy process object
             */
            notQProcess* nxproxyProcess;
            /*!
             * The X11 process object
             */
            notQProcess* x11Process;
            /*!
             * The nxauth process object
             */
            notQProcess* nxauthProcess;
            /*!
             * A callbacks object. This holds the various callback
             * methods. The callback methods are defined here, but
             * are callable from notQProcess etc.
             */
            NXClientLibCallbacks callbacks;
            /*!
             * A temporary file to hold the ssl key
             */
            notQTemporaryFile *keyFile;
            /*!
             * The NX Session object.
             */
            NXSession session;
            /*!
             * A structure holding information about the
             * connection to be made, such as server address, port
             * and id.
             */
            ProxyData proxyData;
            /*!
             * Username for the connection
             */
            string nxuser;
            /*!
             * Password for the connection
             */
            string nxpass;
    };

} // namespace
#endif
