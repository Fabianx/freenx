/***************************************************************************
         nxcmd: A simple command line test for the NXCL dbus daemon.
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

/*!
 * See ../nxcl/main.cpp for general notes on the nxcl program.
 *
 * This program will fork a process and run nxcl, then it will send
 * user, password and some settings to nxcl using a dbus
 * connection. nxcl will then start up the NX connection for you.
 *
 * This is a very hacked together program, part C, part C++, but it
 * serves its purpose of demonstrating the techniques you'll need to
 * write a simple client to nxcl.
 */

#include <iostream>
#include <sstream>

extern "C" {
#include <stdlib.h>
#include <stdio.h>
#define DBUS_API_SUBJECT_TO_CHANGE 1
#include <dbus/dbus.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
}

// This is the only dependency on nxcl here. If you want, all you have
// to do is copy the NXConfigData structure out of nxdata.h and you
// remove the dependency.
#include "../lib/nxdata.h"

using namespace std;

// Default NoMachine certificate for FALLBACK. Don't do it this way in
// your code. Instead, ship a default key in a file and reference
// that.
string cert("-----BEGIN DSA PRIVATE KEY-----\nMIIBuwIBAAKBgQCXv9AzQXjxvXWC1qu3CdEqskX9YomTfyG865gb4D02ZwWuRU/9\nC3I9/bEWLdaWgJYXIcFJsMCIkmWjjeSZyTmeoypI1iLifTHUxn3b7WNWi8AzKcVF\naBsBGiljsop9NiD1mEpA0G+nHHrhvTXz7pUvYrsrXcdMyM6rxqn77nbbnwIVALCi\nxFdHZADw5KAVZI7r6QatEkqLAoGBAI4L1TQGFkq5xQ/nIIciW8setAAIyrcWdK/z\n5/ZPeELdq70KDJxoLf81NL/8uIc4PoNyTRJjtT3R4f8Az1TsZWeh2+ReCEJxDWgG\nfbk2YhRqoQTtXPFsI4qvzBWct42WonWqyyb1bPBHk+JmXFscJu5yFQ+JUVNsENpY\n+Gkz3HqTAoGANlgcCuA4wrC+3Cic9CFkqiwO/Rn1vk8dvGuEQqFJ6f6LVfPfRTfa\nQU7TGVLk2CzY4dasrwxJ1f6FsT8DHTNGnxELPKRuLstGrFY/PR7KeafeFZDf+fJ3\nmbX5nxrld3wi5titTnX+8s4IKv29HJguPvOK/SI7cjzA+SqNfD7qEo8CFDIm1xRf\n8xAPsSKs6yZ6j1FNklfu\n-----END DSA PRIVATE KEY-----");


// Used in receiveSession as the return value
#define REPLY_REQUIRED  1
#define NEW_CONNECTION  2
#define SERVER_CAPACITY 3

/*!
 * Send all the settings in a single dbus signal.
 */
static int sendSettings (DBusConnection *bus, nxcl::NXConfigData& cfg);

/*!
 * Listen to the dbus, waiting for a signal to say either that
 * connection is in progress, or giving us a list of possible sessions
 * we could connect to. Return true if nxcl requires a reply such
 * as "please resume session 1".
 */
static int receiveSession (DBusConnection* conn);

/*!
 * Send a signal containing the identifier for the NX session that the
 * user would like to start.
 */
static int sendReply (DBusConnection *bus, int sessionNum);

/*!
 * Send a signal containing the identifier for the NX session that the
 * user would like to terminate.
 */
static int terminateSession (DBusConnection *bus, int sessionNum);

/*!
 * Lazily use some globals, as this is just an example command line program.
 */
string dbusSendInterface;
string dbusRecvInterface;
string dbusMatchString;


