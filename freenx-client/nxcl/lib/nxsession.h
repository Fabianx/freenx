/***************************************************************************
                                 nxsession.h
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

#ifndef _NXSESSION_H_
#define _NXSESSION_H_

#include <sstream>
#include <fstream>
#include <fcntl.h>
#include <unistd.h>
#include "nxdata.h"
#include <list>

namespace nxcl {

    /*!
     * Virtual Callback class. These callbacks are called based on
     * the output which we get from the nxssh process.
     */
    class NXSessionCallbacks
    {
        public:
            NXSessionCallbacks() {}
            virtual ~NXSessionCallbacks() {}
            virtual void noSessionsSignal (void) {}
            virtual void loginFailedSignal (void) {}
            virtual void readyForProxySignal (void) {}
            /*!
             * Emitted when the initial public key authentication
             * is successful 
             */
            virtual void authenticatedSignal (void) {}
            virtual void sessionsSignal (list<NXResumeData>) {}
    };

    /*!
     * This class is used to parse the output from the nxssh
     * session to the server.
     */
    class NXSession
    {
        public:
            NXSession();
            ~NXSession();

            string parseSSH (string);
            int parseResponse (string);
            void parseResumeSessions (list<string>);
            void resetSession (void);
            void wipeSessions (void);
            bool chooseResumable (int n);
            bool terminateSession (int n);
            string generateCookie (void);
            void runSession (void) { sessionDataSet = true; }

            /*!
             * Accessors
             */
            //@{
            void setUsername (string& user) { nxUsername = user; }
            void setPassword (string& pass) { nxPassword = pass; }
            void setResolution (int x, int y) 
            {
                this->sessionData->xRes = x;
                this->sessionData->yRes = y;
            }

            void setDepth (int d) 
            {
                this->sessionData->depth = d;
            }

            void setRender (bool isRender)
            {
                if (this->sessionDataSet) {
                    this->sessionData->render = isRender;
                }
            }

            void setEncryption (bool enc)
            {
                if (this->sessionDataSet) {
                    this->sessionData->encryption = enc;
                }
            }

            void setContinue (bool allow)
            {
                doSSH = allow;
            }

            void setSessionData (NXSessionData*);

            NXSessionData* getSessionData()
            {
                return this->sessionData;
            }

            bool getSessionDataSet (void)
            {
                return this->sessionDataSet;
            }

            void setCallbacks (NXSessionCallbacks * cb) 
            {
                this->callbacks = cb;
            }
            //@}

        private:
            void reset (void);
            void fillRand(unsigned char *, size_t);

            /*!
             * This is the answer to give to the ssh server if it
             * asks whether we want to continue (say, if we're
             * connecting for the first time and we don't
             * necessarily trust its SSL key).
             */
            bool doSSH;
            /*!
             * Set to true if there are suspended sessions on the
             * server which are owned by nxUsername.
             */
            bool suspendedSessions;
            /*!
             * Set to true of sessionData has been populated
             */
            bool sessionDataSet;
            /*!
             * Holds the stage of the process which we have
             * reached as we go through the process of
             * authenticating with the NX Server.
             */
            int stage;
            /*!
             * File descriptor for the random number device
             */
            int devurand_fd;
            /*!
             * Holds the username for this session
             */
            string nxUsername;
            /*!
             * Holds the password for this session
             */
            string nxPassword;
            /*!
             * A list of sessions which can be resumed, as strings.
             */
            list<string> resumeSessions;
            /*!
             * A list of running sessions, held as NXResumeData
             * structures.
             */
            list<NXResumeData> runningSessions;
            /*!
             * Data for this session.
             */
            NXSessionData *sessionData;
            /*!
             * Pointer to a class containing callback methods.
             */
            NXSessionCallbacks * callbacks;
    };

} // namespace
#endif
