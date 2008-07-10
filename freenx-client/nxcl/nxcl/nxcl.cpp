/***************************************************************************
                        nxcl: The NXCL dbus daemon.
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

#include "../config.h"
#include "nxclientlib_i18n.h"
#include "nxclientlib.h"
#include <fstream>

#include "nxcl.h"

/* This define is required for slightly older versions of dbus as
 * found, for example, in Ubuntu 6.06. */
#define DBUS_API_SUBJECT_TO_CHANGE 1
extern "C" {
#include <dbus/dbus.h>
#include <X11/Xlib.h>
}
#include <stdlib.h>

using namespace nxcl;
using namespace std;


/*!
 * NxclCallbacks class
 */
//@{
NxclCallbacks::NxclCallbacks ()
{
}
NxclCallbacks::~NxclCallbacks ()
{
}
void
NxclCallbacks::write (string msg)
{
	this->parent->sendDbusInfoMsg (msg);
}
void
NxclCallbacks::write (int num, string msg)
{
	this->parent->sendDbusInfoMsg (num, msg);
}
void
NxclCallbacks::error (string msg)
{
	cerr << "NXCL_ERROR> " << msg << endl;
	this->parent->sendDbusErrorMsg (msg);
}
void
NxclCallbacks::debug (string msg)
{
#if DEBUG==1
	cout << "NXCL_DBG> " << msg << endl;
#endif
}
void
NxclCallbacks::stdoutSignal (string msg)
{
#if DEBUG==1
	cout << "nxssh-stdout->" << endl << msg << endl; 
#endif
}
void
NxclCallbacks::stderrSignal (string msg)
{
#if DEBUG==1
	cout << "nxssh-stderr->" << endl << msg << endl; 
#endif
}
void
NxclCallbacks::stdinSignal (string msg)
{
#if DEBUG==1
	cout << "nxssh<-stdin-" << endl << msg << endl;
#endif
}
void
NxclCallbacks::resumeSessionsSignal (list<NXResumeData> data)
{
	this->parent->haveResumableSessions (data);
}
void
NxclCallbacks::noSessionsSignal (void)
{
	this->parent->noResumableSessions();
}
void
NxclCallbacks::serverCapacitySignal (void)
{
	this->parent->serverCapacityReached();
}
//@}

/*!
 * Implementation of the Nxcl class, which manages an NX session on
 * behalf of a calling client, which communicates with this class via
 * a dbus connection.
 */
//@{
Nxcl::Nxcl ()
{
	this->dbusNum = 0;
	this->initiate();
}

Nxcl::Nxcl (int n)
{
	this->dbusNum = n;
	this->initiate();
}


Nxcl::~Nxcl ()
{
}

void
Nxcl::initiate (void)
{
	this->setSessionDefaults();

	this->nxclientlib.setExternalCallbacks (&callbacks);
	this->callbacks.setParent (this);

	// Get the X display information
	Display* display;
	display = XOpenDisplay (getenv("DISPLAY"));
	if (display == NULL) {
		cerr << "Cannot connect to local X server :0\n";
		exit (-1);
	}

	int screenNum = DefaultScreen (display);
	this->xRes = DisplayWidth (display, screenNum);
	this->yRes = DisplayHeight (display, screenNum);
	dbgln ("screen is " << xRes << "x" << yRes);
	unsigned long wp = WhitePixel (display, screenNum);
	unsigned long bp = BlackPixel (display, screenNum);
	unsigned long d = wp-bp+1;
	dbgln ("wp is " << wp << " bp is " << bp << " and difference plus 1 (d) is " << d);
	this->displayDepth = 0;
	while (d > 1) {
		d = d>>1;
		this->displayDepth++;
	}
	dbgln ("displayDepth is " << this->displayDepth << " and d is now " << d);
	XCloseDisplay (display);
}

void
Nxcl::setupDbus (int id)
{
	this->dbusNum = id;
	this->setupDbus();
}

