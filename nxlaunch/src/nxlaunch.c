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
#include "i18n.h"

/*! 
 * config.h is created by the configure script and contains
 * configure/compile time choices made by the program compiler.
 */
#include "../config.h"

#include <glade/glade.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <locale.h>
#include <dirent.h>
#include <string.h>
#include <unistd.h>
#define DBUS_API_SUBJECT_TO_CHANGE 1
#include <dbus/dbus.h>

#include "nxlaunch.h"
#include "../lib/callbacks_nx.h"

/* Use a global for the xml tree */
extern GladeXML * xml_nxlaunch_glob;
#if BUILT_IN_CONFIGURATION==1
extern GladeXML * xml_glob;
#endif

extern GtkListStore * conn_store;
extern GtkTreeIter    conn_iter;
extern GtkTreeIter    active_iter_glob;
extern guint          next_conn_type;
extern GtkListStore * sess_store;
extern GtkTreeIter    sess_iter;
extern guint          next_sess_type;

/*!
 * Lazily use some globals for the send/receive dbus interfaces.
 */
extern gchar * dbusSendInterface;
extern gchar * dbusRecvInterface;
extern gchar * dbusMatchString;
extern int              dbusNum;
extern DBusConnection * dbusConn;
extern DBusError        dbusErr;

/*!
 * The modus operandi. See main.c and nxlaunch.h
 */
extern int nxlaunch_mode;

/*!
 * Oh, look, another global! This one is used to hold connection
 * information pulled out of the gtk_tree_model of connections when
 * the user launches nxlaunch with a connection name argument. This
 * one is local to nxlaunch.o
 *
 * WARNING: This is a possible source of a hanging pointer!
 */
struct nx_connection * nx_conn_glob = NULL;

/*!
 * Callback Functions are first in nxlaunch.c
 */
//@{

/*!
 * Callback for the delete-event signal 
 */
gboolean on_main_win_delete_event (GtkWidget * widget, 
				   GdkEvent * event,
				   gpointer user_data)
{
	return FALSE;
}

/*!
 * There's a password entry on the main window and then a standalone
 * password entry box. 
 *
 * If we are launching from the main window, then we use the
 * password entry box on the main window, which is filled in from the
 * configuration file if applicable.
 *
 * If the user presses launch without entering a password, then we
 * either prompt him to enter on on the main window or we throw up
 * the dedicated password window for him to enter the password there.
 *
 * If we are launching a connection from a name supplied on the cmd
 * line then we don't show the main window, instead we give the user
 * the dedicated password window.
 *
 * If we are launching a connection from a file given on the cmd line,
 * then we do as for a name given on the cmd line.
 *
 * This implies we start up in different modes; something like:
 * MODE_MAIN - no switches given
 * MODE_NAMED - just a name given
 * MODE_FILE - a file switch given
 * MODE_EDIT_NAMED - name and --edit switch given
 * MODE_EDIT_FILE - file and --edit switch given
 *
 * Do we want a global to hold the mode?
 */
void on_button_nxlaunch_launch_clicked (GtkButton * button)
{
	GtkWidget * widget;
	struct nx_connection * nx_conn = NULL;	

	int flags = checkActiveConnectionPassword();

	/* Malloc and get active connection if state is correct */
	if ((flags & STORED_PASSWORD) || 
	    (nxlaunch_mode == MODE_MAIN && (flags & ENTERED_PASSWORD)) ) {
		printerr ("Have entered or stored password\n");

		/* Get the currently activated connection */
		nx_conn = g_malloc0 (sizeof(struct nx_connection));
		getActiveConnection (nx_conn);

		/* Grey out the main window */
		widget = glade_xml_get_widget (xml_nxlaunch_glob, "nxlauncher");
		gtk_widget_set_sensitive (widget, FALSE);
		while (gtk_events_pending()) gtk_main_iteration();
	}


	if (flags & STORED_PASSWORD) {

		printerr ("There's a stored password, just launch connection\n");
		launch_connection (nx_conn);

	} else if (nxlaunch_mode == MODE_MAIN && (flags & ENTERED_PASSWORD)) {

		printerr ("User entered password; get password into nx_conn then launch.\n");
		/* Read the password into nx_conn->Pass, mallocing if necessary */
		widget = glade_xml_get_widget (xml_nxlaunch_glob, "entry_nxlaunch_password");
		if (!(nx_conn->Pass = g_try_realloc (nx_conn->Pass, 
						     NX_FIELDLEN * sizeof(gchar)))) {
			printerr ("NXLAUNCH> Failed g_try_realloc in %s\n", __FUNCTION__);
		}
		strncpy (nx_conn->Pass, gtk_entry_get_text (GTK_ENTRY(widget)), NX_FIELDLEN);
		gtk_entry_set_text (GTK_ENTRY(widget), "");

		launch_connection (nx_conn);

	} else {
		/* In this case, nx_conn should not have been
		   g_malloc0ed */
		if (nx_conn != NULL) {
			g_error ("We seem to have allocated memory when we shouldn't have\n");
			_exit(-1);
		}
		
		/* Prompt user to enter password in the password box */
		widget = glade_xml_get_widget (xml_nxlaunch_glob, "dialog_password");
		gtk_widget_show_all (widget);
		while (gtk_events_pending()) gtk_main_iteration();
		widget = glade_xml_get_widget (xml_nxlaunch_glob, "nxlauncher");
		gtk_widget_set_sensitive (widget, FALSE);
	}
	return;
}

#if BUILT_IN_CONFIGURATION==1
void on_button_nxlaunch_new_clicked (GtkButton * button)
{
	GtkWidget * widget;

	widget = glade_xml_get_widget (xml_glob, "conn_new_nx");
	gtk_widget_show_all (widget);

	/* Initialise the new connection popup */
	zero_new_nx_popup();

	return;
}

void on_button_nxlaunch_edit_clicked (GtkButton * button)
{
	GtkWidget * widget;
	struct nx_connection * nx_conn;

	widget = glade_xml_get_widget (xml_glob, "conn_new_nx");
	gtk_widget_show_all (widget);

	// Read in nx_conn for the highlighted entry
	nx_conn = g_malloc0 (sizeof (struct nx_connection));
	getActiveConnection (nx_conn);	
	setup_nx_popup (nx_conn);
	nx_connection_free (nx_conn);

	return;
}

void on_button_nxlaunch_delete_clicked (GtkButton * button)
{
	GtkWidget * widget;
	widget = glade_xml_get_widget (xml_glob, "confirm_delete");
	gtk_widget_show_all (widget);
	return;
}

void on_button_confirm_delete_clicked (GtkButton * button)
{
	GtkTreeView * list_tree;
	GtkTreeSelection * selected;
	GtkTreeIter iter;
	GtkTreeModel * tree_model;
	gchar * name = NULL;

	/* Get iter for current row and update connection list. */
	list_tree = GTK_TREE_VIEW (glade_xml_get_widget (xml_nxlaunch_glob, "treeview_nxconnection"));
	selected = gtk_tree_view_get_selection (list_tree);
	tree_model = GTK_TREE_MODEL (conn_store);
	if (gtk_tree_selection_get_selected (selected, &tree_model, &iter)) {
		/* Get connection name */
		gtk_tree_model_get (tree_model, &iter, CONN_CONNECTIONNAME, &name, -1);

		/* delete the row */
		if (FALSE == gtk_list_store_remove (conn_store, &iter)) {
			printerr ("NXLAUNCH> Failed to remove that row..\n");
		}
	}

	/* Unlink the connection file */
	delete_nx_connection (name);

	if (name) { g_free (name); }

	return;
}