int
main (int argc, char **argv)
{
	nxcl::NXConfigData cfg;
	DBusConnection *conn;
	DBusError error;
	int ret;
	int theError;
	pid_t pid;
	bool gotName = false;
	int i = 0;

	if (argc != 5) {
		cout << "NXCMD> Usage: nxcmd IP/DNSName user pass sessiontype\n";
		cout << "NXCMD> Eg:    nxcmd 192.168.0.1 me mypass unix-gnome\n";
		return -1;
	}

	cout << "NXCMD> Starting...\n";

	/* Get a connection to the session bus */
	dbus_error_init (&error);
	conn = dbus_bus_get (DBUS_BUS_SESSION, &error);
	if (!conn) {
		cerr << "Failed to connect to the D-BUS daemon: " << error.message << endl;
		dbus_error_free (&error);
		return 1;
	}

	while (gotName == false) {

		stringstream ss;
		string base = "org.freenx.nxcl.";
		ss << base << "client" << i;
		dbusSendInterface = ss.str();

		ss.str("");
		ss << base << "nxcl" << i;
		dbusRecvInterface = ss.str();

		ss.str("");
		ss << "type='signal',interface='org.freenx.nxcl.nxcl" << i << "'";
		dbusMatchString = ss.str();


		// We request a name on the bus which is the same string as the send interface.
		ret = dbus_bus_request_name (conn, dbusSendInterface.c_str(),
					     DBUS_NAME_FLAG_REPLACE_EXISTING, 
					     &error);

		if (dbus_error_is_set(&error)) { 
			cerr << "NXCMD> Name Error (" << error.message << ")\n"; 
			dbus_error_free(&error);
		}

		if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret) {
			cerr << "NXCMD> Couldn't get the name; trying another\n";
			i++;
		} else {
			cout << "NXCMD> Got the name '" << dbusSendInterface << "' on the dbus\n";
			gotName = true;
		}
	}

	stringstream arg;
	arg << i;		

	/* fork and exec the nxcl */
	pid = fork();
	switch (pid) {
	case -1:
		cerr << "Can't fork()!" << endl;
		exit (-1);
	case 0:
		// This is the CHILD process
		// Allocate memory for the program arguments
		// 1+ to allow space for NULL terminating pointer
		execl (PACKAGE_BIN_DIR"/nxcl", "nxcl", arg.str().c_str(), static_cast<char*>(NULL));
		// If process returns, error occurred
		theError = errno; 
		// This'll get picked up by parseResponse
		cerr << "NXCMD> Process error: " << pid << " crashed. errno:" << theError << endl;
		// Child should exit now.
		exit(-1);

	default:
		// This is the PARENT process
		cout << "NXCMD> forked the nxcl process; continuing.\n";
		break;
	}


	/* Prepare interface - add a rule for which messages we want
	   to see - those that are sent to us from the nxcl
	   connection. */
	dbus_bus_add_match(conn, dbusMatchString.c_str(), &error);
	dbus_connection_flush (conn);
	if (dbus_error_is_set(&error)) { 
		cerr << "NXCMD> Match Error (" << error.message << ")\n";
		exit(1);
	} else {
		cout << "NXCMD> Added match '" << dbusMatchString << "'\n";
	}

	// Crude 2 second wait to let nxcl get going before we send
	// the settings. This _must_ be more sophisticated in your
	// application ;)
	sleep (2);

	cout << "NXCMD> Configure session\n";

	// We now set up the config data structure. We take user, host
	// and pass from the command line, but for the sake of
	// simplicity, we hard-code the rest of the settings here.
	cfg.serverHost = argv[1];
	cfg.serverPort = 22;
	cfg.sessionUser = argv[2];
	cfg.sessionPass = argv[3];
	cfg.sessionName = "nxcmd";
	cfg.sessionType = argv[4];
	cfg.cache = 8;
	cfg.images = 32;
	cfg.linkType = "adsl";
	cfg.render = true;
	cfg.backingstore = "when_requested";
	cfg.imageCompressionMethod = 2;
	cfg.imageCompressionLevel = 1;
	cfg.geometry = "800x600+0+0";
	cfg.keyboard = "defkeymap";
	cfg.kbtype = "pc105/defkeymap";
	cfg.media = false;
	cfg.agentServer = "";
	cfg.agentUser = "";
	cfg.agentPass = "";
	cfg.cups = 0;
	cfg.encryption = true;
	cfg.key = cert;
	cfg.fullscreen = false; // If true, ignore cfg.geometry. 	
	
	cout << "NXCMD> Sending settings\n";

	// ...and then send it on dbus.
	sendSettings (conn, cfg);

	cout << "NXCMD> Sent settings\n";
	
	// In real application, we'd now listen on the dbus for a list
	// of sessions we might be able to re-connect to, or
	// confirmation that a session is starting. Then we could return.
	bool done = false;
	while (false == done) {
		if (REPLY_REQUIRED == (ret = receiveSession (conn))) {
			// Need to send back a reply saying which connection
			// to reconnect.
			cout << "NXCMD> Please choose a session. 0 for the first, etc.\nT0 to terminate the first, etc\n";
			if (cfg.sessionType != "shadow") {
				cout << "Enter -1 to start a new session\n";
			}
			// Choose 0 for first connection, 1 for next etc, or
			// <0 for a new connection, even if there are existing
			// sessions.
			int connNum = 0;
			string response;
			cin >> response;
			stringstream ss;
			if (response[0] == 'T') {
				ss << response.substr(1);
				ss >> connNum;
				cout << "NXCMD> Terminating session " << connNum << endl;
				terminateSession (conn, connNum);
			} else {
				ss << response;
				ss >> connNum;
				sendReply (conn, connNum);
				if (connNum < 0) {
					cout << "NXCMD> Starting new NX session\n";			
				} else {
					cout << "NXCMD> Attaching to session " << connNum << endl;
				}
				done = true;
			}

		} else if (SERVER_CAPACITY == ret) {
			// Have run out of capacity (licences) on the server.
			done = true;
		} else {
			cout << "NXCMD> Starting new NX session\n";
			done = true;
		}
	}

	// wait and block for nxcl process to end before exiting.
	int status = 0;
	wait (&status);

	return 0;
}

