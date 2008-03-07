/***************************************************************************
                   nxlaunch: A GTK NX Client based on nxcl
                   ---------------------------------------
    begin                : September 2007
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
#ifndef _NXLAUNCH_H_
#define _NXLAUNCH_H_ 1

#define DBUS_API_SUBJECT_TO_CHANGE 1
#include <dbus/dbus.h>
#include "../lib/callbacks_nx.h"

#ifdef DEBUG
# if DEBUG==0
# undef DEBUG
# endif
#endif

#ifndef PRINTERR_DEFINED
# define PRINTERR_DEFINED 1
# ifdef DEBUG
#  define printerr(args...)	fprintf(stderr, ## args);
#  define printdbg(args...)	fprintf(stdout, ## args);
# else
#  define printerr(args...)
#  define printdbg(args...)
# endif
#endif

/*!
 * Some definitions of numbers that we can send over to the frontend
 * client to tell it how we're getting along with the connection...
 * These should be the same as those found in nxcl/lib/nxdata.h.
 */
#define NXCL_PROCESS_STARTED        1000001
#define NXCL_PROCESS_EXITED         1000002
#define NXCL_AUTH_FAILED            1000003
#define NXCL_AUTHENTICATING         1000004
#define NXCL_LOGIN_FAILED           1000005
#define NXCL_HOST_KEY_VERIFAILED    1000006
#define NXCL_INVOKE_PROXY           1000007
#define NXCL_STARTING               1000008
#define NXCL_FINISHED               1000009
#define NXCL_ALIVE                  1000010

/*
 * Column ids for NX Connection List - the list of configured NX connections 
 * This mirrors nx_connection in callbacks_nx.h
 */
enum
{
	CONN_CONNECTIONNAME,
	CONN_SERVERHOST,
	CONN_SERVERPORT,
	CONN_USER,
	CONN_PASS,
	CONN_REMEMBERPASS,
	CONN_DISABLENODELAY,
	CONN_DISABLEZLIB,
	CONN_ENABLESSLONLY,
	CONN_LINKSPEED,
	CONN_PUBLICKEY,
	CONN_DESKTOP,
	CONN_SESSION,
	CONN_CUSTOMUNIXDESKTOP,
	CONN_COMMANDLINE,
	CONN_VIRTUALDESKTOP,
	CONN_XAGENTENCODING,
	CONN_USETAINT,
	CONN_XDMMODE,
	CONN_XDMHOST,
	CONN_XDMPORT,
	CONN_FULLSCREEN,
	CONN_RESOLUTIONWIDTH,
	CONN_RESOLUTIONHEIGHT,
	CONN_GEOMETRY,
	CONN_IMAGEENCODING,
	CONN_JPEGQUALITY,
	CONN_ENABLESOUND,
	CONN_IPPPORT,
	CONN_IPPPRINTING,
	CONN_SHARES,
	CONN_AGENTSERVER,
	CONN_AGENTUSER,
	CONN_AGENTPASS,
	N_CONN_COLUMNS        /* the number of entries here */
};

/*! Similar to NXResumeData in nxcl, but with gchar* instead of string*/
struct NXResumeData {
	int display;
	gchar * sessionType;
	gchar * sessionID;
	gchar * options;
	int depth;
	gchar * screen;
	gchar * available;
	gchar * sessionName;
};

/*!
 * Column ids for NX Session resume list. This appears when you
 * connect to a server on which there are some running and suspended
 * sessions that you could connect to.
 * This mirrors NXResumeData above.
 */
enum
{
	SESS_INDEX,
	SESS_DISPLAY,
	SESS_SESSIONTYPE,
	SESS_TYPEICON,
	SESS_SESSIONID,
	SESS_OPTIONS,
	SESS_DEPTH,
	SESS_SCREEN,
	SESS_AVAILABLE,
	SESS_SESSIONNAME,
	N_SESS_COLUMNS        /* the number of entries here */
};

/*!
 * Modes in which nxlaunch can be started
 *
 * MODE_MAIN - no switches given
 * MODE_NAMED - just a name given
 * MODE_FILE - a file switch given
 * MODE_EDIT_NAMED - name and --edit switch given
 * MODE_EDIT_FILE - file and --edit switch given
 */
enum
{
	MODE_MAIN,
	MODE_NAMED,
	MODE_FILE,
	MODE_EDIT_NAMED,
	MODE_EDIT_FILE,
	N_MODES
};

/*!
 * Function Declarations
 */
//@{

/*!
 * Callbacks for User Interface
 */
//@{
void on_button_nxlaunch_launch_clicked (GtkButton * button);
#if BUILT_IN_CONFIGURATION==1
void on_button_nxlaunch_new_clicked (GtkButton * button);
void on_button_nxlaunch_edit_clicked (GtkButton * button);
void on_button_nxlaunch_delete_clicked (GtkButton * button);
void on_button_confirm_delete_clicked (GtkButton * button);
/*!
 * Called when the OK button is clicked in the config window. On this
 * callback we need to read the data from the window into an nx_conn
 * structure then read that data into the relevant field in the
 * connection list.
 */