void on_conn_new_ok_clicked (GtkButton * button)
{
	GtkTreeView * list_tree;
	GtkTreeSelection * selected;
	GtkTreeIter iter;
	GtkTreeModel * tree_model;

	gchar dummy_name[NX_FIELDLEN];
	gchar dummy_server[NX_FIELDLEN];
	gchar dummy_uname[NX_FIELDLEN];
	int dummy_fullscreen = 0;
	struct nx_connection * nx_conn;

	/*
	 * read_nx_popup() into nx_conn. Update the connection list
	 * entry from the data.
	 */
	nx_conn = nx_connection_malloc();
	read_nx_popup (dummy_name, dummy_server, dummy_uname, &dummy_fullscreen, nx_conn);

	/* Get iter for current row and update connection list. */
	list_tree = GTK_TREE_VIEW (glade_xml_get_widget (xml_nxlaunch_glob, "treeview_nxconnection"));
	selected = gtk_tree_view_get_selection (list_tree);
	tree_model = GTK_TREE_MODEL (conn_store);
	if (gtk_tree_selection_get_selected (selected, &tree_model, &iter)) {
		gtk_list_store_set (conn_store, &conn_iter,
				    CONN_CONNECTIONNAME,    nx_conn->ConnectionName,
				    CONN_SERVERHOST,        nx_conn->ServerHost,
				    CONN_SERVERPORT,        nx_conn->ServerPort,
				    CONN_USER,              nx_conn->User,
				    CONN_PASS,              nx_conn->Pass,
				    CONN_REMEMBERPASS,      nx_conn->RememberPassword ? TRUE : FALSE,
				    CONN_DISABLENODELAY,    nx_conn->DisableNoDelay ? TRUE : FALSE,
				    CONN_DISABLEZLIB,       nx_conn->DisableZLIB ? TRUE : FALSE,
				    CONN_ENABLESSLONLY,     nx_conn->EnableSSLOnly ? TRUE : FALSE,
				    CONN_LINKSPEED,         nx_conn->LinkSpeed,
				    CONN_PUBLICKEY,         nx_conn->PublicKey,
				    CONN_DESKTOP,           nx_conn->Desktop,
				    CONN_SESSION,           nx_conn->Session,
				    CONN_CUSTOMUNIXDESKTOP, nx_conn->CustomUnixDesktop,
				    CONN_COMMANDLINE,       nx_conn->CommandLine,
				    CONN_VIRTUALDESKTOP,    nx_conn->VirtualDesktop ? TRUE : FALSE,
				    CONN_XAGENTENCODING,    nx_conn->XAgentEncoding ? TRUE : FALSE,
				    CONN_USETAINT,          nx_conn->UseTaint ? TRUE : FALSE,
				    CONN_XDMMODE,           nx_conn->XdmMode,
				    CONN_XDMHOST,           nx_conn->XdmHost,
				    CONN_XDMPORT,           nx_conn->XdmPort,
				    CONN_FULLSCREEN,        nx_conn->FullScreen ? TRUE : FALSE,	    
				    CONN_RESOLUTIONWIDTH,   nx_conn->ResolutionWidth,
				    CONN_RESOLUTIONHEIGHT,  nx_conn->ResolutionHeight,
				    CONN_GEOMETRY,          nx_conn->Geometry,
				    CONN_IMAGEENCODING,     nx_conn->ImageEncoding,
				    CONN_JPEGQUALITY,       nx_conn->JPEGQuality,
				    CONN_ENABLESOUND,       nx_conn->enableSound ? TRUE : FALSE,
				    CONN_IPPPORT,           nx_conn->IPPPort,
				    CONN_IPPPRINTING,       nx_conn->IPPPrinting ? TRUE : FALSE,
				    CONN_SHARES,            nx_conn->Shares ? TRUE : FALSE,
				    CONN_AGENTSERVER,       nx_conn->agentServer,
				    CONN_AGENTUSER,         nx_conn->agentUser,
				    CONN_AGENTPASS,         nx_conn->agentPass,
				    -1);
	}

	/* Re-write the connection to the xml file */
	write_nx_connection (nx_conn);

	nx_connection_free (nx_conn);
	return;
}
#endif	

void on_button_nxlaunch_quit_clicked (GtkButton * button)
{
	gtk_main_quit();
	return;
}

/*!
 * Connect to the highlighted connection
 */
void on_button_nxsession_connect_clicked (GtkButton * button)
{
	GtkTreeView * list_tree;
	GtkTreeSelection * selected;
	GtkTreeIter iter;
	GtkTreeModel * tree_model;
	int sessionNum = -1;

	printerr ("%s() called\n", __FUNCTION__);

	/* Get the highlighted row in the nxsession list window */
	list_tree = GTK_TREE_VIEW (glade_xml_get_widget (xml_nxlaunch_glob, "treeview_nxsession"));
	selected = gtk_tree_view_get_selection (list_tree);
	tree_model = GTK_TREE_MODEL (conn_store);
	if (gtk_tree_selection_get_selected (selected, &tree_model, &iter)) {
		/* Get the sessionNum out. */
		gtk_tree_model_get (tree_model, &iter, SESS_INDEX, &sessionNum, -1);
	}

	/* Call the "session connect dbus message" fn */
	if (sessionNum > -1) {
		sendReply (dbusConn, sessionNum);
	} else {
		printerr ("NXLAUNCH> %s(): Failed to set sessionNum from the session list gtk tree model\n",
			  __FUNCTION__);
	}

	nxlaunch_display_progress();
}

/*!
 * Send a launch message, saying "launch new"
 */
void on_button_nxsession_launch_clicked (GtkButton * button)
{
	sendReply (dbusConn, -1);
	nxlaunch_display_progress();
}

void on_button_nxsession_terminate_clicked (GtkButton * button)
{
	GtkTreeView * list_tree;
	GtkTreeSelection * selected;
	GtkTreeIter iter;
	GtkTreeModel * tree_model;
	int sessionNum = -1;

	printerr ("%s() called\n", __FUNCTION__);

	/* Get the highlighted row in the nxsession list window */
	list_tree = GTK_TREE_VIEW (glade_xml_get_widget (xml_nxlaunch_glob, "treeview_nxsession"));
	selected = gtk_tree_view_get_selection (list_tree);
	tree_model = GTK_TREE_MODEL (conn_store);
	if (gtk_tree_selection_get_selected (selected, &tree_model, &iter)) {
		/* Get the sessionNum out. */
		gtk_tree_model_get (tree_model, &iter, SESS_INDEX, &sessionNum, -1);
	}

	if (sessionNum > -1) {
		terminateSession (dbusConn, sessionNum);
	} else {
		printerr ("NXLAUNCH> %s(): Failed to set sessionNum from the session list gtk tree model\n",
			  __FUNCTION__);
	}
	
	return;
}

void on_button_nxsession_cancel_clicked (GtkButton * button)
{
	GtkWidget * widget;

	widget = glade_xml_get_widget (xml_nxlaunch_glob, "session_list");
	gtk_widget_hide (widget);
	widget = glade_xml_get_widget (xml_nxlaunch_glob, "nxlauncher");
	gtk_widget_set_sensitive (widget, TRUE);
	gtk_widget_show (widget);

	return;
}

void on_treeview_connection_cursor_changed (GtkWidget * widget)
{
	/* We get passed the treeview_nxconnection object here */
	GtkTreeSelection * selected;
	GtkTreeIter iter;
	GtkTreeModel * tree_model;
	gchar * the_user, * the_pass;
	GtkWidget * entrybox;

	/* We get passed the gtktreeview */
	selected = gtk_tree_view_get_selection (GTK_TREE_VIEW (widget));
	tree_model = GTK_TREE_MODEL (conn_store);
	if (gtk_tree_selection_get_selected (selected, &tree_model, &iter)) {

		/* Get all the information out of the row */
		gtk_tree_model_get (tree_model, &iter,
				    CONN_USER, &the_user, 
				    CONN_PASS, &the_pass,
				    -1);
	}	

	/*
	 * Now place the user in entry_nxlaunch_user
	 */
	entrybox = glade_xml_get_widget (xml_nxlaunch_glob, "entry_nxlaunch_user");
	if (strlen (the_user) < 1) {
		the_user = g_realloc (the_user, sizeof(gchar)<<3); // 8 gchars
		snprintf (the_user, 6, "%s", "noone");
	}
	gtk_entry_set_text (GTK_ENTRY(entrybox), the_user);

	/*
	 * Fill in the password if it has been set in the
	 * configuration file
	 */
	entrybox = glade_xml_get_widget (xml_nxlaunch_glob, "entry_nxlaunch_password");
	if (strlen (the_pass) > 0) {
		gtk_entry_set_text (GTK_ENTRY(entrybox), the_pass);
	}
	
	g_free (the_user);
	g_free (the_pass);

	// Place the focus on the password entry box
	//gtk_widget_grab_focus (entrybox);
	
	return;
}

//@} Callbacks