static int
sendSettings (DBusConnection *bus, nxcl::NXConfigData& cfg)
{
	cout << "NXCMD> sendSettings called\n";

	DBusMessage *message;

	/* Create a new setting signal on the "org.freenx.nxcl.client"
	 * interface, from the object "/org/freenx/nxcl/dbus/settingSignal". */
	message = dbus_message_new_signal ("/org/freenx/nxcl/dbus/settingSignal",
					   dbusSendInterface.c_str(), 
					   "sessionConfig");
	if (NULL == message) {
		cerr << "NXCMD> Message Null\n";
		return -1;
	}

	int media=0, enc=0, fs=0;
	if (cfg.media == true) { media = 1; }
	if (cfg.encryption == true) { enc = 1; }
	if (cfg.fullscreen == true) { fs = 1; }
	dbus_message_append_args 
		(message,
		 DBUS_TYPE_STRING, &cfg.serverHost,  //0
		 DBUS_TYPE_INT32,  &cfg.serverPort,
		 DBUS_TYPE_STRING, &cfg.sessionUser, //2
		 DBUS_TYPE_STRING, &cfg.sessionPass,
		 DBUS_TYPE_STRING, &cfg.sessionName, //4
		 DBUS_TYPE_STRING, &cfg.sessionType,
		 DBUS_TYPE_INT32,  &cfg.cache,       //6
		 DBUS_TYPE_INT32,  &cfg.images,
		 DBUS_TYPE_STRING, &cfg.linkType,    //8
		 DBUS_TYPE_INT32,  &cfg.render, 
		 DBUS_TYPE_STRING, &cfg.backingstore,//10
		 DBUS_TYPE_INT32,  &cfg.imageCompressionMethod,
		 DBUS_TYPE_INT32,  &cfg.imageCompressionLevel, //12
		 DBUS_TYPE_STRING, &cfg.geometry,
		 DBUS_TYPE_STRING, &cfg.keyboard,    //14
		 DBUS_TYPE_STRING, &cfg.kbtype,
		 DBUS_TYPE_INT32,  &media,           //16
		 DBUS_TYPE_STRING, &cfg.agentServer,
		 DBUS_TYPE_STRING, &cfg.agentUser,   //18
		 DBUS_TYPE_STRING, &cfg.agentPass,
		 DBUS_TYPE_INT32,  &cfg.cups,        //20
		 DBUS_TYPE_STRING, &cfg.key,
		 DBUS_TYPE_INT32,  &enc,             //22
		 DBUS_TYPE_INT32,  &fs,
		 DBUS_TYPE_STRING, &cfg.customCommand,//24
		 DBUS_TYPE_INVALID);

	/* Send the signal */
	if (!dbus_connection_send (bus, message, NULL)) {
		cerr << "NXCMD> Out Of Memory!\n"; 
		exit(1);
	}

	/* Clean up */
	dbus_message_unref (message);
	dbus_connection_flush (bus);

	return 1;
}

static int
sendReply (DBusConnection *bus, int sessionNum)
{
	DBusMessage *message;
	DBusMessageIter args;

	/* Create a new session choice signal on the "org.freenx.nxcl.client"
	 * interface, from the object "/org/freenx/nxcl/dbus/sessionChoice". */
	message = dbus_message_new_signal ("/org/freenx/nxcl/dbus/sessionChoice",
					   dbusSendInterface.c_str(),
					   "sessionChoice");
	if (NULL == message) { 
		cerr << "NXCMD> Message Null\n";
		return -1;
	}

	dbus_message_iter_init_append (message, &args);
	if (!dbus_message_iter_append_basic (&args,
					     DBUS_TYPE_INT32,
					     (const void*)&sessionNum)) {
		cerr << "NXCMD> Out Of Memory!\n";
		exit(1);
	}

	/* Send the signal */
	if (!dbus_connection_send (bus, message, NULL)) {
		cerr << "NXCMD> Out Of Memory!\n";
		exit(1);	  
	}

	/* Clean up */
	dbus_message_unref (message);
	dbus_connection_flush (bus);

	return 1;
}