void
Nxcl::setupDbus (void)
{
	/* Get a connection to the session bus */
	dbus_error_init (&this->error);
	this->conn = dbus_bus_get (DBUS_BUS_SESSION, &this->error);
	if (!this->conn) {
		cerr << "Failed to connect to the D-BUS daemon: " 
		     << this->error.message << ". Exiting.\n";
		dbus_error_free (&this->error);
		exit (1);
	}
	stringstream ss;
	ss << "org.freenx.nxcl.nxcl" << this->dbusNum;
	this->dbusName = ss.str();
	int ret = dbus_bus_request_name(this->conn, this->dbusName.c_str(),
					DBUS_NAME_FLAG_REPLACE_EXISTING, 
					&this->error);
	if (dbus_error_is_set(&this->error)) { 
		cerr << "Name Error (" << this->error.message << ")\n";
		dbus_error_free(&this->error); 
	}
	if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret) { 
		/* What to do if someone else is running? Try another name? Exit? */
		this->callbacks.error ("There appears to be another nxcl running, won't compete. Exiting.");
		exit(1);
	}
	// Done getting connection to session bus


	// Prepare interface - add a rule for which messages we want
	// to see. We listen for messages from the _client_
	// connection.
	ss.str("");
	ss << "type='signal',interface='org.freenx.nxcl.client" << this->dbusNum << "'";
	this->dbusMatch = ss.str();

	dbus_bus_add_match(this->conn, this->dbusMatch.c_str(), &this->error);
	dbus_connection_flush (this->conn);
	if (dbus_error_is_set(&this->error)) {
		cerr << "Match Error (" << this->error.message << ")\n";
		exit(1);
	}

	ss.str("");
	ss << "org.freenx.nxcl.client" << this->dbusNum;
	this->dbusMatchInterface = ss.str();
	ss.str("");
	ss << "org.freenx.nxcl.nxcl" << this->dbusNum;
	this->dbusSendInterface = ss.str();

	return;
}


int
Nxcl::receiveSettings (void)
{
	DBusMessage * message;
	DBusMessageIter args;
	char * parameter = NULL;
	bool settings_transferred = false;
	stringstream ss;
	int count = 0;

	this->callbacks.debug ("receiveSettings called");

	// loop listening for signals being emitted
	while (settings_transferred == false) {

		if (dbus_error_is_set(&this->error)) {
			stringstream errmsg;
			errmsg << "receiveSettings(): Got a dbus error '"
			       << this->error.name << "': " << this->error.message;
			this->callbacks.error (errmsg.str());
			dbus_error_free (&this->error);
		}

		// non blocking read of the next available message
		dbus_connection_read_write(this->conn, 0);
		message = dbus_connection_pop_message(this->conn);

		// loop again if we haven't read a message
		if (NULL == message) { 
			//this->callbacks.debug ("receiveSettings(): No message yet, sleep a second.");
			usleep(1000000);
			continue;
		}

		// check if the message is a signal from the
		// correct interface and with the correct name
		this->callbacks.debug ("call dbus_message_is_signal()");
		if (dbus_message_is_signal (message, this->dbusMatchInterface.c_str(), "sessionConfig")) {

			if (!dbus_message_iter_init(message, &args)) {
				cerr << "Message has no arguments!\n";
				dbus_connection_flush (conn);
				dbus_message_unref (message);
				continue;
			}
			
			if (DBUS_TYPE_STRING != dbus_message_iter_get_arg_type(&args)) {
				cerr << "First argument is not a string!\n";
			} else {
				dbus_message_iter_get_basic(&args, &parameter);
				ss.str("");
				ss << parameter;
				this->nxserver = ss.str();
				count++;
			}

			// read the rest of the parameters
			int t;
			while (dbus_message_iter_next (&args)) {
				if (DBUS_TYPE_STRING == (t = dbus_message_iter_get_arg_type(&args))) {
					dbus_message_iter_get_basic(&args, &parameter);
					//cout << "Arg-" << count << ": " << parameter << endl;
					ss.str("");
					ss << parameter;
					switch (count) {
					case 2:
						this->nxuser = ss.str();
						break;
					case 3:
						this->nxpass = ss.str();
						break;
					case 4:
						this->sessionData.sessionName = ss.str();
						break;
					case 5:
						this->sessionData.sessionType = ss.str();
						break;
					case 8:
						this->sessionData.linkType = ss.str();
						break;
					case 10:
						this->sessionData.backingstore = ss.str();
						break;
					case 13:
						this->sessionData.geometry = ss.str();
						break;
					case 14:
						this->sessionData.keyboard = ss.str();
						break;
					case 15:
						this->sessionData.kbtype = ss.str();
						break;
					case 17:
						this->sessionData.agentServer = ss.str();
						break;
					case 18:
						this->sessionData.agentUser = ss.str();
						break;
					case 19:
						this->sessionData.agentPass = ss.str();
						break;
					case 21:
						this->sessionData.key = ss.str();
						break;
					case 24:
						this->sessionData.customCommand = ss.str();
						break;
					default:
						this->callbacks.error ("ERROR: parameter type does not match its position in the message.");
						break;
					}
					count++;
				} else if (t == DBUS_TYPE_INT32) {
					int iparam = 0;
					dbus_message_iter_get_basic(&args, &iparam);
					switch (count) {
					case 1:
						this->nxport = iparam;
						break;
					case 6:
						this->sessionData.cache = iparam;
						break;
					case 7:
						this->sessionData.images = iparam;
						break;
					case 9:
						this->sessionData.render = (iparam>0) ? true : false;
						break;
					case 11:
						this->sessionData.imageCompressionMethod = iparam;
						break;
					case 12:
						this->sessionData.imageCompressionLevel = iparam;
						break;
					case 16:
						this->sessionData.media = (iparam>0) ? true : false;
						break;
					case 20:
						this->sessionData.cups = iparam;
						break;
					case 22:
						this->sessionData.encryption = (iparam>0) ? true : false;
						break;
					case 23:
						this->sessionData.fullscreen = (iparam>0) ? true : false;
						break;
					case 25:
						this->sessionData.virtualDesktop = (iparam>0) ? true : false;
						break;
					default:
						this->callbacks.error ("ERROR: parameter type does not match its position in the message.");
						break;
					}
					count++;

				} else {
					this->callbacks.error ("ERROR: parameter is not string or int.");
				}
			}
			settings_transferred = true;			
		} else {
			this->callbacks.debug ("this message is not a signal");
		}

		// Anything else required for cleanup?
		dbus_connection_flush (this->conn);

		// free the message
		dbus_message_unref (message);

	} // while()
	this->callbacks.debug ("Got the session settings over the dbus\n");

	if (this->nxserver.size() == 0 || this->nxuser.size() == 0) {
		// We need at least these to be able to connect. Leave
		// nxpass out, in case that has been set to an empty
		// string.
		return -1;
	} else {
		return 0;
	}
}