int nxconnection_buildlist (void)
{
	DIR *dp;
	struct dirent *ep;
	int nlen = 0, rtn = 0;
	gchar config[1024];

	snprintf (config, 1023, "%s/.nx/config", getenv("HOME"));

	dp = opendir (config);
	if (dp == NULL) {
		printerr ("Couldn't opendir...\n");
		return 0;
	}

	while ((ep = readdir (dp))) {

		/* If d_name ends in .nxs, it should be an NX config file. */
		nlen = strlen (ep->d_name);
		if ( ep->d_name[--nlen] == 's'
		     && ep->d_name[--nlen] == 'x'
		     && ep->d_name[--nlen] == 'n'
		     && ep->d_name[--nlen] == '.' ) {
			
			gchar nxname[128];
			snprintf (nxname, ++nlen, ep->d_name);
			struct nx_connection * nx_conn;
			nx_conn = nx_connection_malloc();
			/* Read in the connection using the callbacks_nx library fn */
			if (0 == read_nx_connection (nx_conn, nxname)) {
				/* Now place it in the connection store list */
				gtk_list_store_append (conn_store, &conn_iter);
				gtk_list_store_set (conn_store, &conn_iter,
						    CONN_CONNECTIONNAME,    nx_conn->ConnectionName,
						    CONN_SERVERHOST,        nx_conn->ServerHost,
						    CONN_SERVERPORT,        nx_conn->ServerPort,
						    CONN_USER,              nx_conn->User,
						    CONN_PASS,              nx_conn->Pass,
						    CONN_REMEMBERPASS,      nx_conn->RememberPassword ? TRUE : FALSE,
						    CONN_DISABLENODELAY,    nx_conn->DisableNoDelay ? TRUE : FALSE,
						    CONN_DISABLEZLIB,       nx_conn->DisableZLIB ? TRUE : FALSE,
						    CONN_ENABLESSLONLY,     nx_conn->EnableSSLOnly ? TRUE : FALSE,
						    CONN_LINKSPEED,         nx_conn->LinkSpeed,
						    CONN_PUBLICKEY,         nx_conn->PublicKey,
						    CONN_DESKTOP,           nx_conn->Desktop,
						    CONN_SESSION,           nx_conn->Session,
						    CONN_CUSTOMUNIXDESKTOP, nx_conn->CustomUnixDesktop,
						    CONN_COMMANDLINE,       nx_conn->CommandLine,
						    CONN_VIRTUALDESKTOP,    nx_conn->VirtualDesktop ? TRUE : FALSE,
						    CONN_XAGENTENCODING,    nx_conn->XAgentEncoding ? TRUE : FALSE,
						    CONN_USETAINT,          nx_conn->UseTaint ? TRUE : FALSE,
						    CONN_XDMMODE,           nx_conn->XdmMode,
						    CONN_XDMHOST,           nx_conn->XdmHost,
						    CONN_XDMPORT,           nx_conn->XdmPort,
						    CONN_FULLSCREEN,        nx_conn->FullScreen ? TRUE : FALSE,	    
						    CONN_RESOLUTIONWIDTH,   nx_conn->ResolutionWidth,
						    CONN_RESOLUTIONHEIGHT,  nx_conn->ResolutionHeight,
						    CONN_GEOMETRY,          nx_conn->Geometry,
						    CONN_IMAGEENCODING,     nx_conn->ImageEncoding,
						    CONN_JPEGQUALITY,       nx_conn->JPEGQuality,
						    CONN_ENABLESOUND,       nx_conn->enableSound ? TRUE : FALSE,
						    CONN_IPPPORT,           nx_conn->IPPPort,
						    CONN_IPPPRINTING,       nx_conn->IPPPrinting ? TRUE : FALSE,
						    CONN_SHARES,            nx_conn->Shares ? TRUE : FALSE,
						    CONN_AGENTSERVER,       nx_conn->agentServer,
						    CONN_AGENTUSER,         nx_conn->agentUser,
						    CONN_AGENTPASS,         nx_conn->agentPass,
						    -1);
				rtn++;
			}
			nx_connection_free (nx_conn);
		}
	}
	closedir (dp);

	if (rtn > 0) {
		GtkTreeIter treeIter;
		GtkTreeView * list_tree;
		GtkTreeSelection * select;

		list_tree = GTK_TREE_VIEW (glade_xml_get_widget (xml_nxlaunch_glob,
								 "treeview_nxconnection"));

		/* Get the first iter */
		gtk_tree_model_get_iter_first (GTK_TREE_MODEL (conn_store), &treeIter);
		/* Now set it as the selected row somehow */
		select = gtk_tree_view_get_selection (list_tree);
		gtk_tree_selection_select_iter (select, &treeIter);
		
		on_treeview_connection_cursor_changed (GTK_WIDGET(list_tree));
	}

	return rtn;
}

void nxlaunch_create_nxconnection_list (void)
{
	GtkTreeViewColumn * column;
	GtkCellRenderer * renderer;
	GtkTreeView * list_tree;

	list_tree = GTK_TREE_VIEW (glade_xml_get_widget (xml_nxlaunch_glob, "treeview_nxconnection"));
	gtk_tree_view_set_model (list_tree, GTK_TREE_MODEL (conn_store));

	/* Connection Name Column */
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Name"), renderer,
							   "text", CONN_CONNECTIONNAME,
							   NULL);
	gtk_tree_view_column_set_resizable (column, TRUE);
	g_object_set (renderer, "width", 160, NULL);
	gtk_tree_view_append_column (list_tree, column);


	/* Server column */
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Server"), renderer,
							   "text", CONN_SERVERHOST,
							   NULL);
	g_object_set (renderer, "width", 100, NULL);
	gtk_tree_view_append_column (list_tree, column);

	/* User column */
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Username"), renderer,
							   "text", CONN_USER,
							   NULL);
	g_object_set (renderer, "width", 60, NULL);
	gtk_tree_view_append_column (list_tree, column);

	/* Session column */
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Session"), renderer,
							   "text", CONN_SESSION,
							   NULL);
	g_object_set (renderer, "width", 60, NULL);
	gtk_tree_view_append_column (list_tree, column);

	/* Desktop column */
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Desktop"), renderer,
							   "text", CONN_DESKTOP,
							   NULL);
	g_object_set (renderer, "width", 60, NULL);
	gtk_tree_view_append_column (list_tree, column);

	/* Geometry column */
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Geometry"), renderer,
							   "text", CONN_GEOMETRY,
							   NULL);
	g_object_set (renderer, "width", 100, NULL);
	gtk_tree_view_append_column (list_tree, column);

	return;
}

void nxlaunch_create_nxsession_list (void)
{
	GtkTreeViewColumn * column;
	GtkCellRenderer * renderer;
	GtkTreeView * list_tree;

	list_tree = GTK_TREE_VIEW (glade_xml_get_widget (xml_nxlaunch_glob, "treeview_nxsession"));
	gtk_tree_view_set_model (list_tree, GTK_TREE_MODEL (sess_store));

	/* Name Column */
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Name"), renderer,
							   "text", SESS_SESSIONNAME,
							   NULL);
	gtk_tree_view_column_set_resizable (column, TRUE);
        g_object_set (renderer, "width", 200, NULL);
	gtk_tree_view_append_column (list_tree, column);

	/* Icon column (icon representing type) with type as text */
	column = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title (column, _("Type"));

	renderer = gtk_cell_renderer_pixbuf_new();
	gtk_tree_view_column_pack_start (column, renderer, FALSE);
	gtk_tree_view_column_set_attributes (column, renderer, "pixbuf", SESS_TYPEICON, NULL);
	g_object_set (renderer, "width", 32, NULL);

	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start (column, renderer, TRUE);
	gtk_tree_view_column_set_attributes (column, renderer, "text", SESS_SESSIONTYPE, NULL);
	g_object_set (renderer, "width", 100, NULL);

	gtk_tree_view_append_column (list_tree, column);

	/* Status column (add icon later, as above.) */
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Status"), renderer,
							   "text", SESS_AVAILABLE,
							   NULL);
	g_object_set (renderer, "width", 100, NULL);
	gtk_tree_view_append_column (list_tree, column);

	/* Display column */
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (_("Display"), renderer,
							   "text", SESS_DISPLAY,
							   NULL);
	g_object_set (renderer, "width", 100, NULL);
	gtk_tree_view_append_column (list_tree, column);

	return;
}