static int
terminateSession (DBusConnection *bus, int sessionNum)
{
	DBusMessage *message;
	DBusMessageIter args;

	/* Create a new session choice signal on the "org.freenx.nxcl.client"
	 * interface, from the object "/org/freenx/nxcl/dbus/sessionChoice". */
	message = dbus_message_new_signal ("/org/freenx/nxcl/dbus/sessionChoice",
					   dbusSendInterface.c_str(),
					   "terminateSession");
	if (NULL == message) { 
		cerr << "NXCMD> Message Null\n";
		return -1;
	}

	dbus_message_iter_init_append (message, &args);
	if (!dbus_message_iter_append_basic (&args,
					     DBUS_TYPE_INT32,
					     (const void*)&sessionNum)) {
		cerr << "NXCMD> Out Of Memory!\n";
		exit(1);
	}

	/* Send the signal */
	if (!dbus_connection_send (bus, message, NULL)) {
		cerr << "NXCMD> Out Of Memory!\n";
		exit(1);	  
	}

	/* Clean up */
	dbus_message_unref (message);
	dbus_connection_flush (bus);

	return 1;
}


/*
 * Wait for and receive a message with available sessions, or a
 * message saying there are no available sessions
 */
static int
receiveSession (DBusConnection* conn)
{
	DBusMessage * message;
	DBusMessageIter args;
	DBusError error;
	char * parameter = NULL;
	dbus_int32_t iparam = 0, t = 0;
	int count = 0;
	bool sessions_obtained = false;
	int rtn = 0;
	stringstream ss;

	cout << "NXCMD> In receiveSession, listening to " << dbusRecvInterface << "\n";

	dbus_error_init (&error);

	int sessionNum = 0;
	// loop listening for signals being emitted
	while (sessions_obtained == false) {
		
		if (dbus_error_is_set(&error)) { 
			cerr << "NXCMD> Name Error (" << error.message << ")\n"; 
			dbus_error_free(&error); 
		}

		count = 0;

		// non blocking read of the next available message
		dbus_connection_read_write(conn, 0);
		message = dbus_connection_pop_message(conn);

		// loop again if we haven't read a message
		if (NULL == message) {
			sleep (1);
			continue;
		}

		// check if the message is a signal from the
		// correct interface and with the correct name
		if (dbus_message_is_signal (message,
					    dbusRecvInterface.c_str(),
					    "AvailableSession")) {

			if (!dbus_message_iter_init (message, &args)) {
				cerr << "NXCMD> Message has no arguments!\n";
				dbus_connection_flush (conn);
				dbus_message_unref (message);
				continue;
			}
			
			if (DBUS_TYPE_INT32 != dbus_message_iter_get_arg_type (&args)) {
				cerr << "NXCMD> First argument is not int32!\n";
			} else {
				dbus_message_iter_get_basic (&args, &iparam);
				if (sessionNum == 0) {
					cout << "NXCMD> Available sessions:\n";
				}
				cout << sessionNum++ << ": " << iparam;
				count++;
			}

			// read the parameters - something like:
			while (dbus_message_iter_next (&args)) {
				if (DBUS_TYPE_STRING == (t = dbus_message_iter_get_arg_type(&args))) {
					dbus_message_iter_get_basic(&args, &parameter);
					cout << " " << parameter;
					ss.str("");
					ss << parameter;
					count++;
				} else if (t == DBUS_TYPE_INT32) {
					dbus_message_iter_get_basic (&args, &iparam);
					cout << " " << iparam;
					ss.str("");
					ss << iparam;
					count++;

				} else {
					cerr << "NXCMD> Error, parameter is not string or int.\n";
				}
				// Now we'd do something sensible with the data...
			}
			cout << endl;

		} else if (dbus_message_is_signal (message,
						   dbusRecvInterface.c_str(),
						   "NoMoreAvailable")) {
			sessions_obtained = true;
			rtn = REPLY_REQUIRED;

		} else if (dbus_message_is_signal (message,
						   dbusRecvInterface.c_str(),
						   "Connecting")) {
			sessions_obtained = true;
			rtn = NEW_CONNECTION;

		} else if (dbus_message_is_signal (message,
						   dbusRecvInterface.c_str(),
						   "ServerCapacityReached")) {
			sessions_obtained = true;
			rtn = SERVER_CAPACITY;
		} else {
#ifdef DEBUG
			cout << "NXCMD> None of the above...\n";
			if (!dbus_message_iter_init(message, &args)) {
				cerr << "NXCMD> Message has no arguments!\n";
				dbus_connection_flush (conn);
				dbus_message_unref (message);
				continue;
			}
			if (DBUS_TYPE_STRING == (t = dbus_message_iter_get_arg_type(&args))) {
				dbus_message_iter_get_basic(&args, &parameter);
				cout << "NXCMD> Parameter: " << parameter << endl;
			}
#endif
		}	

		dbus_connection_flush (conn);
		dbus_message_unref (message);

	} // while()

	return rtn;
}
