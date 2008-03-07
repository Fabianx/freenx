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

/*! \file callbacks_nx.h
 *
 * Functions to do with the configuration of NoMachine NX connections.
 * Contains both the callback functions relating to the NX
 * configuration pop-up and also functions which deal with adding
 * and removing connections from the NX config files (~/.nx/config/nxclient.conf 
 * and ~/.nx/config/perConnection.nxs)
 *
 */


#ifndef __CALLBACKS_NX__
#define __CALLBACKS_NX__
#include "../src/i18n.h"

#ifdef DEBUG
# if DEBUG==0
# undef DEBUG
# endif
#endif

#ifndef PRINTERR_DEFINED
# define PRINTERR_DEFINED 1
# ifdef DEBUG
#  define printerr(args...)	fprintf(stderr, ## args);
# else
#  define printerr(args...)
# endif
#endif

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <glib.h>

#include <gtk/gtk.h>
#include <sys/stat.h>

#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

/*!
 * struct to hold data on nx connection. The names of
 * the elements of this structure are the same as the 
 * names they have in the connection config files
 *
 * With NX_FIELDLEN = 255, and the two extra size fields
 * this structure will take up about 7 kB of memory. 
 *
 * contents of [] brackets are the descriptions of the controls
 * in the GUI.
 *
 * This structure is related to NXConfigData in nxcl/nxcl, but not
 * quite the same. The NXConfigData equivalents are indicated in
 * comments.
 */
struct nx_connection {

	/* The name of the session, this gives the filename of the nx config file */
	gchar * ConnectionName;         // NXCL:sessionName

	/* Configurable variables in the network page */
	gchar * ServerHost;             // NXCL:serverHost
	guint ServerPort;               // NXCL:serverPort
	gchar * User;                   // NXCL:sessionUser
	gchar * Pass;                   // NXCL:sessionPass
	gboolean RememberPassword;      // NXCL:?
	gboolean DisableNoDelay;        // NXCL:?
	gboolean DisableZLIB;           // NXCL:?
	gboolean EnableSSLOnly;         // NXCL:encryption == EnableSSLOnly
	gchar * LinkSpeed;              // NXCL:linkType
	gchar * PublicKey; /* Needs more than 255 chars */ // NXCL:key

	/* Configurable variables in the desktop page */
	gchar * Desktop;                // NXCL:sessionType
	gchar * Session;                // New - matches XML file.
	gchar * CustomUnixDesktop;      // NXCL:in customCommand
	gchar * CommandLine;            // NXCL:customCommand and sessionType?
	gboolean VirtualDesktop; /* if false, then "Floating Window" is true */ // NXCL:
	gboolean XAgentEncoding;        // NXCL:?
	gboolean UseTaint;              // NXCL:?
	gchar * XdmMode;                // NXCL:?
	gchar * XdmHost;                // NXCL:?agentServer?
	guint XdmPort;                  // NXCL:?
	gboolean FullScreen;            // NXCL:fullscreen
	guint ResolutionWidth;          // NXCL:in string geometry
	guint ResolutionHeight;         // NXCL:in string geometry
	gchar * Geometry;               // NXCL:geometry
	guint ImageEncoding;	/* nx_conn->ImageEncoding: 0:Default
	                           1:PNG
	                           2:JPEG
                                   3:XBM   */ // NXCL:imageCompressionMethod
	guint JPEGQuality;                    // NXCL:imageCompressionLevel

	/* Services */
	gboolean enableSound;   /* If true, nxesd will be started */ // NXCL:media
	guint IPPPort;         // NXCL:?
	gboolean IPPPrinting;  // NXCL:?cups?
	gboolean Shares;       // NXCL:?
	gchar * agentServer;
	gchar * agentUser;
	gchar * agentPass;
};
/* We allow up to NX_FIELDLEN characters in the strings above, except for SSLKey */
#define NX_FIELDLEN 255
#define SSLKEYLEN 4096 /* Guess at large enough value.
			  Apparently this is about 10 times as big as it needs to be */
#define CMDLINELEN 1024 /* Allow 1023 char length command lines. */

/* Error return codes used in read_nx_connection and write_nx_connection */
#define NX_XML_NOT_PARSED -1
#define NX_XML_EMPTY -2
#define NX_XML_WRONG_ROOT_NODE -3 /* document of the wrong type, 
				     root node != NXClientSettings */

/* This enum is used setting up some combo boxes */
enum { PROGRAM, N_COLS };

/* The text to place in wfclient.ini when there isn't already one existing. */
static const gchar nxclient_conf[] = 
"<!DOCTYPE NXClientSettings>\n"
"<NXClientSettings application=\"nxclient\" version=\"1.3\" >\n"
"<group name=\"General\" >\n"
"<option key=\"CUPS Password\" value=\"\" />\n"
"<option key=\"CUPS Port\" value=\"0\" />\n"
"<option key=\"Default font\" value=\"Helvetica,10,-1,5,50,0,0,0,0,0\" />\n"
"<option key=\"Hide Full Screen Info\" value=\"false\" />\n"
"<option key=\"Last session\" value=\"Nx1\" />\n"
"<option key=\"Permit Root Login\" value=\"false\" />\n"
"<option key=\"Personal NX dir\" value=\"~/.nx\" />\n"
"<option key=\"Remove old sessions\" value=\"true\" />\n"
"<option key=\"System NX dir\" value=\"/opt/nx\" />\n"
"</group>\n"
"</NXClientSettings>\n";