int sendSettings (DBusConnection *bus, struct nx_connection * nx_conn)
{
	DBusMessage *message;

	/* Create a new setting signal on the "org.freenx.nxcl.client"
	 * interface, from the object "/org/freenx/nxcl/dbus/settingSignal". */
	message = dbus_message_new_signal ("/org/freenx/nxcl/dbus/settingSignal",
					   dbusSendInterface, 
					   "sessionConfig");
	if (NULL == message) {
		printerr ("NXLAUNCH> Message Null\n");
		return -1;
	}

	int media=0, enc=0, fs=0, cups=0, vdesk=0;
	if (nx_conn->enableSound == TRUE) { media = 1; }
	if (nx_conn->EnableSSLOnly == TRUE) { enc = 1; }
	if (nx_conn->FullScreen == TRUE) { fs = 1; }
	if (nx_conn->IPPPrinting == TRUE) { cups = 1; }
	if (nx_conn->VirtualDesktop == TRUE) { vdesk = 1; }
	int cache = 8, images = 24, render = 1;
	const char * backingstore = "when_requested";
	/* FIXME: Would like to store these in the .nxs file */
	const char * keyboard = "";
	const char * kbtype = "";

	printerr ("NXLAUNCH> Sending settings for connection %s, to Server %s\n",
		  nx_conn->ConnectionName, nx_conn->ServerHost);

	/* We have to translate the Desktop setting a bit - adding
	 * unix- to the start of some of them. We're likely to need to
	 * add to this logic for full functionality. */
	char * session_type = NULL;
	session_type = g_malloc0 (NX_FIELDLEN * sizeof (char));
	if ( !strcmp (nx_conn->Session, "unix") ) {
		
		if ( !strcmp (nx_conn->Desktop, "gnome") || 
		     !strcmp (nx_conn->Desktop, "kde") || 
		     !strcmp (nx_conn->Desktop, "cde") || 
		     !strcmp (nx_conn->Desktop, "xdm") ) {
			snprintf (session_type, NX_FIELDLEN, "unix-%s", nx_conn->Desktop);

		} else if (!strcmp (nx_conn->Desktop, "console") ) {

			if (!strcmp (nx_conn->CustomUnixDesktop, "application") ) {
				snprintf (session_type, NX_FIELDLEN, "unix-application");
			} else {
				snprintf (session_type, NX_FIELDLEN, "unix-console");
			}
		}
	} else if ( !strcmp (nx_conn->Session, "shadow") ) {

		snprintf (session_type, NX_FIELDLEN, "shadow");

	} else if ( !strcmp (nx_conn->Session, "windows") ) {
		printerr ("NXLAUNCH> FIXME Add windows support (with agentServer etc)\n");

	} else if ( !strcmp (nx_conn->Session, "vnc") ) {
		printerr ("NXLAUNCH> FIXME Add vnc support (with agentServer etc)\n");

	}

	dbus_message_append_args 
		(message,
		 DBUS_TYPE_STRING, &nx_conn->ServerHost,     //0
		 DBUS_TYPE_INT32,  &nx_conn->ServerPort,
		 DBUS_TYPE_STRING, &nx_conn->User,           //2
		 DBUS_TYPE_STRING, &nx_conn->Pass,
		 DBUS_TYPE_STRING, &nx_conn->ConnectionName, //4
		 DBUS_TYPE_STRING, &session_type,   
		 DBUS_TYPE_INT32,  &cache,                //6
		 DBUS_TYPE_INT32,  &images,
		 DBUS_TYPE_STRING, &nx_conn->LinkSpeed,      //8
		 DBUS_TYPE_INT32,  &render, 
		 DBUS_TYPE_STRING, &backingstore,         //10
		 DBUS_TYPE_INT32,  &nx_conn->ImageEncoding,
		 DBUS_TYPE_INT32,  &nx_conn->JPEGQuality,    //12
		 DBUS_TYPE_STRING, &nx_conn->Geometry,
		 DBUS_TYPE_STRING, &keyboard,             //14
		 DBUS_TYPE_STRING, &kbtype,
		 DBUS_TYPE_INT32,  &media,                //16
		 DBUS_TYPE_STRING, &nx_conn->agentServer,
		 DBUS_TYPE_STRING, &nx_conn->agentUser,      //18
		 DBUS_TYPE_STRING, &nx_conn->agentPass,
		 DBUS_TYPE_INT32,  &cups,                 //20
		 DBUS_TYPE_STRING, &nx_conn->PublicKey,
		 DBUS_TYPE_INT32,  &enc,                  //22
		 DBUS_TYPE_INT32,  &fs,
		 DBUS_TYPE_STRING, &nx_conn->CommandLine,    //24
		 DBUS_TYPE_INT32,  &vdesk,
		 DBUS_TYPE_INVALID);

	/* Send the signal */
	if (!dbus_connection_send (bus, message, NULL)) {
		printerr ("NXLAUNCH> Out Of Memory!\n");
		_exit(1);
	}

	/* Clean up */
	dbus_message_unref (message);
	dbus_connection_flush (bus);
	g_free (session_type);
	return 1;
}

int sendReply (DBusConnection *bus, int sessionNum)
{
	DBusMessage *message;
	DBusMessageIter args;

	/* Create a new session choice signal on the "org.freenx.nxcl.client"
	 * interface, from the object "/org/freenx/nxcl/dbus/sessionChoice". */
	message = dbus_message_new_signal ("/org/freenx/nxcl/dbus/sessionChoice",
					   dbusSendInterface,
					   "sessionChoice");
	if (NULL == message) {
		printerr ("NXLAUNCH> Message Null\n");
		return -1;
	}

	dbus_message_iter_init_append (message, &args);
	if (!dbus_message_iter_append_basic (&args,
					     DBUS_TYPE_INT32,
					     (const void*)&sessionNum)) {
		printerr ("NXLAUNCH> Out Of Memory!\n");
		_exit(1);
	}

	/* Send the signal */
	if (!dbus_connection_send (bus, message, NULL)) {
		printerr ("NXLAUNCH> Out Of Memory!\n");
		_exit(1);	  
	}

	/* Clean up */
	dbus_message_unref (message);
	dbus_connection_flush (bus);

	return 1;
}


int terminateSession (DBusConnection *bus, int sessionNum)
{
	DBusMessage *message;
	DBusMessageIter args;

	/* Create a new session choice signal on the "org.freenx.nxcl.client"
	 * interface, from the object "/org/freenx/nxcl/dbus/sessionChoice". */
	message = dbus_message_new_signal ("/org/freenx/nxcl/dbus/sessionChoice",
					   dbusSendInterface,
					   "terminateSession");
	if (NULL == message) { 
		printerr ("NXLAUNCH> Message Null\n");
		return -1;
	}

	dbus_message_iter_init_append (message, &args);
	if (!dbus_message_iter_append_basic (&args,
					     DBUS_TYPE_INT32,
					     (const void*)&sessionNum)) {
		printerr ("NXLAUNCH> Out Of Memory!\n");
		_exit(1);
	}

	/* Send the signal */
	if (!dbus_connection_send (bus, message, NULL)) {
		printerr ("NXLAUNCH> Out Of Memory!\n");
		_exit(1);	  
	}

	/* Clean up */
	dbus_message_unref (message);
	dbus_connection_flush (bus);

	/* Now we terminated, we listen to nxcl to receive an updated
	 * list of sessions, or for notification that we are now
	 * starting a new session. */
	callReceiveSession (TRUE);

	return 1;
}

void callReceiveSession (gboolean relist_call)
{
	int ret = 0;

	if (REPLY_REQUIRED == (ret = receiveSession (dbusConn))) {
		/* Session window should have been populated
		 * by receiveSession, so we now return and
		 * wait for a button press event (the choice
		 * of session) */

	} else if (SERVER_CAPACITY == ret) {
		/* Have run out of capacity (licences) on the server. */
		nxlaunch_error_requiring_quit (_("Out of licences on server."));

		/* Ok. The above call will lead to quitting the
		 * program. Ideally we should show a list of sessions
		 * so user can terminate one if possible.
		 *
		 * But how to get a list of sessions to terminate?
		 * Does NX actually return that information? I think
		 * it just returns the "out of capacity" message, and
		 * that's that.
		 */

	} else {

		printerr ("NXLAUNCH> receiveSession() returned %d\n", ret);

		/*
		 * Now, in this case, if we have called
		 * callReceiveSession from terminate session, we don't
		 * just want to launch a new session, unless the user
		 * actually presses the new connection button. The
		 * sequence here is: 1) Enter user/pass for the
		 * session. 2) Get back list of 1 suspended session 3)
		 * Terminate it 4) get presented with empty session
		 * window in which you can either click "cancel" to go
		 * back to the connection list, or "new" to create a
		 * new session.
		 */

		if (!relist_call) {
			nxlaunch_display_progress();
		}
	}

	return;
}

