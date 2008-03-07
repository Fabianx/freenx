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
#include <sys/stat.h>
#include <fcntl.h>
#include <locale.h>

#include "nxlaunch.h"

/*!
 * Global Variable Definitions
 */
//@{

/* Use globals for the xml tree */
GladeXML * xml_nxlaunch_glob; /* nxlaunch.glade */
GladeXML * xml_glob;          /* nxconfig.glade */

/* And globals for the lists of connections/sessions */
GtkListStore * conn_store;
GtkTreeIter    conn_iter;
GtkTreeIter    active_iter_glob;
guint          next_conn_type;
GtkListStore * sess_store;
GtkTreeIter    sess_iter;
guint          next_sess_type;

/* Use some globals for the dbus interface as these need to be available to our callbacks. */
gchar * dbusSendInterface;
gchar * dbusRecvInterface;
gchar * dbusMatchString;
int              dbusNum = 0;
DBusConnection * dbusConn = NULL;
DBusError        dbusErr;

/* The "mode" in which the program was started: "main", "from name",
 * "from file", "edit named", "edit file". See nxlaunch.h for
 * #defines */
int nxlaunch_mode = 0;
//@}

/*!
 * main program entry point
 */
gint
main (gint argc, gchar * argv[])
{
	GtkWidget * widget;
	gchar * our_locale;
	int n_conn = 0;

	bindtextdomain (PACKAGE, BINDTEXTDOMAIN);
	bind_textdomain_codeset (PACKAGE, "UTF-8");
	textdomain (PACKAGE);

	gtk_set_locale ();
	gtk_init (&argc, &argv);

	our_locale = setlocale (LC_ALL, NULL);

	xml_nxlaunch_glob = glade_xml_new (PACKAGE_DATA_DIR"/nxlaunch.glade", NULL, NULL);
	if (!xml_nxlaunch_glob) {
		printerr (_("Failed to load the glade XML file (nxlaunch.glade)\n"));
		return EXIT_FAILURE;
	}
	glade_xml_signal_autoconnect (xml_nxlaunch_glob);

#if BUILT_IN_CONFIGURATION==1
	xml_glob = glade_xml_new (PACKAGE_DATA_DIR"/nxconfig.glade", NULL, NULL);
	if (!xml_glob) {
		printerr (_("Failed to load the glade XML file (nxconfig.glade)\n"));
		return EXIT_FAILURE;
	}
	glade_xml_signal_autoconnect (xml_glob);
#endif

	dbusNum = obtainDbusConnection();

	/*
	 * See nxlaunch.h for the enums which relate to these list stores.
	 */
	conn_store = gtk_list_store_new (N_CONN_COLUMNS,		 
					 G_TYPE_STRING,
					 G_TYPE_STRING,
					 G_TYPE_UINT,
					 G_TYPE_STRING,
					 G_TYPE_STRING,
					 G_TYPE_BOOLEAN,
					 G_TYPE_BOOLEAN,
					 G_TYPE_BOOLEAN,
					 G_TYPE_BOOLEAN,
					 G_TYPE_STRING,
					 G_TYPE_STRING,
					 G_TYPE_STRING,
					 G_TYPE_STRING,
					 G_TYPE_STRING,
					 G_TYPE_STRING,
					 G_TYPE_BOOLEAN,
					 G_TYPE_BOOLEAN,
					 G_TYPE_BOOLEAN,
					 G_TYPE_STRING,
					 G_TYPE_STRING,
					 G_TYPE_UINT,
					 G_TYPE_BOOLEAN,
					 G_TYPE_UINT,
					 G_TYPE_UINT,
					 G_TYPE_STRING,
					 G_TYPE_UINT,
					 G_TYPE_UINT,
					 G_TYPE_BOOLEAN,
					 G_TYPE_UINT,
					 G_TYPE_BOOLEAN,
					 G_TYPE_BOOLEAN,
					 G_TYPE_STRING,
					 G_TYPE_STRING,
					 G_TYPE_STRING);

	sess_store = gtk_list_store_new (N_SESS_COLUMNS,
					 G_TYPE_INT,
					 G_TYPE_INT,
					 G_TYPE_STRING,
					 GDK_TYPE_PIXBUF,
					 G_TYPE_STRING,
					 G_TYPE_STRING,
					 G_TYPE_INT,
					 G_TYPE_STRING,
					 G_TYPE_STRING,
					 G_TYPE_STRING);

	/* Create the columns */
	nxlaunch_create_nxconnection_list ();
	nxlaunch_create_nxsession_list ();

	/* fill the models (and hence the lists) from the config file */
	n_conn = nxconnection_buildlist();
	printerr (_("Found %d connections in config directory.\n"), n_conn);

	/* initialise active_iter_glob and conn_iter */
	gtk_tree_model_get_iter_first (GTK_TREE_MODEL (conn_store), &active_iter_glob);
	gtk_tree_model_get_iter_first (GTK_TREE_MODEL (conn_store), &conn_iter);

	gtk_tree_model_get_iter_first (GTK_TREE_MODEL (sess_store), &sess_iter);

	/*
	 * The following needs to be more sophisticated, with libpopt
	 * being used. We'll then set modes from the options. Going to
	 * have these options: --edit|-e --username|-u --file|-f Could
	 * also have options to mod the chosen connection, like
	 * --fullscreen etc.
	 */
	gboolean have_named = FALSE;
	if (argc == 2) {
		/* If we have an argument, we want to launch a connection based on name */
		nxlaunch_mode = MODE_NAMED;
		have_named = launch_named_connection (argv[1]);
	} else {
		nxlaunch_mode = MODE_MAIN;
	}

	/*
	 * If we have a connection named on the cmd line, we just want
	 * to show the password dialog - that means we need to avoid
	 * showing the nxlauncher window, BUT we do need to "realise"
	 * it, so that the tree model containing the connections is
	 * accessible.
	 */

	if (!have_named) {
		/* Unmask the main window */
		widget = glade_xml_get_widget (xml_nxlaunch_glob, "nxlauncher");
		gtk_widget_show_all (widget);

		/*
		 * FIXME: Now we should highlight the first connection
		 * in the list.
		 */

	} else {
		widget = glade_xml_get_widget (xml_nxlaunch_glob, "treeview_nxconnection");
		gtk_widget_realize (widget);
	}

	gtk_main();
	return EXIT_SUCCESS;
}