static const gchar NXDefaultKey[] =
"-----BEGIN DSA PRIVATE KEY-----\n"
"MIIBuwIBAAKBgQCXv9AzQXjxvXWC1qu3CdEqskX9YomTfyG865gb4D02ZwWuRU/9\n"
"C3I9/bEWLdaWgJYXIcFJsMCIkmWjjeSZyTmeoypI1iLifTHUxn3b7WNWi8AzKcVF\n"
"aBsBGiljsop9NiD1mEpA0G+nHHrhvTXz7pUvYrsrXcdMyM6rxqn77nbbnwIVALCi\n"
"xFdHZADw5KAVZI7r6QatEkqLAoGBAI4L1TQGFkq5xQ/nIIciW8setAAIyrcWdK/z\n"
"5/ZPeELdq70KDJxoLf81NL/8uIc4PoNyTRJjtT3R4f8Az1TsZWeh2+ReCEJxDWgG\n"
"fbk2YhRqoQTtXPFsI4qvzBWct42WonWqyyb1bPBHk+JmXFscJu5yFQ+JUVNsENpY\n"
"+Gkz3HqTAoGANlgcCuA4wrC+3Cic9CFkqiwO/Rn1vk8dvGuEQqFJ6f6LVfPfRTfa\n"
"QU7TGVLk2CzY4dasrwxJ1f6FsT8DHTNGnxELPKRuLstGrFY/PR7KeafeFZDf+fJ3\n"
"mbX5nxrld3wi5titTnX+8s4IKv29HJguPvOK/SI7cjzA+SqNfD7qEo8CFDIm1xRf\n"
"8xAPsSKs6yZ6j1FNklfu\n"
"-----END DSA PRIVATE KEY-----";


/*! Utility functions */
gboolean set_combobox (GtkWidget * widget, gchar * match_string);
void nx_general_error (const gchar * msg, const gchar * title);

/* Fn declarations for NX file access */
gint write_nx_connection (struct nx_connection * nx_conn);
gint delete_nx_connection (gchar * name);
gint replace_nx_connection (struct nx_connection * nx_conn, gchar * name);

/*! 
 * Used in read_nx_connection
 */
void parse_general (xmlDocPtr doc, xmlNodePtr cur, 
		    struct nx_connection * nx_conn);

void parse_advanced (xmlDocPtr doc, xmlNodePtr cur, 
		     struct nx_connection * nx_conn);

gint read_default_image_encoding (xmlDocPtr doc, xmlNodePtr cur);
gint find_def_im_enc_in_general (xmlDocPtr doc, xmlNodePtr cur);
void parse_images (xmlDocPtr doc, xmlNodePtr cur, 
		   struct nx_connection * nx_conn);

void parse_login (xmlDocPtr doc, xmlNodePtr cur, 
		  struct nx_connection * nx_conn);

/*!
 * Read an entry in theConn.nxs, putting details into the nx_connection struct.
 *
 * Returns 0 if a match is found and read. -1 on error, -2 for no match.
 */
gint read_nx_connection (struct nx_connection * nx_conn, const gchar * name);


/* Memory management for nx_connection structures */
struct nx_connection * nx_connection_malloc (void);
void nx_connection_zero (struct nx_connection * nx_conn);
gint nx_connection_free (struct nx_connection * nx_conn);

/*! 
 * This does the real work for setup_new_nx_popup() and 
 * zero_new_nx_popup ()
 */
void setup_nx_popup (struct nx_connection * nx_conn);

/*!
 * setup_new_nx_popup () fills in the fields in the NX
 * connection configuration popup (conn_new_nx in glade)
 * using the entry in the config file which has the name
 * conn->name.
 */
void setup_new_nx_popup (const gchar * name);

/*!
 * Resets fields in the NX
 * connection configuration popup (conn_new_nx in glade)
 * to zero/defaults.
 */
void zero_new_nx_popup (void);

/*!
 * This function sets up the connection structure with data taken
 * from the submitted form data in the conn_new_nx dialog.
 *
 * It also sets up the data in nx_conn (which will be written to 
 * the appsrv.ini file) with values taken from the conn_new_nx dialog.
 */
void read_nx_popup (gchar * name, gchar * server, gchar * uname, int * fullscreen,
		    struct nx_connection * nx_conn);

/*!
 * This checks for the existence of the NX configuration
 * files (in ~/.nx/config). If not present, it initialises
 * the files using nx_writeout_nxclient_conf ().
 */
void nx_check_config (void);
gint nx_writeout_nxclient_conf (void);


/*!
 * Callbacks which handle greying/ungreying of fields when the comboboxes
 * and checkboxes in the NX popup window are manipulated
 */

/* Desktop Sessions */
void set_custom_settings_sensitivity (void);
void grey_all_custom_settings (void);
void ungrey_custom_options (void);

void set_xdm_settings (void);
int set_xdm_settings_sensitivity_do_work (void);
void set_xdm_settings_sensitivity (void);
void grey_all_xdm_settings (void);

void on_combobox_nx_desktop_session_changed (GtkComboBox * box);
void on_combobox_nx_xdm_mode_changed (GtkComboBox * box);
void on_radiobutton_nx_new_virtual_desktop_toggled (GtkToggleButton * button);
void on_checkbutton_nx_disable_x_agent_toggled (GtkToggleButton * button);

void on_combobox_new_nx_window_size_changed (GtkComboBox * box);

void on_combobox_nx_image_encoding_changed (GtkComboBox * box);

void on_button_nx_revert_ssl_key_clicked (GtkButton * button);

void on_nx_key_file_chosen (GtkButton * button);

#endif