/*
 * Wait for and receive a message with available sessions, or a
 * message saying there are no available sessions
 */
#define RECEIVE_TIMEOUT_SECONDS 30
int receiveSession (DBusConnection* conn)
{
	GtkWidget * widget;
	GdkPixbuf * icon;
	GError * gerror = NULL;
	DBusMessage * message;
	DBusMessageIter args;
	DBusError error;
	char * parameter = NULL;
	dbus_int32_t iparam = 0, t = 0;
	int count = 0;
	int timeout = 0;
	gboolean sessions_obtained = FALSE;
	int rtn = 0;

	printerr (_("NXLAUNCH> Waiting to receive session information\n"));

	// Clear the list store we're about to populate.
	gtk_list_store_clear (sess_store);

	dbus_error_init (&error);

	int sessionNum = 0;

	// loop listening for signals being emitted
	while ((sessions_obtained == FALSE) && (++timeout < RECEIVE_TIMEOUT_SECONDS)) {
		
		if (dbus_error_is_set(&error)) { 
			printerr ("NXLAUNCH> Name Error (%s)\n", error.message); 
			dbus_error_free(&error); 
		}

		count = 0;

		// non blocking read of the next available message
		dbus_connection_read_write (conn, 0);
		message = dbus_connection_pop_message (conn);

		// loop again if we haven't read a message
		if (NULL == message) {
			usleep (1000000);
			continue;
		}

		// Is the message the one we're interested in?
		if (dbus_message_is_signal (message, dbusRecvInterface, "AvailableSession")) {

			if (!dbus_message_iter_init (message, &args)) {
				printerr ("NXLAUNCH> Message has no arguments!\n");
				dbus_connection_flush (conn);
				dbus_message_unref (message);
				continue;
			}

			/* Now we have a message containing an
			 * available session, append an entry to the
			 * gtk_list_store of available connections. */
			gtk_list_store_append (sess_store, &sess_iter);

			/* Read the parameters of the message into the sess_store gtk_list_store.
			 * The order of session parameters passed in the dbus message is:
			 * [0]Display(i) | [1]Type(s)   | [2]Session ID(s) | [3]Options(s)
			 * [4]Depth(i)   | [5]Screen(s) | [6]Status(s)     | [7]Session Name(s)
			 */

			gtk_list_store_set (sess_store, &sess_iter, SESS_INDEX, sessionNum, -1);
			printerr ("N-%2d: ", sessionNum++);
			do {
				if (DBUS_TYPE_STRING == (t = dbus_message_iter_get_arg_type(&args))) {
					dbus_message_iter_get_basic(&args, &parameter);
					printerr (" '%s'", parameter);
					switch (count) {
					case 1: // Type
						gtk_list_store_set (sess_store, &sess_iter, SESS_SESSIONTYPE, parameter, -1);
						if (!strcmp ("unix-gnome", parameter)) {
							icon = gdk_pixbuf_new_from_file(PACKAGE_DATA_DIR"/gnome-nx-session.png", &gerror);
							gtk_list_store_set (sess_store, &sess_iter, SESS_TYPEICON, icon, -1);
						} else if (!strcmp ("unix-kde", parameter)) {
							icon = gdk_pixbuf_new_from_file(PACKAGE_DATA_DIR"/kde-nx-session.png", &gerror);
							gtk_list_store_set (sess_store, &sess_iter, SESS_TYPEICON, icon, -1);
						} else {
							icon = gdk_pixbuf_new_from_file(PACKAGE_DATA_DIR"/unknown-nx-session.png", &gerror);
							gtk_list_store_set (sess_store, &sess_iter, SESS_TYPEICON, icon, -1);
						}
						break;
					case 2: // Session ID
						gtk_list_store_set (sess_store, &sess_iter, SESS_SESSIONID, parameter, -1);
						break;
					case 3: // Options
						gtk_list_store_set (sess_store, &sess_iter, SESS_OPTIONS, parameter, -1);
						break;
					case 5: // Screen
						gtk_list_store_set (sess_store, &sess_iter, SESS_SCREEN, parameter, -1);
						break;
					case 6: // Status
						gtk_list_store_set (sess_store, &sess_iter, SESS_AVAILABLE, parameter, -1);
						break;
					case 7: // Session Name
						gtk_list_store_set (sess_store, &sess_iter, SESS_SESSIONNAME, parameter, -1);
						break;
					default:
						break;
					}
					count++;

				} else if (t == DBUS_TYPE_INT32) {
					dbus_message_iter_get_basic (&args, &iparam);
					printerr (" d%d", iparam);
					//snprintf (parameter, 16, "%d", iparam);
					switch (count) {
					case 0:
						gtk_list_store_set (sess_store, &sess_iter, SESS_DISPLAY, iparam, -1);
						break;
					case 4: // Depth
						gtk_list_store_set (sess_store, &sess_iter, SESS_DEPTH, iparam, -1);
						break;
					default:
						break;
					}
					count++;

				} else {
					printerr ("NXLAUNCH> Error, parameter is not string or int.\n");
				}

			} while (dbus_message_iter_next (&args));

			printerr ("\n");
		
			/*
			 * The session box should now have been populated, so just reveal it
			 */
			widget = glade_xml_get_widget (xml_nxlaunch_glob, "session_list");
			gtk_widget_show (widget);
			while (gtk_events_pending()) gtk_main_iteration();
			rtn = REPLY_REQUIRED;

		} else if (dbus_message_is_signal (message,
						   dbusRecvInterface,
						   "NoMoreAvailable")) {
			printerr ("NXLAUNCH> Server says \"NoMoreAvailable\"\n");
			sessions_obtained = TRUE;
			rtn = REPLY_REQUIRED;

		} else if (dbus_message_is_signal (message,
						   dbusRecvInterface,
						   "Connecting")) {
			printerr ("NXLAUNCH> Server says \"Connection\"\n");
			sessions_obtained = TRUE;
			rtn = NEW_CONNECTION;

		} else if (dbus_message_is_signal (message,
						   dbusRecvInterface,
						   "ServerCapacityReached")) {
			printerr ("NXLAUNCH> Server says \"ServerCapacityReached\"\n");
			sessions_obtained = TRUE;
			rtn = SERVER_CAPACITY;

		} else {
			/* Nothing */
		}	

		dbus_connection_flush (conn);
		dbus_message_unref (message);

	} // while()

	return rtn;
}


/*!
 * This populates global variables
 */
int obtainDbusConnection (void)
{
	gboolean gotName = FALSE;
	gboolean gotSendName = FALSE;
	gboolean gotRecvName = FALSE;
	int i = 0, ret = 0;
	
	dbusSendInterface = g_malloc0 (64*sizeof (gchar));
	dbusRecvInterface = g_malloc0 (64*sizeof (gchar));
	dbusMatchString   = g_malloc0 (128*sizeof (gchar));

	/* Get a connection to the session bus */
	dbus_error_init (&dbusErr);
	dbusConn = dbus_bus_get (DBUS_BUS_SESSION, &dbusErr);
	if (!dbusConn) {
		printerr ("Failed to connect to the D-BUS daemon: %s\n", dbusErr.message);
		dbus_error_free (&dbusErr);
		return -1;
	}

	/*
	 * Here, we try to get the interface org.freenx.nxcl.client0
	 * in case any other clients are running. We also try to get
	 * the interface org.freenx.nxcl.nxcl0, in case any other nxcl
	 * daemons are running, but we have to release that name, one
	 * we established that it is available.
	 */
	while (gotName == FALSE) {

		snprintf (dbusSendInterface, 63, "org.freenx.nxcl.client%d", i);
		snprintf (dbusRecvInterface, 63, "org.freenx.nxcl.nxcl%d", i);
		snprintf (dbusMatchString, 127, "type='signal',interface='org.freenx.nxcl.nxcl%d'", i);

		// We request a name on the bus which is the same string as the send interface.
		ret = dbus_bus_request_name (dbusConn, dbusSendInterface,
					     DBUS_NAME_FLAG_REPLACE_EXISTING, 
					     &dbusErr);

		if (dbus_error_is_set (&dbusErr)) { 
			printerr ("NXLAUNCH> Name Error (%s)\n", dbusErr.message); 
			dbus_error_free(&dbusErr);
		}

		if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret) {
			printerr ("NXLAUNCH> Couldn't get the send name this iteration\n");
		} else {
			printerr ("NXLAUNCH> Got the send name '%s' on the dbus\n", dbusSendInterface);
			gotSendName = TRUE;
		}

		ret = dbus_bus_request_name (dbusConn, dbusRecvInterface,
					     DBUS_NAME_FLAG_REPLACE_EXISTING, 
					     &dbusErr);

		if (dbus_error_is_set (&dbusErr)) { 
			printerr ("NXLAUNCH> Name Error (%s)\n", dbusErr.message); 
			dbus_error_free(&dbusErr);
		}

		if (DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER != ret) {
			printerr ("NXLAUNCH> Couldn't get the receive name in this iteration\n");
		} else {
			printerr ("NXLAUNCH> Got the recv name '%s' on the dbus\n", dbusRecvInterface);
			gotRecvName = TRUE;
			dbus_bus_release_name (dbusConn, dbusRecvInterface, &dbusErr);
		}

		if (gotSendName && gotRecvName) {
			gotName = TRUE;
		} else {
			gotSendName = FALSE;
			gotRecvName = FALSE;
			i++;
		}
	}

	printerr ("NXLAUNCH> obtainDbusConnection() returning %d\n", i);
	return i;
}