void
Nxcl::setSessionDefaults (void)
{
	// Some defaults
	this->sessionData.sessionName = "default session";
	this->sessionData.sessionType = "unix-gnome";
	this->sessionData.cache = 8;
	this->sessionData.images = 32;
	this->sessionData.linkType = "adsl";
	this->sessionData.render = true;
	this->sessionData.backingstore = "when_requested";
	this->sessionData.imageCompressionMethod = 2;
	// this->sessionData.imageCompressionLevel;
	this->sessionData.geometry = "800x600+0+0"; // This'll be the size of the session
	this->sessionData.keyboard = "defkeymap";
	this->sessionData.kbtype = "pc102/defkeymap";
	this->sessionData.media = false;
	this->sessionData.agentServer = "";
	this->sessionData.agentUser = "";
	this->sessionData.agentPass = "";
	this->sessionData.cups = 0;
	this->sessionData.encryption = true;
	this->sessionData.suspended = false;
	this->sessionData.fullscreen = false; // If true, session.geometry
					  // is ignored
	this->sessionData.virtualDesktop = false;

	return;
}

void
Nxcl::startTheNXConnection (void)
{
	// First things first; set the sessionData.
	// FIXME: Should check if this->sessionData has been set.
	this->nxclientlib.setSessionData (&this->sessionData);

	this->nxclientlib.setUsername (this->nxuser);
	this->nxclientlib.setPassword (this->nxpass);

	// Probe for X display resolution in nxcl.
	this->nxclientlib.setResolution (this->xRes, this->yRes); 
                                               // This is the size of
                                               // your screen... We need
					       // to set this from the
					       // client, unless we can
					       // probe within nxcl.
	this->nxclientlib.setDepth (this->displayDepth); 
                                               // depth gets stored
	                                       // in NXSession
					       // object. nxcl.setDepth
					       // is a wrapper
					       // around
					       // NXSession's
					       // setDepth method.

	/* If we have session info, start up the connection */
	if (this->sessionData.key.size() == 0) { // Shouldn't need this->sessionData.encryption here.
		this->callbacks.error (_("No key supplied! Please fix your client to send a key via dbus!"));
	} else {
		this->nxclientlib.invokeNXSSH("supplied",
					      this->nxserver,
					      this->sessionData.encryption,
					      this->sessionData.key, this->nxport);
	}
}

