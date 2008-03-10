/***************************************************************************
         libtest: A VERY simple command line test for the NXCL library
                             -------------------
    begin                : June 2007
    copyright            : (C) 2007 Embedded Software Foundry Ltd. (U.K.)
                         :     Author: Sebastian James
    email                : seb@esfnet.co.uk
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

/*
 * NB NB! This probably doesn't work. It shouldn't be hard to fix
 * though. It will be broken because I changed the signal handler
 * scheme. Seb.
 */

#include "nxclientlib.h"
#include <fstream>

using namespace nxcl;
using namespace std;

ofstream debugLogFile;

// Signal handlers
void writeOut (string msg)
{
	cout << "NXCL> " << msg << endl;
}
void stdinInfo (string msg)
{
	cout << "nxssh<-stdin-" << endl << msg << endl;
}
void stdoutInfo (string msg)
{
	cout << "nxssh-stdout->" << endl << msg << endl;
}
void stderrInfo (string msg)
{
	cout << "nxssh-stderr->" << endl << msg << endl;
}

int main (int argc, char **argv)
{
	NXClientLib lib;
	stringstream ss;

	ss << argv[2];
	string un = ss.str();
	ss.str("");
	ss << argv[3];
	string pw = ss.str();
	ss.str("");

	if (argc!=4) {
		cout << "Usage: libtest IP user pass" << endl;
		return -1;
	}

	debugLogFile.open("/tmp/libtest.log", ios::out|ios::trunc);

	lib.invokeNXSSH("default", argv[1], true, "", 22);
	lib.setUsername(un);
	lib.setPassword(pw);
	lib.setResolution(1280,1024); // This is the size of your screen
	lib.setDepth(24);
	lib.setRender(true);

	NXSessionData theSesh;

	// HARDCODED TEST CASE
	theSesh.sessionName = "TEST";
	theSesh.sessionType = "unix-gnome";
	theSesh.cache = 8;
	theSesh.images = 32;
	theSesh.linkType = "adsl";
	theSesh.render = true;
	theSesh.backingstore = "when_requested";
	theSesh.imageCompressionMethod = 2;
	// theSesh.imageCompressionLevel;
	theSesh.geometry = "800x600+0+0"; // This'll be the size of the session
	theSesh.keyboard = "defkeymap";
	theSesh.kbtype = "pc102/defkeymap";
	theSesh.media = false;
	theSesh.agentServer = "";
	theSesh.agentUser = "";
	theSesh.agentPass = "";
	theSesh.cups = 0;
	theSesh.suspended = false;
	theSesh.fullscreen = true; // If true, theSesh.geometry is ignored

	lib.setSessionData(&theSesh);

	// Set the handler you would like to output messages to the user. We'll just use stdout for this test.
	//lib.callbackWriteSignal.connect (&writeOut);

	// If you want a nice quiet session, leave these signals unconnected
	/*
	lib.stdinSignal.connect (&stdinInfo);
	lib.stdoutSignal.connect (&stdoutInfo);
	lib.stderrSignal.connect (&stderrInfo);
	*/

	//notQProcess& p = lib.getNXSSHProcess();
	notQProcess* p;

	while ((lib.getIsFinished()) == false) {
		// We need to repeatedly check if there is any output to parse.
		if (lib.getReadyForProxy() == false) {
			p = lib.getNXSSHProcess();
			p->probeProcess();
		} else {
			p = lib.getNXSSHProcess();
			p->probeProcess();
			p = lib.getNXProxyProcess();
			p->probeProcess();
		}

		usleep (1000);
	}

	writeOut ("Program finished.");

	debugLogFile.close();

	return 0;
}