void execNxcl (void)
{
	pid_t pid = 0;
	char argStr[8];
	char progLocn[128];
	char prog[32];
	int theError = 0;

	snprintf (argStr, 7, "%d", dbusNum);
	snprintf (progLocn, 127, PACKAGE_BIN_DIR"/nxcl");
	snprintf (prog, 31, "nxcl");

	printerr ("NXLAUNCH> About to exec '%s %s'\n", prog, argStr);

	/* fork and exec the nxcl */
	pid = fork();
	switch (pid) {
	case -1:
		printerr ("Can't fork()!\n");
		_exit(1);
	case 0:
		// This is the CHILD process
		// Allocate memory for the program arguments
		// 1+ to allow space for NULL terminating pointer
		

		// FIXME: This isn't passing the argument correctly to nxcl.
		execl (progLocn, prog, argStr, (char*)NULL);
		// If process returns, error occurred
		theError = errno;
		// This'll get picked up by parseResponse
		printerr ("NXLAUNCH> Process error: %d crashed. errno:%d\n", pid, theError);
		// Child should exit now.
		_exit(1);

	default:
		// This is the PARENT process
		printerr ("NXLAUNCH> forked the nxcl process; continuing.\n");
		break;
	}
}


void sendNxclSettings (struct nx_connection * nx_conn)
{
	/* Prepare interface - add a rule for which messages we want
	   to see - those that are sent to us from the nxcl
	   connection. */
	dbus_bus_add_match (dbusConn, dbusMatchString, &dbusErr);
	dbus_connection_flush (dbusConn);
	if (dbus_error_is_set(&dbusErr)) { 
		printerr ("NXLAUNCH> Match Error (%s)\n", dbusErr.message);
		_exit(1);
	} else {
		printerr ("NXLAUNCH> Added match '%s'\n", dbusMatchString);
	}

	if (waitForAlive() == FALSE) {
		printerr ("NXLAUNCH> nxcl didn't start!");
		_exit (-1);		
	}

	// ...and then send it on dbus.
	sendSettings (dbusConn, nx_conn);
	printerr ("NXLAUNCH> sent settings\n");
	
	return;
}

/*!
 * This is a callback, but is better placed here in the code near
 * getActiveConnection.
 */
void on_button_password_ok_clicked (GtkButton * button)
{
	struct nx_connection * nx_conn = NULL;
	GtkWidget * widget;

	printerr ("NXLAUNCH> %s() called\n", __FUNCTION__);

	if (nx_conn_glob == NULL) {
		nx_conn = g_malloc0 (sizeof (struct nx_connection));
		getActiveConnection (nx_conn);
	} else { /* we already populated nx_conn in launch_named_connection() */
		nx_conn = nx_conn_glob;
		nx_conn_glob = NULL;
	}

	/* Read the password into nx_conn->Pass, mallocing if necessary */
	widget = glade_xml_get_widget (xml_nxlaunch_glob, "entry_password_dialog");
	if (!(nx_conn->Pass = g_try_realloc (nx_conn->Pass, NX_FIELDLEN * sizeof(gchar)))) {
		printerr ("NXLAUNCH> Failed g_try_realloc\n");
	}
	if (strlen (gtk_entry_get_text (GTK_ENTRY(widget))) > 0) {
		strncpy (nx_conn->Pass, gtk_entry_get_text (GTK_ENTRY (widget)),  NX_FIELDLEN);
	} else {
		printerr ("NXLAUNCH> Zero length password\n");
		// FIXME: Need better user feedback here.
		return;
	}

	/* Hide the password window (this may not happen until this
	 * callback returns, which is not what we want, really) */
	widget = glade_xml_get_widget (xml_nxlaunch_glob, "dialog_password");
	gtk_widget_hide_all (widget);
	while (gtk_events_pending()) gtk_main_iteration();
	/* We still have an un-redrawn patch on the greyed out nxlauncher main window... */
	widget = glade_xml_get_widget (xml_nxlaunch_glob, "nxlauncher");
	gtk_widget_show_all (widget);
	while (gtk_events_pending()) gtk_main_iteration();
	launch_connection (nx_conn);
	return;
}

void on_button_password_cancel_clicked (GtkButton * button)
{
	GtkWidget * widget;
	widget = glade_xml_get_widget (xml_nxlaunch_glob, "nxlauncher");
	gtk_widget_set_sensitive (widget, TRUE);
	gtk_widget_show (widget);
	return;
}

void getActiveConnection (struct nx_connection * nx_conn)
{
	GtkTreeView * list_tree;
	GtkTreeSelection * selected;
	GtkTreeIter iter;
	GtkTreeModel * tree_model;

	printerr ("%s() called\n", __FUNCTION__);

	list_tree = GTK_TREE_VIEW (glade_xml_get_widget (xml_nxlaunch_glob, "treeview_nxconnection"));
	selected = gtk_tree_view_get_selection (list_tree);
	tree_model = GTK_TREE_MODEL (conn_store);

	if (gtk_tree_selection_get_selected (selected, &tree_model, &iter)) {

		/* Get all the information out of the row */
		gtk_tree_model_get (tree_model, &iter,
				    CONN_CONNECTIONNAME,    &nx_conn->ConnectionName,//
				    CONN_SERVERHOST,        &nx_conn->ServerHost,//
				    CONN_SERVERPORT,        &nx_conn->ServerPort,
				    CONN_USER,              &nx_conn->User,//
				    CONN_PASS,              &nx_conn->Pass,//
				    CONN_REMEMBERPASS,      &nx_conn->RememberPassword,
				    CONN_DISABLENODELAY,    &nx_conn->DisableNoDelay,
				    CONN_DISABLEZLIB,       &nx_conn->DisableZLIB,
				    CONN_ENABLESSLONLY,     &nx_conn->EnableSSLOnly,
				    CONN_LINKSPEED,         &nx_conn->LinkSpeed,//
				    CONN_PUBLICKEY,         &nx_conn->PublicKey,//
				    CONN_DESKTOP,           &nx_conn->Desktop,//
				    CONN_SESSION,           &nx_conn->Session,//
				    CONN_CUSTOMUNIXDESKTOP, &nx_conn->CustomUnixDesktop,//
				    CONN_COMMANDLINE,       &nx_conn->CommandLine,//
				    CONN_VIRTUALDESKTOP,    &nx_conn->VirtualDesktop,
				    CONN_XAGENTENCODING,    &nx_conn->XAgentEncoding,
				    CONN_USETAINT,          &nx_conn->UseTaint,
				    CONN_XDMMODE,           &nx_conn->XdmMode,//
				    CONN_XDMHOST,           &nx_conn->XdmHost,//
				    CONN_XDMPORT,           &nx_conn->XdmPort,
				    CONN_FULLSCREEN,        &nx_conn->FullScreen,	    
				    CONN_RESOLUTIONWIDTH,   &nx_conn->ResolutionWidth,
				    CONN_RESOLUTIONHEIGHT,  &nx_conn->ResolutionHeight,
				    CONN_GEOMETRY,          &nx_conn->Geometry,//
				    CONN_IMAGEENCODING,     &nx_conn->ImageEncoding,
				    CONN_JPEGQUALITY,       &nx_conn->JPEGQuality,
				    CONN_ENABLESOUND,       &nx_conn->enableSound,
				    CONN_IPPPORT,           &nx_conn->IPPPort,
				    CONN_IPPPRINTING,       &nx_conn->IPPPrinting,
				    CONN_SHARES,            &nx_conn->Shares,
				    CONN_AGENTSERVER,       &nx_conn->agentServer,//
				    CONN_AGENTUSER,         &nx_conn->agentUser,//
				    CONN_AGENTPASS,         &nx_conn->agentPass,//
				    -1);

		/* Put this iter in the global iter */
		conn_iter = iter;
	}
}