void
Nxcl::haveResumableSessions (list<NXResumeData> resumable)
{
	this->callbacks.debug ("haveResumableSessions Called");
	// Send the list to the calling client:
	this->sendResumeList (resumable);
	this->callbacks.debug ("sent the resume list, about to receiveStartInstruction");
	// Wait (and block) until we are told whether to resume one of
	// the sessions or start a new one.
	this->receiveStartInstruction();
	this->callbacks.debug ("receiveStartInstruction() returned.");
}

void
Nxcl::noResumableSessions (void)
{
	this->callbacks.debug ("noResumableSessions Called");
	DBusMessage *msg = dbus_message_new_signal ("/org/freenx/nxcl/dbus/AvailableSession",
						    this->dbusSendInterface.c_str(),
						    "Connecting");

	dbus_connection_send (this->conn, msg, NULL);
	dbus_message_unref (msg);
}

void
Nxcl::sendResumeList (list<NXResumeData>& resumable)
{
	this->callbacks.debug ("sendResumeList called, will send on " + this->dbusSendInterface + " interface");
	list<NXResumeData>::iterator it;
	for (it=resumable.begin(); it!=resumable.end(); it++) {

		this->callbacks.debug ("Sending a session..");

		DBusMessage *message;

		/* Create a new signal "AvailableSession" on the
		 * "org.freenx.nxcl.nxcl" interface, from the object
		 * "/org/freenx/nxcl/dbus/AvailableSession". */
		message = dbus_message_new_signal ("/org/freenx/nxcl/dbus/AvailableSession",
						   this->dbusSendInterface.c_str(),
						   "AvailableSession");

		// We have to create a const char* pointer for each
		// string variable in the NXResumeSessions struct.
		const char* sessionType = (*it).sessionType.c_str();
		const char* sessionID = (*it).sessionID.c_str();
		const char* options = (*it).options.c_str();
		const char* screen = (*it).screen.c_str();
		const char* available = (*it).available.c_str();
		const char* sessionName = (*it).sessionName.c_str();

		// Bundle up the available session
		dbus_message_append_args 
			(message,
			 DBUS_TYPE_INT32,  &(*it).display,
			 DBUS_TYPE_STRING, &sessionType,
			 DBUS_TYPE_STRING, &sessionID,
			 DBUS_TYPE_STRING, &options,
			 DBUS_TYPE_INT32,  &(*it).depth,
			 DBUS_TYPE_STRING, &screen,
			 DBUS_TYPE_STRING, &available,
			 DBUS_TYPE_STRING, &sessionName,
			 DBUS_TYPE_INVALID);
		dbus_connection_send (this->conn, message, NULL);
		dbus_message_unref (message);
	}

	DBusMessage *complete = dbus_message_new_signal ("/org/freenx/nxcl/dbus/AvailableSession",
						         this->dbusSendInterface.c_str(),
							 "NoMoreAvailable");

	this->callbacks.debug ("About to send the finishup message");
	dbus_connection_send (this->conn, complete, NULL);
	this->callbacks.debug ("Sent the finishup message");
	dbus_message_unref (complete);
}

void
Nxcl::serverCapacityReached (void)
{
	DBusMessage *msg = dbus_message_new_signal ("/org/freenx/nxcl/dbus/AvailableSession",
						    this->dbusSendInterface.c_str(),
						    "ServerCapacityReached");
	
	dbus_connection_send (this->conn, msg, NULL);
	dbus_message_unref (msg);
}