void on_conn_new_ok_clicked (GtkButton * button);
#endif
void on_button_nxlaunch_quit_clicked (GtkButton * button);
void on_button_nxsession_connect_clicked (GtkButton * button);
void on_button_nxsession_launch_clicked (GtkButton * button);
void on_button_nxsession_terminate_clicked (GtkButton * button);
void on_button_nxsession_cancel_clicked (GtkButton * button);
/*!
 * \brief Set the password in nx_conn and launch the connection.
 *
 * This callback will check if there is an nx_connection structure in
 * memory, pointed to by the global pointer nx_conn_glob. If there
 * _is_ one, it will launch that connection using launch connection,
 * after setting the password from the password dialogue box. If there
 * is no nx_connection pointed to by the global pointer, it will
 * malloc space for nx_conn, read in the active row from the
 * tree_model in the main window, and then launch that connection.
 */
void on_button_password_ok_clicked (GtkButton * button);
void on_button_password_cancel_clicked (GtkButton * button);
/*!
 * Called when you single click a row in the connection list. This
 * function will update the user in the username entry box on the
 * nxlauncher window.
 */
void on_treeview_connection_cursor_changed (GtkWidget * widget);

//@}

/*!
 * Look in ~/.nx, and build a list of connections based on the number
 * of .nxs files in there.
 *
 * Return number of connections.
 */
int nxconnection_buildlist (void);

/*!
 * Create the memory structure for the treelist of connections
 */
void nxlaunch_create_nxconnection_list (void);

/*!
 * Create the memory structure for the treelist of available sessions
 */
void nxlaunch_create_nxsession_list (void);

/*!
 * Send all the settings in a single dbus signal.
 */
int sendSettings (DBusConnection *bus, struct nx_connection * nx_conn);

/*!
 * Calls receiveSession and interprets the output.
 *
 * \param relist_call Set this to TRUE if this is a call to
 * receiveSession to re-list the sessions in the session list, after
 * terminating a session.
 */
void callReceiveSession (gboolean relist_call);

/*!
 * Used in receiveSession as the return value
 */
#define REPLY_REQUIRED  1
#define NEW_CONNECTION  2
#define SERVER_CAPACITY 3
/*!
 * Listen to the dbus, waiting for a signal to say either that
 * connection is in progress, or giving us a list of possible sessions
 * we could connect to. Return true if nxcld requires a reply such
 * as "please resume session 1".
 */
int receiveSession (DBusConnection* conn);

/*!
 * Send a signal containing the identifier for the NX session that the
 * user would like to start.
 */
int sendReply (DBusConnection *bus, int sessionNum);

/*!
 * Send a signal containing the identifier for the NX session that the
 * user would like to terminate.
 */
int terminateSession (DBusConnection *bus, int sessionNum);

/*!
 * Set up DBUS.
 *
 * Uses global variables dbusConn and dbusErr which need to be set up
 * in the main.c file.
 *
 * Returns the suffix of the dbus naming - e.g. 0 if dbusSendInterface
 * is org.freenx.nxcl.client0 etc.
 *
 * Returns -1 on error.
 */
int obtainDbusConnection (void);


/*!
 * Fork and exec the nxcl process.
 */
void execNxcl (void);

/*!
 * Prepare DBUS for sending/receiving and then send the settings
 */
void sendNxclSettings (struct nx_connection * nx_conn);

/*!
 * Populate nx_conn from the treeview list.
 */
void getActiveConnection (struct nx_connection * nx_conn);

/*!
 * If there is a configured connection with the name connection_name,
 * launch it by calling launch_connection.
 */
gboolean launch_named_connection (gchar * connection_name);

/*!
 * Launch the connection by calling nxcl, then communicating with it
 * via dbus.
 */
void launch_connection (struct nx_connection * nx_conn);

/*!
 * Hide the main nxlaunch window
 */
void nxlauncher_hide (void);

/*!
 * Show the main nxlaunch window
 */
void nxlauncher_show (void);

/*!
 * Get the password field stored in the active (highlighted)
 * connection. If there is a password, return TRUE, if the length of
 * the password is 0, return FALSE.
 */
#define STORED_PASSWORD  0x1
#define ENTERED_PASSWORD 0x2
int checkActiveConnectionPassword (void);

/*!
 * Display a window saying "We're done", then exit after 5 seconds.
 * This is so that the program doesn't appear to exit before the NX
 * session window appears.
 */
void nxlaunch_quit_with_window (void);

/*!
 * Display a progress window, which could have a progress bar on it,
 * and should have some text output to the user.
 */
void nxlaunch_display_progress (void);

/*!
 * A listener for dbus status messages. We use the contents of these
 * messages to update the progress bar for user feedback on connection
 * progress.
 */
gboolean nxlaunch_status_listener (gpointer data);

/*!
 * Wait for an alive message from nxcl. This function blocks until the
 * message is received or a timeout has occurred.
 */
gboolean waitForAlive (void);

/*!
 * A non-recoverable error occurred. Show the user a dialog explaining
 * the error, with a quit button as the only available option. Grey
 * out all other windows.
 *
 * \param message The error message to display
 */
void nxlaunch_error_requiring_quit (const char * message);

//@} Fn Declarations

#endif /* _NXLAUNCH_H_ */