/*!
 * Returns true if we have a named connection, but we are waiting for
 * user to input a password.
 */
gboolean launch_named_connection (gchar * connection_name)
{
	/* Set the iter to point to the named connection, and if we
	 * have a match, called on_button_nxlaunch_launch_clicked */
	GtkWidget * widget;
	GtkWidget * tree;
	GtkTreeIter iter;
	gchar * str_data;
	gboolean valid;
	gboolean got_connection = FALSE;
	struct nx_connection * nx_conn;

	nx_conn = g_malloc0 (sizeof (struct nx_connection));

	/* Need to get the associated treemodel */
	tree = glade_xml_get_widget (xml_nxlaunch_glob, "treeview_nxconnection");
	valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (conn_store), &iter);
	while (valid) {
		gtk_tree_model_get (GTK_TREE_MODEL (conn_store), &iter, 
				    CONN_CONNECTIONNAME, &str_data,
				    -1);
		printerr ("Got connection named '%s'\n", str_data);
		if (!strcmp (connection_name, str_data)) {
			printerr ("This is the connection to activate\n");
			got_connection = TRUE;

			/* NB: This function mallocs the data for the strings */
			gtk_tree_model_get (GTK_TREE_MODEL(conn_store), &iter,
					    CONN_CONNECTIONNAME,    &nx_conn->ConnectionName,
					    CONN_SERVERHOST,        &nx_conn->ServerHost,
					    CONN_SERVERPORT,        &nx_conn->ServerPort,
					    CONN_USER,              &nx_conn->User,
					    CONN_PASS,              &nx_conn->Pass,
					    CONN_REMEMBERPASS,      &nx_conn->RememberPassword,
					    CONN_DISABLENODELAY,    &nx_conn->DisableNoDelay,
					    CONN_DISABLEZLIB,       &nx_conn->DisableZLIB,
					    CONN_ENABLESSLONLY,     &nx_conn->EnableSSLOnly,
					    CONN_LINKSPEED,         &nx_conn->LinkSpeed,
					    CONN_PUBLICKEY,         &nx_conn->PublicKey,
					    CONN_DESKTOP,           &nx_conn->Desktop,
					    CONN_SESSION,           &nx_conn->Session,
					    CONN_CUSTOMUNIXDESKTOP, &nx_conn->CustomUnixDesktop,
					    CONN_COMMANDLINE,       &nx_conn->CommandLine,
					    CONN_VIRTUALDESKTOP,    &nx_conn->VirtualDesktop,
					    CONN_XAGENTENCODING,    &nx_conn->XAgentEncoding,
					    CONN_USETAINT,          &nx_conn->UseTaint,
					    CONN_XDMMODE,           &nx_conn->XdmMode,
					    CONN_XDMHOST,           &nx_conn->XdmHost,
					    CONN_XDMPORT,           &nx_conn->XdmPort,
					    CONN_FULLSCREEN,        &nx_conn->FullScreen,	    
					    CONN_RESOLUTIONWIDTH,   &nx_conn->ResolutionWidth,
					    CONN_RESOLUTIONHEIGHT,  &nx_conn->ResolutionHeight,
					    CONN_GEOMETRY,          &nx_conn->Geometry,
					    CONN_IMAGEENCODING,     &nx_conn->ImageEncoding,
					    CONN_JPEGQUALITY,       &nx_conn->JPEGQuality,
					    CONN_ENABLESOUND,       &nx_conn->enableSound,
					    CONN_IPPPORT,           &nx_conn->IPPPort,
					    CONN_IPPPRINTING,       &nx_conn->IPPPrinting,
					    CONN_SHARES,            &nx_conn->Shares,
					    CONN_AGENTSERVER,       &nx_conn->agentServer,
					    CONN_AGENTUSER,         &nx_conn->agentUser,
					    CONN_AGENTPASS,         &nx_conn->agentPass,
					    -1);
			printerr ("nx_conn->ServerHost: %s\n", nx_conn->ServerHost);
			g_free (str_data);
			break;
		}
		g_free (str_data);
		valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (conn_store), &iter);
	}
	
	if (!got_connection) {
		printerr ("NXLAUNCH> Couldn't find the connection '%s'\n", connection_name);
		return FALSE;
	}

	if (nx_conn->RememberPassword == FALSE) {
		/* Show the window to enter the password */
		/* Set the global pointer to point to the memory we
		 * just malloced. */
		nx_conn_glob = nx_conn;
		widget = glade_xml_get_widget (xml_nxlaunch_glob, "dialog_password");
		gtk_widget_show_all (widget);
		return TRUE;

	} else {
		launch_connection (nx_conn);
		nx_connection_free (nx_conn);
		return FALSE;
	}
}

/*!
 * nx_conn should be malloced already when passed to this fn.
 * This function will then free that memory, and exit the program.
 */
void launch_connection (struct nx_connection * nx_conn)
{
	GtkWidget * widget;

	printerr ("%s() called\n", __FUNCTION__);

	execNxcl();
	sendNxclSettings (nx_conn);
	callReceiveSession (FALSE);
	printerr ("NXLAUNCH> Received session\n");

	if (!(nx_conn->Pass = g_try_realloc (nx_conn->Pass, NX_FIELDLEN * sizeof(gchar)))) {
		printerr ("NXLAUNCH> Failed g_try_realloc in %s()\n", __FUNCTION__);
	}
	memset (nx_conn->Pass, 0, NX_FIELDLEN);

	/* Clear the password GTK entry here where necessary */
	widget = glade_xml_get_widget (xml_nxlaunch_glob, "entry_nxlaunch_password");
	gtk_entry_set_text (GTK_ENTRY(widget), "");
	widget = glade_xml_get_widget (xml_nxlaunch_glob, "entry_password_dialog");
	gtk_entry_set_text (GTK_ENTRY(widget), "");

	nx_connection_free (nx_conn);
	return;
}

void nxlauncher_hide (void)
{
	GtkWidget * widget;
	widget = glade_xml_get_widget (xml_nxlaunch_glob, "nxlauncher");
	printerr ("NXLAUNCH> Hide all\n");
	gtk_widget_hide_all (widget);
	return;
}

void nxlauncher_show (void)
{
	GtkWidget * widget;
	widget = glade_xml_get_widget (xml_nxlaunch_glob, "nxlauncher");
	printerr ("NXLAUNCH> Show all\n");
	gtk_widget_show_all (widget);
	return;
}

int checkActiveConnectionPassword (void)
{
	GtkWidget * widget;
	GtkTreeView * list_tree;
	GtkTreeSelection * selected;
	GtkTreeIter iter;
	GtkTreeModel * tree_model;
	gchar * str;
	int flags = 0;

	/* Check active row in treeview */
	list_tree = GTK_TREE_VIEW (glade_xml_get_widget (xml_nxlaunch_glob, "treeview_nxconnection"));
	selected = gtk_tree_view_get_selection (list_tree);
	tree_model = GTK_TREE_MODEL (conn_store);
	if (gtk_tree_selection_get_selected (selected, &tree_model, &iter)) {
		gtk_tree_model_get (tree_model, &iter, CONN_PASS, &str, -1);		
		if (strlen (str) > 0) {
			flags |= STORED_PASSWORD;
		}
		g_free (str);
	}

	widget = glade_xml_get_widget (xml_nxlaunch_glob, "entry_nxlaunch_password");
	str = g_malloc0 (NX_FIELDLEN * sizeof (gchar));
	strncpy (str, gtk_entry_get_text (GTK_ENTRY(widget)), NX_FIELDLEN);
	if (strlen (str) > 0) {
		flags |= ENTERED_PASSWORD;
	}
	g_free (str);
	return flags;
}

/*
 * Likely to become obsolete.
 */