void
Nxcl::sendDbusInfoMsg (string& info)
{
	DBusMessage *msg = dbus_message_new_signal ("/org/freenx/nxcl/dbus/SessionStatus",
						    this->dbusSendInterface.c_str(),
						    "InfoMessage");

	/* Add info to msg*/
	const char* infoptr = info.c_str();

	// Bundle up the available session
	dbus_message_append_args (msg,
				  DBUS_TYPE_STRING, &infoptr,
				  DBUS_TYPE_INVALID);

	dbus_connection_send (this->conn, msg, NULL);
	dbus_message_unref (msg);
}

void
Nxcl::sendDbusInfoMsg (int num, string& info)
{
	DBusMessage *msg = dbus_message_new_signal ("/org/freenx/nxcl/dbus/SessionStatus",
						    this->dbusSendInterface.c_str(),
						    "InfoMessage");

	/* Add info to msg*/
	const char* infoptr = info.c_str();

	// Bundle up the available session
	dbus_message_append_args (msg,
				  DBUS_TYPE_STRING, &infoptr,
				  DBUS_TYPE_INT32, &num,
				  DBUS_TYPE_INVALID);

	dbus_connection_send (this->conn, msg, NULL);
	dbus_message_unref (msg);
}

void
Nxcl::sendDbusErrorMsg (string& errorMsg)
{
	DBusMessage *msg = dbus_message_new_signal ("/org/freenx/nxcl/dbus/SessionStatus",
						    this->dbusSendInterface.c_str(),
						    "ErrorMessage");

	/* Add info to msg */
	const char* errptr = errorMsg.c_str();

	// Bundle up the available session
	dbus_message_append_args (msg,
				  DBUS_TYPE_STRING, &errptr,
				  DBUS_TYPE_INVALID);

	dbus_connection_send (this->conn, msg, NULL);
	dbus_message_unref (msg);
}

void
Nxcl::receiveStartInstruction (void)
{
	DBusMessage * message;
	DBusMessageIter args;
	dbus_int32_t parameter = 0;
	bool instruction_received = false;
	stringstream ss;

	this->callbacks.debug ("receiveStartInstruction() called");

	// loop listening for signals being emitted
	while (instruction_received == false) {
		
		// non blocking read of the next available message
		dbus_connection_read_write(this->conn, 0);
		message = dbus_connection_pop_message(this->conn);

		// loop again if we haven't read a message
		if (NULL == message) { 
			usleep(1000000);
			continue;
		}

		// check if the message is a signal from the
		// correct interface and with the correct name
		if (dbus_message_is_signal (message, this->dbusMatchInterface.c_str(), "sessionChoice")) {
			// read the parameters
			if (!dbus_message_iter_init(message, &args))
				cerr << "Message has no arguments!\n"; 
			else if (DBUS_TYPE_INT32 != dbus_message_iter_get_arg_type(&args))
				cerr << "Argument is not int32!\n";
			else {
				dbus_message_iter_get_basic(&args, &parameter);
				instruction_received = true;
				if (parameter < 0) {
					// No action, start a new connection
				} else {
					this->nxclientlib.chooseResumable (parameter);
				}
			}

		} else if (dbus_message_is_signal (message, this->dbusMatchInterface.c_str(), "terminateSession")) {
			// read the parameters
			if (!dbus_message_iter_init(message, &args))
				cerr << "Message has no arguments!\n"; 
			else if (DBUS_TYPE_INT32 != dbus_message_iter_get_arg_type(&args))
				cerr << "Argument is not int32!\n";
			else {
				dbus_message_iter_get_basic(&args, &parameter);
				ss.str("");
				ss << parameter;
				this->callbacks.debug ("Terminating: " + ss.str());
				instruction_received = true;
				if (parameter < 0) {
					// No action, start a new connection
				} else {
					this->nxclientlib.terminateSession (parameter);
				}
			}
		}
		dbus_connection_flush (this->conn);
		dbus_message_unref (message);
	}

}

void
Nxcl::requestConfirmation (string msg)
{
	this->callbacks.error ("WARNING: requestConfirmation(): This is a placeholder method "
			       "to deal with sending back a yes "
			       "or a no answer. For now, we just set "
			       "this->nxclientlib.getSession().setContinue(true);");
	this->nxclientlib.getSession()->setContinue (true);
}
//@}
