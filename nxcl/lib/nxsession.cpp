/***************************************************************************
                                nxsession.cpp
                             -------------------
    begin                : Wed 26th July 2006
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

// Enumerated type defining the stages through which the client goes when
// connecting
enum { HELLO_NXCLIENT,
    ACKNOWLEDGE,
    SHELL_MODE,
    AUTH_MODE,
    LOGIN,
    LIST_SESSIONS,
    PARSESESSIONS,
    STARTSESSION,
    FINISHED };

#define CLIENT_VERSION "3.0.0"

#include <iostream>

#include "notQt.h"
#include "nxclientlib.h"
#include "nxsession.h"

using namespace std;
using namespace nxcl;

NXSession::NXSession() :
    devurand_fd(-1),
    stage(HELLO_NXCLIENT),
    sessionDataSet(false),
    nxUsername("nouser"),
    nxPassword("nopass")
{
}

NXSession::~NXSession()
{
}

void NXSession::resetSession()
{
    this->stage = 0;
    this->sessionDataSet = false;
}

string NXSession::parseSSH(string message)
{
    dbgln ("NXSession::parseSSH called for: " + message);

    int response = parseResponse (message);
    string returnMessage;

    if (response == 211) {
        if (doSSH == true) {
            returnMessage = "yes";
            doSSH = false;
        } else
            returnMessage = "no";
    }

    if (response == 204) { // Authentication failed
        returnMessage = "204";
    }

    if (response == 147) { // Server capacity reached
        returnMessage = "147";
        this->stage = FINISHED;
    }

    switch (this->stage) {
        case HELLO_NXCLIENT:
            dbgln ("HELLO_NXCLIENT stage");

            if (message.find("HELLO NXSERVER - Version") != string::npos) {
                this->callbacks->authenticatedSignal();
                returnMessage = "hello NXCLIENT - Version ";
                returnMessage.append(CLIENT_VERSION);
                this->stage++;
            }

            break;
        case ACKNOWLEDGE:
            dbgln ("ACKNOWLEDGE stage");

            if (response == 105)
                this->stage++;

            break;
        case SHELL_MODE:
            dbgln ("SHELL_MODE stage");

            if (response == 105) {
                returnMessage = "SET SHELL_MODE SHELL";
                this->stage++;
            }

            break;
        case AUTH_MODE:
            dbgln ("AUTH_MODE stage");

            if (response == 105) {
                returnMessage = "SET AUTH_MODE PASSWORD";
                this->stage++;
            }

            break;
        case LOGIN:
            dbgln ("LOGIN stage");

            switch (response) {
                case 105:
                    returnMessage = "login";
                    break;
                case 101:
                    returnMessage = nxUsername;
                    break;
                case 102:
                    returnMessage = nxPassword;
                    break;
                case 103:
                    this->stage++;
                    break;
                case 404:
                    this->callbacks->loginFailedSignal();
            }

            break;
        case LIST_SESSIONS:
            dbgln ("LIST_SESSIONS stage");

            if (this->sessionData->terminate == true) {
                // Wait for termination
                dbgln ("Waiting for termination");

                if (response == 900) {
                    stringstream termsession;

                    termsession << "NX> 900 Session id: "
                        << this->sessionData->id
                        << " terminated.";

                    if (message.find (termsession.str().c_str(), 0) == 0) {
                        // Session terminated.
                        this->sessionData->terminate = false;
                    } else {
                        usleep (10000);
                    }
                }

            } else if (response == 105) {
                // Get a list of the available sessions on the server, for
                // given user, with given status, and any type. Not sure if
                // geometry is ignored or not.
                stringstream ss;

                // We want to list suspended or running sessions, with this
                // command:
                dbgln ("this->resumeSessions.size() == "
                        << this->resumeSessions.size());

                if (this->sessionData->sessionType == "shadow") {
                    // This is how to list shadow sessions. Run NoMachine's
                    // client and see ~/.nx/temp/(pid)/sshlog for connection
                    // details
                    ss << "listsession --type=\"shadow\"";

                } else {

                    ss << "listsession --user=\"" << nxUsername
                        << "\" --status=\"suspended,running\" --geometry=\""
                        << this->sessionData->xRes << "x"
                        << this->sessionData->yRes << "x"
                        << this->sessionData->depth
                        << (this->sessionData->render ? "+render" : "")

                        // If you leave --type blank, you can re-connect to any
                        // sessions available.
                        << "\" --type=\"" << this->sessionData->sessionType
                        << "\"";
                }

                returnMessage = ss.str();
                this->stage++;
            }
            break;
        case PARSESESSIONS:
            dbgln ("PARSESESSIONS stage");

            if ((this->sessionData->sessionType == "shadow" &&
                        response != 105) ||
                    (this->sessionData->sessionType != "shadow" &&
                        response != 148)  ) {

                dbgln ("Building resumeSessions:"
                        << " resumeSessions.push_back(message);");

                this->resumeSessions.push_back(message);

            } else if ((this->sessionData->sessionType == "shadow" &&
                        response == 105)
                    || (this->sessionData->sessionType != "shadow" &&
                        response == 148)) {

                dbgln ("Parsing resumeSessions:"
                        << " parseResumeSessions(resumeSessions);");

                parseResumeSessions (this->resumeSessions);

                dbgln ("parseResumeSessions(resumeSessions) returned");

                // Now, the problem we have here, is that when
                // we return from the last 105 response, we
                // don't then get another stdout message to
                // act upon. So, we want to recurse back into
                // parseSSH to get onto the STARTSESSION stage here:
                returnMessage = this->parseSSH (message);
            }
            break;

        case STARTSESSION:
            dbgln ("STARTSESSION stage");
            if (response == 105 && sessionDataSet) {

                dbgln ("response is 105 and sessionDataSet is true");;
                int media = 0;
                string fullscreen = "";
                if (this->sessionData->media) {
                    media = 1;
                }

                if (this->sessionData->fullscreen) {
                    this->sessionData->geometry = "fullscreen";
                    fullscreen = "+fullscreen";
                }

                if (this->sessionData->sessionType == "shadow" &&
                        this->sessionData->terminate == false) {

                    dbgln ("It's a shadow session!");
                    stringstream ss;

                    // These are the session parameters that NoMachine's client
                    // sends for resume

                    ss << "attachsession "
                        << "--link=\"" << this->sessionData->linkType << "\" "
                        << "--backingstore=\""
                            << this->sessionData->backingstore << "\" "
                        << "--encryption=\"" << this->sessionData->encryption
                            << "\" "
                        << "--cache=\"" << this->sessionData->cache << "M\" "
                        << "--images=\"" << this->sessionData->images << "M\" "
                        // probably has been autodetected from my display
                        << "--shmem=\"1\" "
                        // probably has been autodetected from my display
                        << "--shpix=\"1\" "
                        // probably has been autodetected from my display
                        << "--strict=\"0\" "
                        // probably has been autodetected from my display
                        << "--composite=\"1\" "
                        << "--media=\"" << media << "\" "
                        << "--session=\"" << this->sessionData->sessionName
                            << "\" "
                        << "--type=\"" << this->sessionData->sessionType
                            << "\" "
                        // FIXME: This may be some other OS if you compile it on
                        // Sun, Windows, etc.
                        << "--client=\"linux\" "
                        << "--keyboard=\"" << this->sessionData->keyboard
                            << "\" "
                        << "--id=\"" << this->sessionData->id << "\" "
                        // This may be the key?
                        << "--display=\"0\" "
                        << "--geometry=\"" << this->sessionData->geometry
                            << "\" ";

                    returnMessage = ss.str();

                    dbgln ("session parameter command: " + ss.str());

                    this->stage++;

                } else if (this->sessionData->terminate == true) {

                    stringstream ss;

                    // These are the session parameters that NoMachine's client
                    // sends for resume
                    ss << "Terminate --sessionid=\"" << this->sessionData->id
                        << "\"";

                    returnMessage = ss.str();

                    dbgln ("session parameter command: " + ss.str());

                    // Back to listsessions after terminating a session.
                    this->stage -= 2;

                    // Clear the list of sessions to resume
                    this->resumeSessions.clear();
                    this->runningSessions.clear();

                } else if (this->sessionData->suspended) {

                    dbgln ("this->sessionData->suspended is true");

                    stringstream ss;

                    // These are the session parameters that NoMachine's client
                    // sends for resume
                    ss << "restoresession --id=\"" << this->sessionData->id <<
                        "\" --session=\"" << this->sessionData->sessionName <<
                        "\" --type=\"" << this->sessionData->sessionType <<
                        "\" --cache=\"" << this->sessionData->cache <<
                        "M\" --images=\"" << this->sessionData->images <<
                        "M\" --cookie=\"" << generateCookie() <<
                        "\" --link=\"" << this->sessionData->linkType <<
                        "\" --kbtype=\"" << this->sessionData->kbtype <<
                        "\" --nodelay=\"1\" --encryption=\""
                            << this->sessionData->encryption <<
                        "\" --backingstore=\""
                            << this->sessionData->backingstore <<
                        "\" --geometry=\"" << this->sessionData->geometry <<
                        "\" --media=\"" << media <<
                        "\" --agent_server=\""
                            << this->sessionData->agentServer <<
                        "\" --agent_user=\"" << this->sessionData->agentUser <<
                        "\" --agent_password=\""
                            << this->sessionData->agentPass << "\"";

                    returnMessage = ss.str();

                    dbgln ("session parameter command: " + ss.str());

                    this->stage++;

                } else {

                    dbgln ("this->sessionData->suspended is false, and it's" <<
                            " not a shadow session.");

                    stringstream ss;

                    ss << "startsession --session=\""
                            << this->sessionData->sessionName

                        << "\" --type=\"" << this->sessionData->sessionType
                        << "\" --cache=\"" << this->sessionData->cache
                        << "M\" --images=\"" << this->sessionData->images
                        << "M\" --cookie=\"" << generateCookie()
                        << "\" --link=\"" << this->sessionData->linkType
                        << "\" --render=\""
                            << (this->sessionData->render ? 1 : 0)

                        << "\" --encryption=\""
                            << this->sessionData->encryption

                        << "\" --backingstore=\""
                            << this->sessionData->backingstore

                        << "\" --imagecompressionmethod=\""
                        << this->sessionData->imageCompressionMethod
                        << "\" --geometry=\"" << this->sessionData->geometry
                        << "\" --screeninfo=\"" << this->sessionData->xRes
                        << "x" << this->sessionData->yRes << "x"
                        << this->sessionData->depth
                        << (this->sessionData->render ? "+render" : "")
                        << fullscreen << "\" --keyboard=\""
                            << this->sessionData->keyboard

                        << "\" --kbtype=\"" << this->sessionData->kbtype
                        << "\" --media=\"" << media
                        << "\" --agent_server=\""
                            << this->sessionData->agentServer

                        << "\" --agent_user=\""
                            << this->sessionData->agentUser

                        << "\" --agent_password=\""
                            << this->sessionData->agentPass

                        << "\"";

                    ss << " --title=\"sebtest\""; // testing a window title

                    if (this->sessionData->sessionType == "unix-application") {
                        ss << " --application=\"" 
                            << this->sessionData->customCommand << "\"";

                        if (this->sessionData->virtualDesktop == true) {
                            ss << " --rootless=\"0\" --virtualdesktop=\"1\"";
                        } else {
                            ss << " --rootless=\"1\" --virtualdesktop=\"0\"";
                        }

                    } else if
                        (this->sessionData->sessionType == "unix-console") {

                        if (this->sessionData->virtualDesktop == true) {
                            ss << " --rootless=\"0\" --virtualdesktop=\"1\"";
                        } else {
                            ss << " --rootless=\"1\" --virtualdesktop=\"0\"";
                        }

                    } else if
                        (this->sessionData->sessionType == "unix-default") {
                        // ignore this - does anyone use it?
                    }

                    returnMessage = ss.str();

                    dbgln ("session parameter command: " + ss.str());

                    this->stage++;
                }
            } else {
                dbgln ("either response is not 105 or sessionDataSet is"
                        << " false.");
            }
            break;

        case FINISHED:
            dbgln ("FINISHED stage. Response is " << response
                    << ". That should mean that session set up is complete.");
            this->callbacks->readyForProxySignal();
    }

    dbgln ("NXSession::parseSSH, about to return a message: " + returnMessage);

    if (!returnMessage.empty()) {
        returnMessage.append("\n");
        return returnMessage;
    } else
        return "";
}

void NXSession::setSessionData (NXSessionData *sd)
{
    this->sessionData = sd;
}

int NXSession::parseResponse(string message)
{
    string::size_type idx1, idx2;

    int response;

    dbgln ("NXSession::parseResponse called for message:" << message);

    if ((idx1 = message.find ("notQProcess error", 0)) != string::npos) {

        dbgln ("Found notQProcess error");

        // This means a process crashed, we're going to return a number >100000
        // to indicate this.
        if ( ((idx2 = message.find ("crashed", 0)) != string::npos) && 
                idx2 > idx1) {

            stringstream ss;
            ss << message.substr((idx1+19), idx2-1-(idx1+19));

            // This is the pid that crashed
            ss >> response;

            // Add 100000 and return this
            response += 100000;

            return response;
        } else {
            dbgln ("Uh oh, didn't find \"crashed\"");
        }
    }

    // Find out the server response number
    // This will only be present in strings which contain "NX>"
    if (message.find("NX>") != string::npos && message.find("NX>") == 0) {
        idx1 = message.find("NX>") + 4;

        if ((idx2 = message.find(" ", idx1)) == string::npos) {
            if ((idx2 = message.find("\n", idx1)) == string::npos) {
                idx2 = message.size();
            }
        }

        if (idx2>idx1) {
            stringstream ss;
            ss << message.substr(idx1, idx2-idx1);
            ss >> response;
        } else {
            response = 0;
        }

    } else {
        response = 0;
    }

    dbgln ("NXSession::parseResponse() returning " << response);
    return response;
}

void NXSession::parseResumeSessions(list<string> rawdata)
{
    // Was: QStringList sessions, and got rawdata appended to it?
    list<string> sessions;
    list<string>::iterator iter, at;

    dbgln ("NXSession::parseResumeSessions called.");

    for (iter = rawdata.begin(); iter != rawdata.end(); iter++) {
        if (((*iter).find("-------") != string::npos) && !(*iter).empty()) {
            at = iter;
        }
    }

    for (iter = ++at; iter != rawdata.end(); iter++) {
        if ((!(*iter).find("NX> 148") != string::npos) && !(*iter).empty()) {
            sessions.push_back(*iter);
        }
    }

    list < vector<string> > rawsessions;
    list < vector<string> >::iterator rsIter;

    // Clean up each string in sessions[i], then push back
    // sessions[i] onto rawsessions., except that means
    // rawsessions is then just a list<string>...
    vector<string> session;
    vector<string>::iterator seshIter;

    for (iter = sessions.begin(); iter != sessions.end(); iter++) {
        session.clear();

        // Simplify one line of list<string> sessions
        (*iter) = notQtUtilities::simplify (*iter); 

        // Split one line of list<string> sessions into a vector<string>
        notQtUtilities::splitString (*iter, ' ', session); 

        // Add that to rawsessions
        rawsessions.push_back(session);
    }

    NXResumeData resData;

    for (rsIter = rawsessions.begin(); rsIter != rawsessions.end(); rsIter++) {
        stringstream ss1, ss2;
        int tmp;

        dbgln ("*rsIter.size() == " << (*rsIter).size());
        ss1 << (*rsIter)[0];
        ss1 >> tmp;

        resData.display = tmp;

        dbgln ("resData.display = " << resData.display);
        resData.sessionType = (*rsIter)[1];

        dbgln ("resData.sessionType = " << resData.sessionType);
        resData.sessionID = (*rsIter)[2];

        dbgln ("resData.sessionID = " << resData.sessionID);
        resData.options = (*rsIter)[3];

        dbgln ("resData.options = " << resData.options);
        ss2 << (*rsIter)[4];
        ss2 >> tmp;

        resData.depth = tmp;

        dbgln ("resData.depth = " << resData.depth);
        resData.screen = (*rsIter)[5];

        dbgln ("resData.screen = " << resData.screen);
        resData.available = (*rsIter)[6];

        dbgln ("resData.available = " << resData.available);
        resData.sessionName = (*rsIter)[7];

        dbgln ("resData.sessionName = " << resData.sessionName);
        this->runningSessions.push_back(resData);
    }

    if (this->runningSessions.size() != 0) {
        this->suspendedSessions = true;

        dbgln ("NXSession::parseResumeSessions(): Calling sessionsSignal.");

        // runningSessions is a list of NXResumeData
        this->callbacks->sessionsSignal (this->runningSessions);
    } else {
        dbgln ("NXSession::parseResumeSessions(): Calling"
                << " this->callbacks->noSessionsSignal()");

        // In case we previously had one resumable session,
        // which the user terminated, then we listsessions and
        // got no resumable sessions, we need to make sure
        // startsession is called, not restoresession. hence
        // set sessionData->suspended to false.
        this->sessionData->suspended = false;
        this->callbacks->noSessionsSignal();
    }

    dbgln ("Increment stage");
    this->stage++;
    dbgln ("NXSession::parseResumeSessions() returning.");
}

void NXSession::wipeSessions()
{
    while (!this->runningSessions.empty()) {
        this->runningSessions.pop_front();
    }
}

string NXSession::generateCookie()
{
    unsigned long long int int1, int2;
    stringstream cookie;

    devurand_fd = open("/dev/urandom", O_RDONLY);

    fillRand((unsigned char*)&int1, sizeof(int1));
    fillRand((unsigned char*)&int2, sizeof(int2));
    cookie << int1 << int2;
    return cookie.str();
}

void NXSession::fillRand(unsigned char *buf, size_t nbytes)
{
    ssize_t r;
    unsigned char *where = buf;

    while (nbytes) {
        while ((r = read(devurand_fd, where, nbytes)) == -1)
            where  += r;
        nbytes -= r;
    }
}

bool NXSession::chooseResumable (int n)
{
    dbgln ("NXSession::chooseResumable called.");
    if (this->runningSessions.size() <= static_cast<unsigned int>(n)) {

        // No nth session to resume.
        dbgln ("No nth session to resume, return false.");
        return false;
    }

    // Set to false while we change contents of sessionData
    this->sessionDataSet = false;

    list<NXResumeData>::iterator it = this->runningSessions.begin();
    for (int i = 0; i<n; i++) { it++; }

    // If it's a shadow session, we don't want to replace "shadow" with
    // X11-local
    if (this->sessionData->sessionType != "shadow") {
        this->sessionData->sessionType = (*it).sessionType;
    }

    this->sessionData->display = (*it).display;
    this->sessionData->sessionName = (*it).sessionName;
    this->sessionData->id = (*it).sessionID;

    stringstream geom;	

    // Plus render, if necessary
    geom << (*it).screen << "x" << (*it).display;

    // FIXME: This not yet quite complete.
    // With depth in there too?
    this->sessionData->geometry = geom.str();
    this->sessionData->suspended=true;

    this->sessionDataSet = true;

    dbgln ("NXSession::chooseResumable returning true.");
    return true;
}

bool NXSession::terminateSession (int n)
{
    dbgln ("NXSession::terminateSession called.");
    if (this->runningSessions.size() <= static_cast<unsigned int>(n)) {

        // No nth session to terminate
        dbgln ("No nth session to terminate, return false.");
        return false;
    }

    // Set to false while we change the contents of sessionData
    this->sessionDataSet = false;

    list<NXResumeData>::iterator it = this->runningSessions.begin();
    for (int i = 0; i<n; i++) { it++; }

    this->sessionData->terminate = true;
    this->sessionData->display = (*it).display;
    this->sessionData->sessionName = (*it).sessionName;
    this->sessionData->id = (*it).sessionID;
    this->sessionData->suspended=true;

    this->sessionDataSet = true;

    return true;
}