void nxlaunch_quit_with_window (void)
{
	printerr ("%s called\n", __FUNCTION__);
	GtkWidget * widget;
	printerr ("NXLAUNCH> Quitting in 8 seconds...\n");
	widget = glade_xml_get_widget (xml_nxlaunch_glob, "dialog_quitting");
	gtk_widget_show_all (widget);
	while (gtk_events_pending()) gtk_main_iteration();
	widget = glade_xml_get_widget (xml_nxlaunch_glob, "label_reason_for_exit");
	gtk_widget_show (widget);
	while (gtk_events_pending()) gtk_main_iteration();
	printerr ("About to sleep.. it would be better to pass control back to the main loop for a few seconds before quitting here.\n");
	usleep (8000000);
	printerr ("Slept, now quit..\n");
	while (gtk_events_pending()) gtk_main_iteration();
	gtk_main_quit();
}

void nxlaunch_display_progress (void)
{
	GtkWidget * widget;
	widget = glade_xml_get_widget (xml_nxlaunch_glob, "nxlaunch_progress");
	gtk_widget_show (widget);
	while (gtk_events_pending()) gtk_main_iteration();
	widget = glade_xml_get_widget (xml_nxlaunch_glob, "label_progress");
	gtk_widget_show (widget);
	/* Add a status message listener - this will update the status
	   message in the nxlaunch_progress window, and the messages will
	   be used to set the progress bar, too. */
	g_timeout_add (500, &nxlaunch_status_listener, NULL);
	return;
}

gboolean nxlaunch_status_listener (gpointer data)
{
	GtkWidget * widget;
	DBusMessage * message;
	DBusMessageIter args;
	DBusError error;
	char * parameter = NULL;
	dbus_int32_t iparam = 0, t = 0;

	//printerr (_("NXLAUNCH> Listening for status information\n"));

	dbus_error_init (&error);

	if (dbus_error_is_set(&error)) { 
		printerr ("NXLAUNCH> Name Error (%s)\n", error.message); 
		dbus_error_free(&error); 
	}

	// non blocking read of the next available message
	dbus_connection_read_write(dbusConn, 0);
	message = dbus_connection_pop_message(dbusConn);

	// return if no message
	if (NULL == message) { return TRUE; }

	if (dbus_message_is_signal (message, dbusRecvInterface, "InfoMessage")) {

		if (!dbus_message_iter_init (message, &args)) {
			printerr ("NXLAUNCH> That message had no arguments.\n");
			dbus_connection_flush (dbusConn);
			dbus_message_unref (message);
			return TRUE;
		}
		printerr ("(NXCL)> Info: ");
		do {
			if (DBUS_TYPE_STRING == (t = dbus_message_iter_get_arg_type(&args))) {
				dbus_message_iter_get_basic(&args, &parameter);
				printerr (" '%s'", parameter);
				widget = glade_xml_get_widget (xml_nxlaunch_glob, "label_progress");
				gtk_label_set_text (GTK_LABEL (widget), parameter);
			} else if (t == DBUS_TYPE_INT32) {
		 			dbus_message_iter_get_basic (&args, &iparam);
					printerr (" d%d", iparam);
					/* We used the integer passed with these progress signals to set the progress bar */
					widget = glade_xml_get_widget (xml_nxlaunch_glob, "progressbar");
					switch (iparam) {
					case 700: /* 'Got a session ID' */
						gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (widget), 0.125);
						break;
					case 702: /* 'Got a proxy IP' or 'All data will be SSL tunnelled' */
						if (gtk_progress_bar_get_fraction (GTK_PROGRESS_BAR (widget)) < 0.300) {
							gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (widget), 0.250);
						} else {
							gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (widget), 0.500);
						}
						break;
					case 706: /* 'Got an agent cookie' */
						gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (widget), 0.375);
						break;
					case 710: /* 'Session status is "running"' */
						gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (widget), 0.625);
						break;
					case 1000007: /* 'Starting NX session' */
						gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (widget), 0.750);
						break;
					case 1000001: /* 'nxproxy process started' */
						gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (widget), 0.825);
						break;
					case 287: /* 'The session has been started successfully' */
						gtk_progress_bar_set_fraction (GTK_PROGRESS_BAR (widget), 1.000);
						while (gtk_events_pending()) gtk_main_iteration();
						usleep (2000000);
						gtk_main_quit();
						break;
					default:
						break;
					}
			} else {
				printerr ("NXLAUNCH> Error, parameter is not string or int.\n");
			}

		} while (dbus_message_iter_next (&args));
		printerr ("\n");

	} else if (dbus_message_is_signal (message, dbusRecvInterface, "ErrorMessage")) {

		if (!dbus_message_iter_init (message, &args)) {
			printerr ("NXLAUNCH> That message had no arguments.\n");
			dbus_connection_flush (dbusConn);
			dbus_message_unref (message);
			return TRUE;
		}

		printerr ("(NXCL)> Error: ");
		do {
			if (DBUS_TYPE_STRING == (t = dbus_message_iter_get_arg_type(&args))) {
				dbus_message_iter_get_basic(&args, &parameter);
				printerr (" '%s'", parameter);
				widget = glade_xml_get_widget (xml_nxlaunch_glob, "label_progress");
				gtk_label_set_text (GTK_LABEL (widget), parameter);

			} else if (t == DBUS_TYPE_INT32) {
					dbus_message_iter_get_basic (&args, &iparam);
					printerr (" d%d", iparam);
			} else {
				printerr ("NXLAUNCH> Error, parameter is not string or int.\n");
			}

		} while (dbus_message_iter_next (&args));
		printerr ("\n");

	} else {
		/* Nothing */
	}	
	
	dbus_connection_flush (dbusConn);
	dbus_message_unref (message);

	return TRUE;
}

gboolean waitForAlive (void)
{
	DBusMessage * message = NULL;
	DBusMessageIter args;
	DBusError error;
	char * parameter = NULL;
	dbus_int32_t iparam = 0, t = 0;
	gboolean nxclAlive = FALSE;
	int i = 0;

	dbus_error_init (&error);

	while (nxclAlive == FALSE && i<60) {

		i++;

		if (dbus_error_is_set(&error)) { 
			printerr ("NXLAUNCH> Name Error (%s)\n", error.message); 
			dbus_error_free(&error); 
		}

		// non blocking read of the next available message
		dbus_connection_read_write(dbusConn, 0);
		message = dbus_connection_pop_message(dbusConn);

		// return if no message
		if (NULL == message) {
			usleep (1000000);
			continue;
		}

		if (dbus_message_is_signal (message, dbusRecvInterface, "InfoMessage")) {

			if (!dbus_message_iter_init (message, &args)) {
				printerr ("NXLAUNCH> That message had no arguments.\n");
				dbus_connection_flush (dbusConn);
				dbus_message_unref (message);
				continue;
			}

			do {
				if (DBUS_TYPE_STRING == (t = dbus_message_iter_get_arg_type(&args))) {
					dbus_message_iter_get_basic(&args, &parameter);
				} else if (t == DBUS_TYPE_INT32) {
		 			dbus_message_iter_get_basic (&args, &iparam);
					if (iparam == NXCL_ALIVE) {
						/* Set global variable to say we're alive */
						nxclAlive = TRUE;
					}
				} else {
					printerr ("NXLAUNCH> Error, parameter is not string or int.\n");
				}

			} while (dbus_message_iter_next (&args));

		} else {
			/* Nothing */
		}	
	}
	dbus_connection_flush (dbusConn);
	dbus_message_unref (message);

	if (i<60 && nxclAlive == TRUE) {
		return TRUE;
	} else {
		return FALSE;
	}
}

void nxlaunch_error_requiring_quit (const char * message)
{
	GtkWidget * widget;
	
	/* Grey out all the other windows */
	widget = glade_xml_get_widget (xml_nxlaunch_glob, "nxlauncher");
	gtk_widget_set_sensitive (widget, FALSE);
	widget = glade_xml_get_widget (xml_nxlaunch_glob, "session_list");
	gtk_widget_set_sensitive (widget, FALSE);
	widget = glade_xml_get_widget (xml_nxlaunch_glob, "nxlaunch_progress");
	gtk_widget_set_sensitive (widget, FALSE);
	widget = glade_xml_get_widget (xml_nxlaunch_glob, "dialog_password");
	gtk_widget_set_sensitive (widget, FALSE);
	while (gtk_events_pending()) gtk_main_iteration();

	/* Bring up the general error window, with quit button */
	widget = glade_xml_get_widget (xml_nxlaunch_glob, "unrecoverable_error_message");
	gtk_label_set_text (GTK_LABEL(widget), message);
	widget = glade_xml_get_widget (xml_nxlaunch_glob, "unrecoverable_error");
	gtk_widget_set_sensitive (widget, TRUE);
	gtk_widget_show_all (widget);

	return;
}
