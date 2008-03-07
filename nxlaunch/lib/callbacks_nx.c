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

#include "callbacks_nx.h"

#include <unistd.h>
#include <string.h>
#include <time.h>

#include <glade/glade.h>

/* Use libxml to write out the configuration file */
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

extern GladeXML * xml_glob;

/* A global variable for locking. */
gboolean can_update_nx_settings = TRUE;

extern int xmlIndentTreeOutput;

/*!
 * A useful utility function to set the value of a combobox.
 */
gboolean set_combobox (GtkWidget * widget, gchar * match_string)
{
	GtkListStore * prog_list;
	GtkTreeIter iter;
	GtkTreeIter new_iter;
	gchar * str_data;
	gboolean valid;
	gboolean have_match = FALSE;

	prog_list = GTK_LIST_STORE (gtk_combo_box_get_model (GTK_COMBO_BOX (widget)));
	valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (prog_list), &iter);

	while (valid) {
		gtk_tree_model_get (GTK_TREE_MODEL (prog_list), &iter, 
				    PROGRAM, &str_data, -1);
		if (strstr (match_string, str_data)) {
			new_iter = iter;
			have_match = TRUE;
		}
		g_free (str_data);
		valid = gtk_tree_model_iter_next (GTK_TREE_MODEL (prog_list), &iter);
	}

	/* new_iter undefined if no match is found */
	if (have_match == TRUE) {
		gtk_combo_box_set_active_iter (GTK_COMBO_BOX (widget), &new_iter);		
	} else {
		/* do nothing */
	}

	return have_match;
}

/*!
 * Utility function. Places an error in a glade window which you must
 * define in the glade file.
 */
void nx_general_error (const gchar * msg, const gchar * title)
{
	GtkWidget * widget;
	widget = glade_xml_get_widget (xml_glob, "label_general_error");
	gtk_label_set_text (GTK_LABEL(widget), msg);
	widget = glade_xml_get_widget (xml_glob, "dialog_general_error");
	gtk_window_set_title (GTK_WINDOW(widget), title);
	gtk_widget_show (widget);
	return;
}

/*!
 * Reads the password from the XML file into xmlChar * password, so
 * that we can re-write it out into our re-created XML file.
 *
 * FIXME:
 * This is a quick and dirty solution to remembering the password that
 * the NX Client sets. Ideally, write_nx_connection should read in the
 * XML tree from file if it exists and then only change/replace those 
 * nodes that it needs to.
 */
gint get_nx_password (gchar * password, gchar * filename)
{
	xmlDocPtr doc;
	xmlNodePtr n1, n2;
	xmlChar * group_name;
	xmlChar * option_key;

	printerr ("%s called\n", __FUNCTION__);

	doc = xmlParseFile (filename);

	if (doc == NULL ) {
		/* document not parsed successfully */
		printerr ("doc not parsed\n");
		return NX_XML_NOT_PARSED;
	}

	n1 = xmlDocGetRootElement (doc);

	if (n1 == NULL) {
		/* empty document */
		printerr ("doc is empty\n");
		xmlFreeDoc (doc);
		return NX_XML_EMPTY;
	}

	if (xmlStrcmp (n1->name, BAD_CAST "NXClientSettings")) {
		/* document of the wrong type, root node != NXClientSettings */
		printerr ("doc wrong root node\n");
		xmlFreeDoc (doc);
		return NX_XML_WRONG_ROOT_NODE;
	}

	n1 = n1->xmlChildrenNode;
	printerr ("while no 1\n");
	while (n1 != NULL) {
		if ((!xmlStrcmp (n1->name, BAD_CAST "group"))){

			printerr ("group match!\n");
			group_name = xmlGetProp (n1, BAD_CAST "name");
			if ((!xmlStrcmp (group_name, BAD_CAST "Login"))) {

				printerr ("Login match!\n");
				n2 = n1->xmlChildrenNode;
				printerr ("while no 2\n");
				while (n2 != NULL) {
					if ((!xmlStrcmp (n2->name, BAD_CAST "option"))) {
							
						option_key = xmlGetProp (n2, BAD_CAST "key");
						if ((!xmlStrcmp (option_key, BAD_CAST "Auth"))) {
							printerr ("Auth match!\n");
							/* This is the password */
							strncpy (password, 
								 (gchar *)xmlGetProp (n2, BAD_CAST "value"), 
								 NX_FIELDLEN-1); /* 254 chars probably enough */
						}
						xmlFree (option_key);
					}
					n2 = n2->next;
				}
			}
			xmlFree (group_name);
		}
		n1 = n1->next;
	}
	xmlFreeDoc (doc);
	printerr ("%s returning\n", __FUNCTION__);
	return 0;
}



/* Add a connection file (theConnectionName.nxs) */
gint write_nx_connection (struct nx_connection * nx_conn)
{
	struct stat * buf;
	FILE * fp;
	gchar connection_file[1024];
	int rtn = 0;

	gboolean new_config_file = TRUE;

	/* for generation of the xml file */
	xmlDocPtr doc;
	xmlNodePtr docRoot;
	xmlNodePtr group;
	xmlNodePtr optionNode;
	xmlDtdPtr dtd = NULL; 
	char str[16];
	char* homepath;
	gchar path[1024];
	gchar * password = NULL;

	printerr ("%s called\n", __FUNCTION__);

	homepath = getenv("HOME");
	snprintf (path, 1023, "%s/.nx", homepath);
	mkdir (path, 0770);
	snprintf (path, 1023, "%s/.nx/config", homepath);
	mkdir (path, 0770);

	/* 
	 * Check we have the required general config files
	 */
	buf = g_malloc0 (sizeof (struct stat));
	snprintf (path, 1023, "%s/.nx/config/nxclient.conf", homepath);	
	stat (path, buf);
	if (!S_ISREG (buf->st_mode)) {
		fp = fopen (path, "w");
		if (fp) {
			fwrite (nxclient_conf, sizeof (gchar), strlen (nxclient_conf), fp);
			fclose (fp);
		} else {
			/* Calling fn generates popup */
			/*nx_general_error (_("Couldn't create file ~/.nx/config/nxclient.conf. Please check disk space"), 
			  _("Can't create file"));*/
			return -1;
		}
	}
	g_free (buf);

	/*
	 * Generate the filename which the connection will be written into
	 */
	snprintf (connection_file, 1023, "%s/.nx/config/%s.nxs",
		 homepath, nx_conn->ConnectionName);


	/* Read the file in, if it exists, and take the values of Auth, the password, which
	   may have been written by the nxclient. Then only need to create the tree if 
	   we had no file to read in. */
	password = g_malloc0 (NX_FIELDLEN * sizeof (gchar));
	get_nx_password (password, connection_file);
	if (strlen (password) <= 0) {
		strncpy (password, "EMPTY_PASSWORD", NX_FIELDLEN-1);
	}

	/*
	 * Create and populate the XML config file....
	 */

	/* create new xml document with XML version 1.0 */

	/*if (new_config_file == TRUE) {*/
	doc = xmlNewDoc (BAD_CAST "1.0");
	docRoot = xmlNewNode (NULL, BAD_CAST "NXClientSettings");
	xmlDocSetRootElement(doc, docRoot);
	xmlNewProp (docRoot, BAD_CAST "application", BAD_CAST "nxclient");
	xmlNewProp (docRoot, BAD_CAST "version", BAD_CAST "1.3"); 

	dtd = xmlCreateIntSubset(doc, BAD_CAST "NXClientSettings", NULL, NULL);

	/*} else { alternative }*/

	/* 
	 * create the first group, "Advanced" 
	 */
	group = xmlNewChild (docRoot, NULL, BAD_CAST "group", NULL);
	xmlNewProp (group, BAD_CAST "name", BAD_CAST "Advanced");

	/* hardcoded */
	if (new_config_file == TRUE) {
		optionNode = xmlNewChild (group, NULL, BAD_CAST "option", NULL);
		xmlNewProp (optionNode, BAD_CAST "key", BAD_CAST "Cache size");
		xmlNewProp (optionNode, BAD_CAST "value", BAD_CAST "8");

		/* Hardcode a zero cache size on disk for now */
		optionNode = xmlNewChild (group, NULL, BAD_CAST "option", NULL);
		xmlNewProp (optionNode, BAD_CAST "key", BAD_CAST "Cache size on disk");
		xmlNewProp (optionNode, BAD_CAST "value", BAD_CAST "0");

		optionNode = xmlNewChild (group, NULL, BAD_CAST "option", NULL);
		xmlNewProp (optionNode, BAD_CAST "key", BAD_CAST "Current keyboard");
		xmlNewProp (optionNode, BAD_CAST "value", BAD_CAST "true");

		optionNode = xmlNewChild (group, NULL, BAD_CAST "option", NULL);
		xmlNewProp (optionNode, BAD_CAST "key", BAD_CAST "Custom keyboard layout");
		xmlNewProp (optionNode, BAD_CAST "value", BAD_CAST "us");

		optionNode = xmlNewChild (group, NULL, BAD_CAST "option", NULL);
		xmlNewProp (optionNode, BAD_CAST "key", BAD_CAST "Restore cache");
		xmlNewProp (optionNode, BAD_CAST "value", BAD_CAST "true");

		/* This options seems never to change (deprecated in nx 1.5 perhaps?) */
		optionNode = xmlNewChild (group, NULL, BAD_CAST "option", NULL);
		xmlNewProp (optionNode, BAD_CAST "key", BAD_CAST "StreamCompression");
		xmlNewProp (optionNode, BAD_CAST "value", BAD_CAST "");
	}
	/* configurable */
	optionNode = xmlNewChild (group, NULL, BAD_CAST "option", NULL);
	xmlNewProp (optionNode, BAD_CAST "key", BAD_CAST "Disable ZLIB stream compression");
	xmlNewProp (optionNode, BAD_CAST "value", 
		    nx_conn->DisableZLIB ? BAD_CAST "true" : BAD_CAST "false");

	optionNode = xmlNewChild (group, NULL, BAD_CAST "option", NULL);
	xmlNewProp (optionNode, BAD_CAST "key", BAD_CAST "Disable TCP no-delay");
	xmlNewProp (optionNode, BAD_CAST "value",  
		    nx_conn->DisableNoDelay ? BAD_CAST "true" : BAD_CAST "false");

	optionNode = xmlNewChild (group, NULL, BAD_CAST "option", NULL);
	xmlNewProp (optionNode, BAD_CAST "key", BAD_CAST "Enable SSL encryption");
	xmlNewProp (optionNode, BAD_CAST "value",  
		    nx_conn->EnableSSLOnly ? BAD_CAST "true" : BAD_CAST "false");


	/* 
	 * create the second group, "Environment" 
	 */
	if (new_config_file == TRUE) {
		group = xmlNewChild (docRoot, NULL, BAD_CAST "group", NULL);
		xmlNewProp (group, BAD_CAST "name", BAD_CAST "Environment");
		/* single option for Environment group */
		optionNode = xmlNewChild (group, NULL, BAD_CAST "option", NULL);
		xmlNewProp (optionNode, BAD_CAST "key", BAD_CAST "CUPSD path");
		xmlNewProp (optionNode, BAD_CAST "value", BAD_CAST "/usr/sbin/cupsd");
	}
	/* 
	 * create the third group, "General" 
	 */
	group = xmlNewChild (docRoot, NULL, BAD_CAST "group", NULL);
	xmlNewProp (group, BAD_CAST "name", BAD_CAST "General");

	/* hardcoded options */
	if (new_config_file == TRUE) {
		/* FIXME: This needs to be fixed to match the system
		   it was compiled for */
		optionNode = xmlNewChild (group, NULL, BAD_CAST "option", NULL);
		xmlNewProp (optionNode, BAD_CAST "key", BAD_CAST "CUPSD path");
		xmlNewProp (optionNode, BAD_CAST "value", BAD_CAST "/usr/sbin/cupsd");

		optionNode = xmlNewChild (group, NULL, BAD_CAST "option", NULL);
		xmlNewProp (optionNode, BAD_CAST "key", BAD_CAST "Automatic reconnect");
		xmlNewProp (optionNode, BAD_CAST "value", BAD_CAST "true");

		optionNode = xmlNewChild (group, NULL, BAD_CAST "option", NULL);
		xmlNewProp (optionNode, BAD_CAST "key", BAD_CAST "Backingstore");
		xmlNewProp (optionNode, BAD_CAST "value", BAD_CAST "when_requested");

		optionNode = xmlNewChild (group, NULL, BAD_CAST "option", NULL);
		xmlNewProp (optionNode, BAD_CAST "key", BAD_CAST "Use render");
		xmlNewProp (optionNode, BAD_CAST "value", BAD_CAST "true");

		optionNode = xmlNewChild (group, NULL, BAD_CAST "option", NULL);
		xmlNewProp (optionNode, BAD_CAST "key", BAD_CAST "displaySaveOnExit");
		xmlNewProp (optionNode, BAD_CAST "value", BAD_CAST "true");
	}

	/* options which are set via our interface */
	optionNode = xmlNewChild (group, NULL, BAD_CAST "option", NULL);
	xmlNewProp (optionNode, BAD_CAST "key", BAD_CAST "Desktop");
	xmlNewProp (optionNode, BAD_CAST "value", BAD_CAST nx_conn->Desktop);

	optionNode = xmlNewChild (group, NULL, BAD_CAST "option", NULL);
	xmlNewProp (optionNode, BAD_CAST "key", BAD_CAST "Session");
	xmlNewProp (optionNode, BAD_CAST "value", BAD_CAST nx_conn->Session);
	
	optionNode = xmlNewChild (group, NULL, BAD_CAST "option", NULL);
	xmlNewProp (optionNode, BAD_CAST "key", BAD_CAST "Custom Unix Desktop");
	xmlNewProp (optionNode, BAD_CAST "value", BAD_CAST nx_conn->CustomUnixDesktop);
	
	optionNode = xmlNewChild (group, NULL, BAD_CAST "option", NULL);
	xmlNewProp (optionNode, BAD_CAST "key", BAD_CAST "Command line");
	xmlNewProp (optionNode, BAD_CAST "value", BAD_CAST nx_conn->CommandLine);
	
	optionNode = xmlNewChild (group, NULL, BAD_CAST "option", NULL);
	xmlNewProp (optionNode, BAD_CAST "key", BAD_CAST "Link speed");
	xmlNewProp (optionNode, BAD_CAST "value", BAD_CAST nx_conn->LinkSpeed);
	
	optionNode = xmlNewChild (group, NULL, BAD_CAST "option", NULL);
	xmlNewProp (optionNode, BAD_CAST "key", BAD_CAST "Remember password");
	xmlNewProp (optionNode, BAD_CAST "value", 
		    nx_conn->RememberPassword ? BAD_CAST "true" : BAD_CAST "false" );
	
	/* We _could_ offer "Available Area" here */
	sprintf (str, "%dx%d", nx_conn->ResolutionWidth, nx_conn->ResolutionHeight);
	optionNode = xmlNewChild (group, NULL, BAD_CAST "option", NULL);
	xmlNewProp (optionNode, BAD_CAST "key", BAD_CAST "Resolution");
	xmlNewProp (optionNode, BAD_CAST "value", 
		    nx_conn->FullScreen ? BAD_CAST "fullscreen" : BAD_CAST str );

	sprintf (str, "%d", nx_conn->ResolutionWidth);
	optionNode = xmlNewChild (group, NULL, BAD_CAST "option", NULL);
	xmlNewProp (optionNode, BAD_CAST "key", BAD_CAST "Resolution width");
	xmlNewProp (optionNode, BAD_CAST "value", BAD_CAST str); 

	sprintf (str, "%d", nx_conn->ResolutionHeight);
	optionNode = xmlNewChild (group, NULL, BAD_CAST "option", NULL);
	xmlNewProp (optionNode, BAD_CAST "key", BAD_CAST "Resolution height");
	xmlNewProp (optionNode, BAD_CAST "value", BAD_CAST str); 

	optionNode = xmlNewChild (group, NULL, BAD_CAST "option", NULL);
	xmlNewProp (optionNode, BAD_CAST "key", BAD_CAST "Server host");
	xmlNewProp (optionNode, BAD_CAST "value", BAD_CAST nx_conn->ServerHost); 

	sprintf (str, "%d", nx_conn->ServerPort);
	optionNode = xmlNewChild (group, NULL, BAD_CAST "option", NULL);
	xmlNewProp (optionNode, BAD_CAST "key", BAD_CAST "Server port");
	xmlNewProp (optionNode, BAD_CAST "value", BAD_CAST str); 

	optionNode = xmlNewChild (group, NULL, BAD_CAST "option", NULL);
	xmlNewProp (optionNode, BAD_CAST "key", BAD_CAST "Use default image encoding");
	xmlNewProp (optionNode, BAD_CAST "value", 
		    nx_conn->ImageEncoding == 0 ? BAD_CAST "0" : BAD_CAST "1");

	optionNode = xmlNewChild (group, NULL, BAD_CAST "option", NULL);
	xmlNewProp (optionNode, BAD_CAST "key", BAD_CAST "Use taint");
	xmlNewProp (optionNode, BAD_CAST "value", 
		    nx_conn->UseTaint ? BAD_CAST "true" : BAD_CAST "false" );

	optionNode = xmlNewChild (group, NULL, BAD_CAST "option", NULL);
	xmlNewProp (optionNode, BAD_CAST "key", BAD_CAST "Virtual desktop");
	xmlNewProp (optionNode, BAD_CAST "value", 
		    nx_conn->VirtualDesktop ? BAD_CAST "true" : BAD_CAST "false" );

	optionNode = xmlNewChild (group, NULL, BAD_CAST "option", NULL);
	xmlNewProp (optionNode, BAD_CAST "key", BAD_CAST "XAgent encoding");
	xmlNewProp (optionNode, BAD_CAST "value", 
		    nx_conn->XAgentEncoding ? BAD_CAST "true" : BAD_CAST "false" );

	/* the xdm entries */
	optionNode = xmlNewChild (group, NULL, BAD_CAST "option", NULL);
	xmlNewProp (optionNode, BAD_CAST "key", BAD_CAST "xdm mode");
	xmlNewProp (optionNode, BAD_CAST "value", BAD_CAST nx_conn->XdmMode);

	sprintf (str, "%d", nx_conn->XdmPort);
	optionNode = xmlNewChild (group, NULL, BAD_CAST "option", NULL);
	xmlNewProp (optionNode, BAD_CAST "key", BAD_CAST "xdm list port");
	xmlNewProp (optionNode, BAD_CAST "value", BAD_CAST str);
	optionNode = xmlNewChild (group, NULL, BAD_CAST "option", NULL);
	xmlNewProp (optionNode, BAD_CAST "key", BAD_CAST "xdm broadcast port");
	xmlNewProp (optionNode, BAD_CAST "value", BAD_CAST str);
	optionNode = xmlNewChild (group, NULL, BAD_CAST "option", NULL);
	xmlNewProp (optionNode, BAD_CAST "key", BAD_CAST "xdm query port");
	xmlNewProp (optionNode, BAD_CAST "value", BAD_CAST str);

	optionNode = xmlNewChild (group, NULL, BAD_CAST "option", NULL);
	xmlNewProp (optionNode, BAD_CAST "key", BAD_CAST "xdm list host");
	xmlNewProp (optionNode, BAD_CAST "value", BAD_CAST nx_conn->XdmHost);
	optionNode = xmlNewChild (group, NULL, BAD_CAST "option", NULL);
	xmlNewProp (optionNode, BAD_CAST "key", BAD_CAST "xdm query host");
	xmlNewProp (optionNode, BAD_CAST "value", BAD_CAST nx_conn->XdmHost);
	
	/* 
	 * create the fourth group, "Images" 
	 */
	group = xmlNewChild (docRoot, NULL, BAD_CAST "group", NULL);
	xmlNewProp (group, BAD_CAST "name", BAD_CAST "Images");

	/* hardcoded options, most of these probably to do with VNC/RDP connections */
	if (new_config_file == TRUE) {
		optionNode = xmlNewChild (group, NULL, BAD_CAST "option", NULL);
		xmlNewProp (optionNode, BAD_CAST "key", BAD_CAST "Disable JPEG Compression");
		xmlNewProp (optionNode, BAD_CAST "value", BAD_CAST "0");
	
		optionNode = xmlNewChild (group, NULL, BAD_CAST "option", NULL);
		xmlNewProp (optionNode, BAD_CAST "key", BAD_CAST "Disable all image optimisations");
		xmlNewProp (optionNode, BAD_CAST "value", BAD_CAST "false");
	
		optionNode = xmlNewChild (group, NULL, BAD_CAST "option", NULL);
		xmlNewProp (optionNode, BAD_CAST "key", BAD_CAST "Image Encoding Type");
		xmlNewProp (optionNode, BAD_CAST "value", BAD_CAST "0");
	
		optionNode = xmlNewChild (group, NULL, BAD_CAST "option", NULL);
		xmlNewProp (optionNode, BAD_CAST "key", BAD_CAST "Image JPEG Encoding");
		xmlNewProp (optionNode, BAD_CAST "value", BAD_CAST "false");
	
		optionNode = xmlNewChild (group, NULL, BAD_CAST "option", NULL);
		xmlNewProp (optionNode, BAD_CAST "key", BAD_CAST "RDP optimization for low-bandwidth link");
		xmlNewProp (optionNode, BAD_CAST "value", BAD_CAST "false");
	
		optionNode = xmlNewChild (group, NULL, BAD_CAST "option", NULL);
		xmlNewProp (optionNode, BAD_CAST "key", BAD_CAST "Reduce colors to");
		xmlNewProp (optionNode, BAD_CAST "value", BAD_CAST "");
	
		optionNode = xmlNewChild (group, NULL, BAD_CAST "option", NULL);
		xmlNewProp (optionNode, BAD_CAST "key", BAD_CAST "VNC images compression");
		xmlNewProp (optionNode, BAD_CAST "value", BAD_CAST "0");
	
		optionNode = xmlNewChild (group, NULL, BAD_CAST "option", NULL);
		xmlNewProp (optionNode, BAD_CAST "key", BAD_CAST "Windows Image Compression");
		xmlNewProp (optionNode, BAD_CAST "value", BAD_CAST "1");
	}

	/* configurable options */

	/* nx_conn->ImageEncoding: 0:Default
	                           1:PNG
	                           2:JPEG
                                   3:XBM   */

	/* 0:JPEG no custom 
	   1:JPEG with custom
	   2:PNG/XBM */
	optionNode = xmlNewChild (group, NULL, BAD_CAST "option", NULL);
	xmlNewProp (optionNode, BAD_CAST "key", BAD_CAST "Image Compression Type");
	if (nx_conn->ImageEncoding == 1 || nx_conn->ImageEncoding == 3) {
		xmlNewProp (optionNode, BAD_CAST "value", BAD_CAST "2");
	} else {
		xmlNewProp (optionNode, BAD_CAST "value", BAD_CAST "1");
	}

	/* The value of the image quality spinbutton */
	sprintf (str, "%d", nx_conn->JPEGQuality);
	optionNode = xmlNewChild (group, NULL, BAD_CAST "option", NULL);
	xmlNewProp (optionNode, BAD_CAST "key", BAD_CAST "JPEG Quality");
	xmlNewProp (optionNode, BAD_CAST "value", BAD_CAST str);

	/* PNG: true, otherwise false */
	optionNode = xmlNewChild (group, NULL, BAD_CAST "option", NULL);
	xmlNewProp (optionNode, BAD_CAST "key", BAD_CAST "Use PNG Compression");
	xmlNewProp (optionNode, BAD_CAST "value", 
		    nx_conn->ImageEncoding == 1 ? BAD_CAST "true" : BAD_CAST "false");

	/* 
	 * create the 5th group, "Login" 
	 */
	group = xmlNewChild (docRoot, NULL, BAD_CAST "group", NULL);
	xmlNewProp (group, BAD_CAST "name", BAD_CAST "Login");

	optionNode = xmlNewChild (group, NULL, BAD_CAST "option", NULL);
	xmlNewProp (optionNode, BAD_CAST "key", BAD_CAST "User");
	xmlNewProp (optionNode, BAD_CAST "value", BAD_CAST nx_conn->User);

	/* nx_conn->PublicKey should have been set to the default NX one to begin with */
	optionNode = xmlNewChild (group, NULL, BAD_CAST "option", NULL);
	xmlNewProp (optionNode, BAD_CAST "key", BAD_CAST "Public Key");
	xmlNewProp (optionNode, BAD_CAST "value", BAD_CAST nx_conn->PublicKey);

	/* Don't do anything right now about key="Auth", the saved password */
	optionNode = xmlNewChild (group, NULL, BAD_CAST "option", NULL);
	xmlNewProp (optionNode, BAD_CAST "key", BAD_CAST "Auth");
	xmlNewProp (optionNode, BAD_CAST "value", BAD_CAST password);

	/* Now we can free the password memory */
       g_free (password);

	/* 
	 * create the 6th group, "Services", all currently hardcoded except for sound
	 */
	if (new_config_file == TRUE) {
		group = xmlNewChild (docRoot, NULL, BAD_CAST "group", NULL);
		xmlNewProp (group, BAD_CAST "name", BAD_CAST "Services");

		optionNode = xmlNewChild (group, NULL, BAD_CAST "option", NULL);
		xmlNewProp (optionNode, BAD_CAST "key", BAD_CAST "Audio");
		xmlNewProp (optionNode, BAD_CAST "value", 
			    nx_conn->enableSound ? BAD_CAST "true" : BAD_CAST "false" );
		optionNode = xmlNewChild (group, NULL, BAD_CAST "option", NULL);
		xmlNewProp (optionNode, BAD_CAST "key", BAD_CAST "IPPPort");
		xmlNewProp (optionNode, BAD_CAST "value", BAD_CAST "631");
		optionNode = xmlNewChild (group, NULL, BAD_CAST "option", NULL);
		xmlNewProp (optionNode, BAD_CAST "key", BAD_CAST "IPPPrinting");
		xmlNewProp (optionNode, BAD_CAST "value", BAD_CAST "false");
		optionNode = xmlNewChild (group, NULL, BAD_CAST "option", NULL);
		xmlNewProp (optionNode, BAD_CAST "key", BAD_CAST "Shares");
		xmlNewProp (optionNode, BAD_CAST "value", BAD_CAST "false");
	}

	/* 
	 * create the 7th group, "Share0" - the on client print queue, which we hardcode
	 */
	if (new_config_file == TRUE) {
		group = xmlNewChild (docRoot, NULL, BAD_CAST "group", NULL);
		xmlNewProp (group, BAD_CAST "name", BAD_CAST "Share0");

		optionNode = xmlNewChild (group, NULL, BAD_CAST "option", NULL);
		xmlNewProp (optionNode, BAD_CAST "key", BAD_CAST "Default");
		xmlNewProp (optionNode, BAD_CAST "value", BAD_CAST "true");
		optionNode = xmlNewChild (group, NULL, BAD_CAST "option", NULL);
		xmlNewProp (optionNode, BAD_CAST "key", BAD_CAST "Driver");
		xmlNewProp (optionNode, BAD_CAST "value", BAD_CAST "cups printer");
		optionNode = xmlNewChild (group, NULL, BAD_CAST "option", NULL);
		xmlNewProp (optionNode, BAD_CAST "key", BAD_CAST "Password");
		xmlNewProp (optionNode, BAD_CAST "value", BAD_CAST "");
		optionNode = xmlNewChild (group, NULL, BAD_CAST "option", NULL);
		xmlNewProp (optionNode, BAD_CAST "key", BAD_CAST "Public");
		xmlNewProp (optionNode, BAD_CAST "value", BAD_CAST "false");
		optionNode = xmlNewChild (group, NULL, BAD_CAST "option", NULL);
		xmlNewProp (optionNode, BAD_CAST "key", BAD_CAST "Sharename");
		xmlNewProp (optionNode, BAD_CAST "value", BAD_CAST "wm");
		optionNode = xmlNewChild (group, NULL, BAD_CAST "option", NULL);
		xmlNewProp (optionNode, BAD_CAST "key", BAD_CAST "Type");
		xmlNewProp (optionNode, BAD_CAST "value", BAD_CAST "cupsprinter");
		optionNode = xmlNewChild (group, NULL, BAD_CAST "option", NULL);
		xmlNewProp (optionNode, BAD_CAST "key", BAD_CAST "Username");
		xmlNewProp (optionNode, BAD_CAST "value", BAD_CAST "hc");
	}

	/* 
	 * create the 8th group, "share chosen" again hardcoded
	 */
	if (new_config_file == TRUE) {
		group = xmlNewChild (docRoot, NULL, BAD_CAST "group", NULL);
		xmlNewProp (group, BAD_CAST "name", BAD_CAST "share chosen");

		optionNode = xmlNewChild (group, NULL, BAD_CAST "option", NULL);
		xmlNewProp (optionNode, BAD_CAST "key", BAD_CAST "Share number");
		xmlNewProp (optionNode, BAD_CAST "value", BAD_CAST "1");
		optionNode = xmlNewChild (group, NULL, BAD_CAST "option", NULL);
		xmlNewProp (optionNode, BAD_CAST "key", BAD_CAST "Share0");
		xmlNewProp (optionNode, BAD_CAST "value", BAD_CAST "Share0");
		optionNode = xmlNewChild (group, NULL, BAD_CAST "option", NULL);
		xmlNewProp (optionNode, BAD_CAST "key", BAD_CAST "default printer");
		xmlNewProp (optionNode, BAD_CAST "value", BAD_CAST "Share0");
	}

	/* Save with Indentation */
	xmlIndentTreeOutput = 1;

	rtn = xmlSaveFormatFile (connection_file, doc, 1);
	/* specifying encoding: rtn = xmlSaveFormatFileEnc (connection_file, doc, "iso8859-1", 1);*/

	xmlFreeDoc (doc);

	if (rtn == -1) {
		/* Calling function generates popup */
		/*nx_general_error (_("Error saving NX configuration. Please check disk space"), 
		  _("Error saving configuration"));*/
		return -1;
	}

	printerr ("%s returning\n", __FUNCTION__);
	/* Clean up and return */
	return rtn;
}

/* Delete a connection file */
gint delete_nx_connection (gchar * name)
{
	gchar connection_file[1024];
	snprintf (connection_file, 1023, "%s/.nx/config/%s.nxs", getenv("HOME"), name);
	unlink (connection_file);
	return 0;
}

/**
 * Replace a connection config file. You do: 
 * delete_nx_connection (); 
 * write_nx_connection ();
 *
 */
gint replace_nx_connection (struct nx_connection * nx_conn, gchar * name)
{
	delete_nx_connection (name);	
	return (write_nx_connection (nx_conn));
}

/* places contents of the general group in nx_conn */
void parse_general (xmlDocPtr doc, xmlNodePtr cur, 
		    struct nx_connection * nx_conn)
{
	xmlChar * option_key;

	xmlChar * xdm_mode = NULL;
	guint xdm_li_port = 177;
	xmlChar * xdm_li_host = NULL;
	guint xdm_bc_port = 177;
	guint xdm_qu_port = 177;
	xmlChar * xdm_qu_host = NULL;

	cur = cur->xmlChildrenNode;

	while (cur != NULL) {
		if ((!xmlStrcmp (cur->name, BAD_CAST "option"))) {

			option_key = xmlGetProp (cur, BAD_CAST "key");

			if ((!xmlStrcmp (option_key, BAD_CAST "Server host"))) {
				strncpy (nx_conn->ServerHost, (gchar *) xmlGetProp (cur, BAD_CAST "value"), NX_FIELDLEN-1);
			}
			else if ((!xmlStrcmp (option_key, BAD_CAST "Server port"))) {
				nx_conn->ServerPort = atoi ((gchar *) xmlGetProp (cur, BAD_CAST "value"));
			}
			else if ((!xmlStrcmp (option_key, BAD_CAST "Desktop"))) {
				strncpy (nx_conn->Desktop, (gchar *) xmlGetProp (cur, BAD_CAST "value"), NX_FIELDLEN-1);
			}
			else if ((!xmlStrcmp (option_key, BAD_CAST "Session"))) {
				strncpy (nx_conn->Session, (gchar *) xmlGetProp (cur, BAD_CAST "value"), NX_FIELDLEN-1);
			}
			else if ((!xmlStrcmp (option_key, BAD_CAST "Custom Unix Desktop"))) {
				strncpy (nx_conn->CustomUnixDesktop, (gchar *) xmlGetProp (cur, BAD_CAST "value"), NX_FIELDLEN-1);
			}
			else if ((!xmlStrcmp (option_key, BAD_CAST "Command line"))) {
				strncpy (nx_conn->CommandLine, (gchar *) xmlGetProp (cur, BAD_CAST "value"), NX_FIELDLEN-1);
			}
			else if ((!xmlStrcmp (option_key, BAD_CAST "Link speed"))) {
				strncpy (nx_conn->LinkSpeed, (gchar *) xmlGetProp (cur, BAD_CAST "value"), NX_FIELDLEN-1);
			}
			else if ((!xmlStrcmp (option_key, BAD_CAST "Remember password"))) {
				nx_conn->RememberPassword = 
					(!xmlStrcmp (xmlGetProp (cur, BAD_CAST "value"), BAD_CAST "true"))
					? TRUE : FALSE;
			}
			/* bit of a pain */
			else if ((!xmlStrcmp (option_key, BAD_CAST "Resolution"))) {
				nx_conn->FullScreen = 
					(!xmlStrcmp (xmlGetProp (cur, BAD_CAST "value"), BAD_CAST "fullscreen"))
					? TRUE : FALSE;
			}
			else if ((!xmlStrcmp (option_key, BAD_CAST "Resolution width"))) {
				nx_conn->ResolutionWidth = atoi ((gchar *)xmlGetProp (cur, BAD_CAST "value"));
			}
			else if ((!xmlStrcmp (option_key, BAD_CAST "Resolution height"))) {
				nx_conn->ResolutionHeight = atoi ((gchar *)xmlGetProp (cur, BAD_CAST "value"));
			}
			else if ((!xmlStrcmp (option_key, BAD_CAST "Use default image encoding"))) {
				if (!xmlStrcmp (xmlGetProp (cur, BAD_CAST "value"), BAD_CAST "0")) {
					nx_conn->ImageEncoding = 0; /* default */
				}
			}
			else if ((!xmlStrcmp (option_key, BAD_CAST "Use taint"))) {
				nx_conn->UseTaint = 
					(!xmlStrcmp (xmlGetProp (cur, BAD_CAST "value"), BAD_CAST "true"))
					? TRUE : FALSE;
			}
			else if ((!xmlStrcmp (option_key, BAD_CAST "Virtual desktop"))) {
				nx_conn->VirtualDesktop = 
					(!xmlStrcmp (xmlGetProp (cur, BAD_CAST "value"), BAD_CAST "true"))
					? TRUE : FALSE;
				printerr ("VirtualDesktop is %d\n", nx_conn->VirtualDesktop);
			}
			else if ((!xmlStrcmp (option_key, BAD_CAST "XAgent encoding"))) {
				nx_conn->XAgentEncoding = 
					(!xmlStrcmp (xmlGetProp (cur, BAD_CAST "value"), BAD_CAST "true"))
					? TRUE : FALSE;
			}
			else if ((!xmlStrcmp (option_key, BAD_CAST "xdm mode"))) {
				xdm_mode = xmlGetProp (cur, BAD_CAST "value");
			}
			else if ((!xmlStrcmp (option_key, BAD_CAST "xdm list port"))) {
				xdm_li_port = atoi ((gchar *) xmlGetProp (cur, BAD_CAST "value"));
			}
			else if ((!xmlStrcmp (option_key, BAD_CAST "xdm broadcast port"))) {
				xdm_bc_port = atoi ((gchar *) xmlGetProp (cur, BAD_CAST "value"));
			}
			else if ((!xmlStrcmp (option_key, BAD_CAST "xdm query port"))) {
				xdm_qu_port = atoi ((gchar *) xmlGetProp (cur, BAD_CAST "value"));
			}
			else if ((!xmlStrcmp (option_key, BAD_CAST "xdm list host"))) {
				xdm_li_host = xmlGetProp (cur, BAD_CAST "value");
			}
			else if ((!xmlStrcmp (option_key, BAD_CAST "xdm query host"))) {
				xdm_qu_host = xmlGetProp (cur, BAD_CAST "value");
			}

			xmlFree (option_key);
		}
		cur = cur->next;
	}

	/* Now we need to do a little extra work to set nx_conn->XdmHost and XdmPort */
	if (xdm_mode != NULL) {
		
		strncpy (nx_conn->XdmMode, (gchar *)xdm_mode, NX_FIELDLEN-1);

		if ((!xmlStrcmp (xdm_mode, BAD_CAST "list"))) {
			if (xdm_li_host != NULL) {
				strncpy (nx_conn->XdmHost, (gchar *)xdm_li_host, NX_FIELDLEN-1);
			} else {
				strncpy (nx_conn->XdmHost, "your.X.host", NX_FIELDLEN-1);
			}
			nx_conn->XdmPort = xdm_li_port;
			
		} else if ((!xmlStrcmp (xdm_mode, BAD_CAST "query"))) {
			if (xdm_qu_host != NULL) {
				strncpy (nx_conn->XdmHost, (gchar *)xdm_qu_host, NX_FIELDLEN-1);
			} else {
				strncpy (nx_conn->XdmHost, "your.X.host", NX_FIELDLEN-1);
			}
			nx_conn->XdmPort = xdm_qu_port;
			
		} else if ((!xmlStrcmp (xdm_mode, BAD_CAST "broadcast"))) {
			if (xdm_qu_host != NULL) {
				strncpy (nx_conn->XdmHost, (gchar *)xdm_qu_host, NX_FIELDLEN-1);
			} else {
				strncpy (nx_conn->XdmHost, "your.X.host", NX_FIELDLEN-1);
			}
			nx_conn->XdmPort = xdm_bc_port;

		} else {
			/* xdm_mode is "server decide", null or something random  */
			strncpy (nx_conn->XdmHost, (gchar *)xdm_qu_host, NX_FIELDLEN-1);
			nx_conn->XdmPort = 177;	
		}

		if (xdm_qu_host != NULL) {
			xmlFree (xdm_qu_host);
		}
		if (xdm_li_host != NULL) {
			xmlFree (xdm_li_host);
		}

		xmlFree (xdm_mode);

	} else {
		strncpy (nx_conn->XdmHost, "your.X.host", NX_FIELDLEN-1);
		nx_conn->XdmPort = 177;		
	}

	return;
}

/* places contents of the advanced group in nx_conn */
void parse_advanced (xmlDocPtr doc, xmlNodePtr cur, 
		     struct nx_connection * nx_conn)
{
	xmlChar * option_key;

	cur = cur->xmlChildrenNode;

	while (cur != NULL) {
		if ((!xmlStrcmp (cur->name, BAD_CAST "option"))) {

			option_key = xmlGetProp (cur, BAD_CAST "key");

			if ((!xmlStrcmp (option_key, BAD_CAST "Disable ZLIB stream compression"))) {
				nx_conn->DisableZLIB = 
					(!xmlStrcmp (xmlGetProp (cur, BAD_CAST "value"), BAD_CAST "true"))
					? TRUE : FALSE;
			}
			else if ((!xmlStrcmp (option_key, BAD_CAST "Disable TCP no-delay"))) {
				nx_conn->DisableNoDelay = 
					(!xmlStrcmp (xmlGetProp (cur, BAD_CAST "value"), BAD_CAST "true"))
					? TRUE : FALSE;
			}
			else if ((!xmlStrcmp (option_key, BAD_CAST "Enable SSL encryption"))) {
				nx_conn->EnableSSLOnly = 
					(!xmlStrcmp (xmlGetProp (cur, BAD_CAST "value"), BAD_CAST "true"))
					? TRUE : FALSE;
			}

			xmlFree (option_key);
		}
		cur = cur->next;
	}
	return;
}

/* places contents of the general group def im enc in data */
gint read_default_image_encoding (xmlDocPtr doc, xmlNodePtr cur)
{
	xmlChar * option_key;
	gint rtn = -1;
	cur = cur->xmlChildrenNode;

	while (cur != NULL) {
		if ((!xmlStrcmp (cur->name, BAD_CAST "option"))) {
			
			option_key = xmlGetProp (cur, BAD_CAST "key");
							
			if ((!xmlStrcmp (option_key, BAD_CAST "Use default image encoding"))) {
				if (!xmlStrcmp (xmlGetProp (cur, BAD_CAST "value"), BAD_CAST "0")) {
					rtn = 0; /* default */
				} else if (!xmlStrcmp (xmlGetProp (cur, BAD_CAST "value"), BAD_CAST "1")) { 
					rtn = 1;
				} else {
					rtn = -1;
				}
			}
			xmlFree (option_key);
			if (rtn != -1) {
				break;
			}
		}
		cur = cur->next;
	}
	return rtn;
}

/*!
 * Return value of "Use default image encoding" node 
 * which is 0 or 1. -1 on failure.
 */
gint find_def_im_enc_in_general (xmlDocPtr doc, xmlNodePtr cur)
{
	xmlChar * group_name;
	gint rtn = -1;
	gboolean rtn_ready = FALSE;

	if (!xmlStrcmp (cur->name, BAD_CAST "NXClientSettings")) {

		cur = cur->xmlChildrenNode;
		while (cur != NULL) {
			if ((!xmlStrcmp (cur->name, BAD_CAST "group"))){

				group_name = xmlGetProp (cur, BAD_CAST "name");

				if ((!xmlStrcmp (group_name, BAD_CAST "General"))) {
					printerr ("\"General\" group found (lookin for def. im. enc.)\n");

					rtn = read_default_image_encoding (doc, cur);
					rtn_ready = TRUE;
				}
				xmlFree (group_name);
				if (rtn_ready == TRUE) { break; }
			}
			cur = cur->next;
		}
	} else {
		printerr ("Got passed the wrong parent node\n");
	}

	return rtn;
}


#define NOT_YET_SET 99
/* places contents of the images group in nx_conn */
void parse_images (xmlDocPtr doc, xmlNodePtr cur, 
		   struct nx_connection * nx_conn)
{
	xmlChar * option_key;
	guint comp = 5;
	gboolean png = FALSE;
	xmlNodePtr parent; /* used to get the "Use default image encoding" setting. */
	gint default_image_encoding = -1;

	/* At this point, cur is <group name="Images"> */
	parent = cur->parent;

	default_image_encoding = find_def_im_enc_in_general (doc, parent);
	printerr ("default_image_encoding = %d\n", default_image_encoding);

	if (default_image_encoding == 0) {
		/* This means "USE DEFAULT" */
		nx_conn->ImageEncoding = 0;
	} else if (default_image_encoding == 1) {
		/* This means "DON'T USE DEFAULT" */
		nx_conn->ImageEncoding = NOT_YET_SET; /* used a few lines down */
	} else {
		nx_conn->ImageEncoding = NOT_YET_SET;
	}

	cur = cur->children;

	while (cur != NULL) {
		if ((!xmlStrcmp (cur->name, BAD_CAST "option"))) {

			option_key = xmlGetProp (cur, BAD_CAST "key");

			if ((!xmlStrcmp (option_key, BAD_CAST "Image Compression Type"))) {				
				comp = atoi ((gchar *) xmlGetProp (cur, BAD_CAST "value"));
			}
			else if ((!xmlStrcmp (option_key, BAD_CAST "JPEG Quality"))) {
				nx_conn->JPEGQuality = atoi ((gchar *) xmlGetProp (cur, BAD_CAST "value"));
			}
			else if ((!xmlStrcmp (option_key, BAD_CAST "Use PNG Compression"))) {
				png = (!xmlStrcmp (xmlGetProp (cur, BAD_CAST "value"), BAD_CAST "true"))
					? TRUE : FALSE;	
			}

			xmlFree (option_key);
		}
		cur = cur->next;
	}

	if (nx_conn->ImageEncoding == NOT_YET_SET) {
		if (png) {
			nx_conn->ImageEncoding = 1; /* PNG */
		} else {
			if (comp == 2) {
				nx_conn->ImageEncoding = 3; /* XBM */
			} else if (comp == 0 || comp == 1) {
				nx_conn->ImageEncoding = 2; /* JPEG */
			}
		}
	}

	return;
}

/* places contents of the login group in nx_conn */
void parse_login (xmlDocPtr doc, xmlNodePtr cur, 
		  struct nx_connection * nx_conn)
{
	xmlChar * option_key;
	gint i, end;

	cur = cur->xmlChildrenNode;

	while (cur != NULL) {
		if ((!xmlStrcmp (cur->name, BAD_CAST "option"))) {

			option_key = xmlGetProp (cur, BAD_CAST "key");

			if ((!xmlStrcmp (option_key, BAD_CAST "User"))) {
				strncpy (nx_conn->User, (gchar *) xmlGetProp (cur, BAD_CAST "value"), NX_FIELDLEN-1);
			}
			else if ((!xmlStrcmp (option_key, BAD_CAST "Public Key"))) {
				strncpy (nx_conn->PublicKey, (gchar *) xmlGetProp (cur, BAD_CAST "value"), SSLKEYLEN-1);
				/* We need to parse the PublicKey now and replace and spaces with new lines, because
				   the nxclient program seems to incorrectly save \n characters in the .nxs file which
				   are read in and converted to whitespace (the space char 0x20) by libxml2. */
				
				/* Need to avoid the -----BEGIN DSA PRIVATE KEY----- string and the
				   -----END DSA PRIVATE KEY----- string, but escape all other characters.
				*/

				/* Find the end of the key */
				end = 0;
				while (nx_conn->PublicKey[end++]) {}

				i = 30; /* 30 means this should start on a '-' character */
				while (nx_conn->PublicKey[i] && i<(end-29)) {

					/*printerr ("PublicKey[%d] = '%c' or 0x%x\n", 
					  i, nx_conn->PublicKey[i], nx_conn->PublicKey[i]);*/
					
					/* Replace spaces in the actual key with newline characters */
					if (nx_conn->PublicKey[i] == 0x20) {
						nx_conn->PublicKey[i] = 0xa;
					}
					i++;
				}
			}

			xmlFree (option_key);
		}
		cur = cur->next;
	}
	return;
}

void parse_services (xmlDocPtr doc, xmlNodePtr cur, 
		     struct nx_connection * nx_conn)
{
	xmlChar * option_key;

	cur = cur->children;

	while (cur != NULL) {
		if ((!xmlStrcmp (cur->name, BAD_CAST "option"))) {

			option_key = xmlGetProp (cur, BAD_CAST "key");

			if ((!xmlStrcmp (option_key, BAD_CAST "Audio"))) {
				nx_conn->enableSound =
					(!xmlStrcmp (xmlGetProp (cur, BAD_CAST "value"), BAD_CAST "true")) ? TRUE : FALSE;

			} else if ((!xmlStrcmp (option_key, BAD_CAST "IPPPort"))) {
				nx_conn->IPPPort = atoi ((gchar *) xmlGetProp (cur, BAD_CAST "value"));

			} else if ((!xmlStrcmp (option_key, BAD_CAST "IPPPrinting"))) {
				nx_conn->IPPPrinting =
					(!xmlStrcmp (xmlGetProp (cur, BAD_CAST "value"), BAD_CAST "true")) ? TRUE : FALSE;

			} else if ((!xmlStrcmp (option_key, BAD_CAST "Shares"))) {
				nx_conn->Shares =
					(!xmlStrcmp (xmlGetProp (cur, BAD_CAST "value"), BAD_CAST "true")) ? TRUE : FALSE;
			}

			xmlFree (option_key);
		}
		cur = cur->next;
	}

	return;
}


/**
 * Read an entry in theConn.nxs to display in the nx pop up.
 *
 * Returns 0 if all ok. Otherwise, an error code as found in callbacks_nx.h
 */
gint read_nx_connection (struct nx_connection * nx_conn, const gchar * name)
{
	xmlDocPtr doc;
	xmlNodePtr cur;
	xmlChar * group_name;

	gchar * doc_name;

	strncpy (nx_conn->ConnectionName, name, NX_FIELDLEN-1);

	doc_name = g_malloc (1024 * sizeof (gchar));
	snprintf (doc_name, 1023, "%s/.nx/config/%s.nxs", getenv("HOME"), name);
	doc = xmlParseFile (doc_name);
	/* Assume it's ok to free up docname here */
	g_free (doc_name);

	if (doc == NULL ) {
		/* document not parsed successfully */
		return NX_XML_NOT_PARSED;
	}

	cur = xmlDocGetRootElement (doc);

	if (cur == NULL) {
		/* empty document */
		xmlFreeDoc (doc);
		return NX_XML_EMPTY;
	}

	if (xmlStrcmp (cur->name, BAD_CAST "NXClientSettings")) {
		/* document of the wrong type, root node != NXClientSettings */
		xmlFreeDoc (doc);
		return NX_XML_WRONG_ROOT_NODE;
	}

	cur = cur->xmlChildrenNode;
	while (cur != NULL) {
		if ((!xmlStrcmp (cur->name, BAD_CAST "group"))){

			group_name = xmlGetProp (cur, BAD_CAST "name");

			if ((!xmlStrcmp (group_name, BAD_CAST "General"))) {
				printerr ("\"General\" group found\n");
				parse_general (doc, cur, nx_conn);

			} else if ((!xmlStrcmp (group_name, BAD_CAST "Advanced"))) {
				printerr ("\"Advanced\" group found\n");
				parse_advanced (doc, cur, nx_conn);

			} else if ((!xmlStrcmp (group_name, BAD_CAST "Images"))) {
				printerr ("\"Images\" group found\n");
				parse_images (doc, cur, nx_conn);

			} else if ((!xmlStrcmp (group_name, BAD_CAST "Login"))) {
				printerr ("\"Login\" group found\n");
				parse_login (doc, cur, nx_conn);

			} else if ((!xmlStrcmp (group_name, BAD_CAST "Services"))) {
				printerr ("\"Services\" group found\n");
				parse_services (doc, cur, nx_conn);

#ifdef EXTRAXMLNEEDED
			} else if ((!xmlStrcmp (group_name, BAD_CAST "Environment"))) {
				printerr ("\"Environment\" group found\n");
				/* don't need (hardcoded): 
				   parse_environment (doc, cur, nx_conn);*/

			} else if ((!xmlStrcmp (group_name, BAD_CAST "Share0"))) {
				printerr ("\"Share0\" group found\n");
				/* don't need:
				   parse_share0 (doc, cur, nx_conn);*/

			} else if ((!xmlStrcmp (group_name, BAD_CAST "share chosen"))) {
				printerr ("\"share chosen\" group found\n");
				/* don't need:
				   parse_share_chosen (doc, cur, nx_conn);*/
#endif
			} else {
				printerr ("hardcoded/unknown group found\n");
			}

			xmlFree (group_name);

		}
		cur = cur->next;
	}

	/* the Geometry field is not stored in the XML, but we create it, for convenience of other apps */
	snprintf (nx_conn->Geometry, 32, "%dx%d", nx_conn->ResolutionWidth, nx_conn->ResolutionHeight);

	xmlFreeDoc (doc);
	printerr ("returning from %s\n", __FUNCTION__);
	return 0;
}



/*
 * Allocate memory for an nx connection data structure.
 * Note that if you use gtk tree models, you may prefer to only malloc
 * the struct nx_connection, as some of the gtk functions will malloc
 * before they copy a string into you nx_connection structure.
 */
struct nx_connection * nx_connection_malloc (void)
{
	struct nx_connection * nx_conn;

	/* malloc the structure itself */
	nx_conn = g_malloc0 (sizeof (struct nx_connection));
	
	/* and all the gchars that it has to point to */
	nx_conn->ConnectionName = g_malloc0 (NX_FIELDLEN * sizeof(gchar));
	nx_conn->ServerHost = g_malloc0 (NX_FIELDLEN * sizeof(gchar));
	nx_conn->User = g_malloc0 (NX_FIELDLEN * sizeof(gchar));
	nx_conn->Pass = g_malloc0 (NX_FIELDLEN * sizeof(gchar));
	nx_conn->Geometry = g_malloc0 (NX_FIELDLEN * sizeof(gchar));
	nx_conn->LinkSpeed = g_malloc0 (NX_FIELDLEN * sizeof(gchar));
	nx_conn->Desktop = g_malloc0 (NX_FIELDLEN * sizeof(gchar));
	nx_conn->Session = g_malloc0 (NX_FIELDLEN * sizeof(gchar));
	nx_conn->CustomUnixDesktop = g_malloc0 (NX_FIELDLEN * sizeof(gchar));
	nx_conn->XdmMode = g_malloc0 (NX_FIELDLEN * sizeof(gchar));
	nx_conn->XdmHost = g_malloc0 (NX_FIELDLEN * sizeof(gchar));

	/* More memory required for SSLKey and the command line */
	nx_conn->PublicKey = g_malloc0 (SSLKEYLEN * sizeof(gchar));
	nx_conn->CommandLine = g_malloc0 (CMDLINELEN * sizeof(gchar));

	nx_conn->agentServer = g_malloc0 (NX_FIELDLEN * sizeof(gchar));
	nx_conn->agentUser = g_malloc0 (NX_FIELDLEN * sizeof(gchar));
	nx_conn->agentPass = g_malloc0 (NX_FIELDLEN * sizeof(gchar));

	return nx_conn;
}

/* reset an nx connection data structure to default values */
void nx_connection_zero (struct nx_connection * nx_conn)
{
	strcpy (nx_conn->ConnectionName, "");
	strcpy (nx_conn->ServerHost, "");
	strcpy (nx_conn->User, "");
	strcpy (nx_conn->Pass, "");
	strcpy (nx_conn->LinkSpeed, "wan");
	strcpy (nx_conn->Desktop, "kde");
	strcpy (nx_conn->Session, "unix");
	strcpy (nx_conn->CustomUnixDesktop, "console");
	strncpy (nx_conn->PublicKey, NXDefaultKey, SSLKEYLEN-1); /* Set to default */
	strcpy (nx_conn->CommandLine, "");
	strcpy (nx_conn->XdmMode, "server decide");
	strcpy (nx_conn->XdmHost, "");
	strcpy (nx_conn->agentServer, "");
	strcpy (nx_conn->agentUser, "");
	strcpy (nx_conn->agentPass, "");


	nx_conn->ServerPort = 22;
	nx_conn->ResolutionWidth = 800;
	nx_conn->ResolutionHeight = 600;
	strcpy (nx_conn->Geometry, "800x600");
	nx_conn->ImageEncoding = 0;
	nx_conn->JPEGQuality = 5;
	nx_conn->XdmPort = 177;

	nx_conn->RememberPassword = FALSE;
	nx_conn->DisableNoDelay = FALSE;
	nx_conn->DisableZLIB = FALSE;
	nx_conn->EnableSSLOnly = TRUE;
	nx_conn->VirtualDesktop = FALSE;
	nx_conn->XAgentEncoding = FALSE;
	nx_conn->UseTaint = FALSE;
	nx_conn->FullScreen = TRUE;
	nx_conn->enableSound = FALSE;
	return;
}

/* free up memory associated with an nx connection data structure */
gint nx_connection_free (struct nx_connection * nx_conn)
{
	if (nx_conn->ConnectionName)
		g_free (nx_conn->ConnectionName);
	if (nx_conn->ServerHost)
		g_free (nx_conn->ServerHost);
	if (nx_conn->User)
		g_free (nx_conn->User);
	if (nx_conn->Pass)
		g_free (nx_conn->Pass);
	if (nx_conn->Geometry)
		g_free (nx_conn->Geometry);
	if (nx_conn->LinkSpeed)
		g_free (nx_conn->LinkSpeed);
	if (nx_conn->Desktop)
		g_free (nx_conn->Desktop);
	if (nx_conn->Session)
		g_free (nx_conn->Session);
	if (nx_conn->CustomUnixDesktop)
		g_free (nx_conn->CustomUnixDesktop);
	if (nx_conn->PublicKey)
		g_free (nx_conn->PublicKey);
	if (nx_conn->CommandLine)
		g_free (nx_conn->CommandLine);
	if (nx_conn->XdmMode)
		g_free (nx_conn->XdmMode);
	if (nx_conn->XdmHost)
		g_free (nx_conn->XdmHost);
	if (nx_conn->agentServer)
		g_free (nx_conn->agentServer);
	if (nx_conn->agentUser)
		g_free (nx_conn->agentUser);
	if (nx_conn->agentPass)
		g_free (nx_conn->agentPass);

	if (nx_conn != NULL)
		g_free (nx_conn);
	
	return 0;
}




void setup_nx_popup (struct nx_connection * nx_conn)
{
	GtkWidget * widget;
	GtkTextBuffer * buffer;

	/* First set the page of the notebook so it always comes up the same way */
	widget = glade_xml_get_widget (xml_glob, "notebook_nx");
	gtk_notebook_set_current_page (GTK_NOTEBOOK (widget), 0);

	widget = glade_xml_get_widget (xml_glob, "entry_nx_connection_name");
	gtk_entry_set_text (GTK_ENTRY (widget), nx_conn->ConnectionName);

	widget = glade_xml_get_widget (xml_glob, "entry_nx_user");
	gtk_entry_set_text (GTK_ENTRY (widget), nx_conn->User);

	widget = glade_xml_get_widget (xml_glob, "entry_nx_server");
	gtk_entry_set_text (GTK_ENTRY (widget), nx_conn->ServerHost);

	widget = glade_xml_get_widget (xml_glob, "spinbutton_nx_port");
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (widget), nx_conn->ServerPort);

	widget = glade_xml_get_widget (xml_glob, "checkbutton_nx_save_password");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (widget), 
				     nx_conn->RememberPassword ? TRUE : FALSE);

	widget = glade_xml_get_widget (xml_glob, "checkbutton_nx_disable_no_delay");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), 
				      nx_conn->DisableNoDelay ? TRUE : FALSE); 

	widget = glade_xml_get_widget (xml_glob, "checkbutton_nx_disable_zlib");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), 
				      nx_conn->DisableZLIB ? TRUE : FALSE); 

	widget = glade_xml_get_widget (xml_glob, "checkbutton_nx_enable_ssl_for_all");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (widget), 
				     nx_conn->EnableSSLOnly ? TRUE : FALSE); 

	widget = glade_xml_get_widget (xml_glob, "hscale_nx_connection_capacity");
	if (!strncmp (nx_conn->LinkSpeed, "modem", NX_FIELDLEN)) {
		gtk_range_set_value (GTK_RANGE (widget), 1);
	} else if (!strncmp (nx_conn->LinkSpeed, "isdn", NX_FIELDLEN)) {
		gtk_range_set_value (GTK_RANGE (widget), 2);
	} else if (!strncmp (nx_conn->LinkSpeed, "adsl", NX_FIELDLEN)) {
		gtk_range_set_value (GTK_RANGE (widget), 3);
	} else if (!strncmp (nx_conn->LinkSpeed, "wan", NX_FIELDLEN)) {
		gtk_range_set_value (GTK_RANGE (widget), 4);
	} else if (!strncmp (nx_conn->LinkSpeed, "lan", NX_FIELDLEN)) {
		gtk_range_set_value (GTK_RANGE (widget), 5);
	} else {
		gtk_range_set_value (GTK_RANGE (widget), 3);
	}


	widget = glade_xml_get_widget (xml_glob, "combobox_nx_desktop_session");

	if (!strncmp (nx_conn->Session, "unix", NX_FIELDLEN)) {

		// Unix sessions
		if (!strncmp (nx_conn->Desktop, "kde", NX_FIELDLEN)) {
			set_combobox (widget, "KDE");

		} else 	if (!strncmp (nx_conn->Desktop, "gnome", NX_FIELDLEN)) {
			set_combobox (widget, "GNOME");

		} else 	if (!strncmp (nx_conn->Desktop, "cde", NX_FIELDLEN)) {
			set_combobox (widget, "CDE");

		} else 	if (!strncmp (nx_conn->Desktop, "xdm", NX_FIELDLEN)) {
			set_combobox (widget, "XDM");

		} else 	if (!strncmp (nx_conn->Desktop, "console", NX_FIELDLEN)) {

			if (!strncmp (nx_conn->CustomUnixDesktop, "console", NX_FIELDLEN)) {

				set_combobox (widget, _("Console"));

			} else if (!strncmp (nx_conn->CustomUnixDesktop, "default", NX_FIELDLEN)) {

				set_combobox (widget, _("Default X client script on server"));

			} else if (!strncmp (nx_conn->CustomUnixDesktop, "application", NX_FIELDLEN)) {

				set_combobox (widget, _("Custom command"));
			} else {
				set_combobox (widget, _("Console"));
			}

		} else {
			set_combobox (widget, "KDE");
		}

	} else if (!strncmp (nx_conn->Session, "windows", NX_FIELDLEN)) {
		// Windows
		set_combobox (widget, "Windows");
	} else if (!strncmp (nx_conn->Session, "vnc", NX_FIELDLEN)) {
		// VNC
		set_combobox (widget, "VNC");
	} else if (!strncmp (nx_conn->Session, "shadow", NX_FIELDLEN)) {
		// Shadow sessions
		set_combobox (widget, "Shadow");
	}

	widget = glade_xml_get_widget (xml_glob, "entry_nx_custom_command");
	gtk_entry_set_text (GTK_ENTRY (widget), nx_conn->CommandLine);

	widget = glade_xml_get_widget (xml_glob, "radiobutton_nx_new_virtual_desktop");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), nx_conn->VirtualDesktop);

	widget = glade_xml_get_widget (xml_glob, "radiobutton_nx_floating_window");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), !nx_conn->VirtualDesktop);

	widget = glade_xml_get_widget (xml_glob, "checkbutton_nx_disable_x_agent");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), !nx_conn->XAgentEncoding);

	widget = glade_xml_get_widget (xml_glob, "checkbutton_nx_disable_x_replies");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (widget), !nx_conn->UseTaint);

	widget = glade_xml_get_widget (xml_glob, "combobox_nx_xdm_mode");
	if (!strncmp (nx_conn->XdmMode, "server decide", NX_FIELDLEN)) {
		set_combobox (widget, "Let NX server decide");

	} else if (!strncmp (nx_conn->XdmMode, "query", NX_FIELDLEN)) {
		set_combobox (widget, "Send an XDM query to a host");

	} else if (!strncmp (nx_conn->XdmMode, "broadcast", NX_FIELDLEN)) {
		set_combobox (widget, "Broadcast an XDM query");

	} else if (!strncmp (nx_conn->XdmMode, "list", NX_FIELDLEN)) {
		set_combobox (widget, "Get a list of available X display managers");

	} else {
		printerr ("deflt, nx_conn->XdmMode = %s\n", nx_conn->XdmMode);
		set_combobox (widget, "Let NX server decide");
	}

	widget = glade_xml_get_widget (xml_glob, "entry_nx_xdm_host");
	gtk_entry_set_text (GTK_ENTRY (widget), nx_conn->XdmHost);

	widget = glade_xml_get_widget (xml_glob, "spinbutton_nx_xdm_port");
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (widget), nx_conn->XdmPort);

	widget = glade_xml_get_widget (xml_glob, "combobox_nx_window_size");
	if (nx_conn->FullScreen) {
		set_combobox (widget, "Full Screen");	
	} else {

		if (nx_conn->ResolutionWidth == 640 
		    && nx_conn->ResolutionHeight == 480) {
			set_combobox (widget, "640 x 480");	
		} else if (nx_conn->ResolutionWidth == 800
		    && nx_conn->ResolutionHeight == 600) {
			set_combobox (widget, "800 x 600");
		} else if (nx_conn->ResolutionWidth == 1024
		    && nx_conn->ResolutionHeight == 768) {
			set_combobox (widget, "1024 x 768");
		} else if (nx_conn->ResolutionWidth == 1280
		    && nx_conn->ResolutionHeight == 1024) {
			set_combobox (widget, "1280 x 1024");
		} else {
			set_combobox (widget, "Custom");
		}
	}

	widget = glade_xml_get_widget (xml_glob, "spinbutton_nx_width");
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (widget), nx_conn->ResolutionWidth);

	widget = glade_xml_get_widget (xml_glob, "spinbutton_nx_height");
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (widget), nx_conn->ResolutionHeight);

	widget = glade_xml_get_widget (xml_glob, "combobox_nx_image_encoding");
	switch (nx_conn->ImageEncoding) {
	case 1:
		set_combobox (widget, "PNG");
		break;
	case 2:
		set_combobox (widget, "JPEG");
		break;
	case 3:
		set_combobox (widget, "X Bitmap");
		break;
	default:
		set_combobox (widget, "Default");
		break;
	}

	widget = glade_xml_get_widget (xml_glob, "spinbutton_nx_jpeg_quality");
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (widget), nx_conn->JPEGQuality);

	/* SSL key */
	widget = glade_xml_get_widget (xml_glob, "textview_nx_key");
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (widget));
	gtk_text_buffer_set_text (buffer, nx_conn->PublicKey, -1);

	/* Enable sound */
	widget = glade_xml_get_widget (xml_glob, "checkbutton_nx_enable_sound");
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON (widget), 
				     nx_conn->enableSound ? TRUE : FALSE); 
	return;
}


/*!
 * Pass the name of the NX connection then read from suitable config
 * file into an nx_connection data structure and use this to set the
 * fields in the pop up window.
 *
 */
void setup_new_nx_popup (const gchar * name)
{
	struct nx_connection * nx_conn;
	nx_conn = nx_connection_malloc ();
	read_nx_connection (nx_conn, name);
	setup_nx_popup (nx_conn);
	nx_connection_free (nx_conn);
	return;
}

/*!
 * This function re-sets the NX popup to have
 * zero or default values.
 */
void zero_new_nx_popup (void)
{
	struct nx_connection * nx_conn;
	nx_conn = nx_connection_malloc ();
	nx_connection_zero (nx_conn);
	setup_nx_popup (nx_conn);
	nx_connection_free (nx_conn);
	return;
}

/*!
 * This function sets up name, server, user and fullscreen with data
 * taken from the submitted form data in conn_new_nx dialog. This is
 * not required in nxlaunch, but is used by another program which
 * links to callbacks_nx.c
 *
 * It also sets up the data in nx_conn (which will be written to the
 * config files) with values taken from the conn_new_nx dialog. This
 * is the really important action as far as nxlaunch is concerned. You
 * can just malloc some fields NX_FIELDLEN in size and pass the
 * pointers to this function in nxlaunch and similar programs.
 *
 * \param name Filled with the name for the connection.
 *
 * \param server Filled with the server ip or address
 *
 * \param uname Filled with the username for the connection
 *
 * \param fullscreen Set >0 if the connection is fullscreen
 *
 * \param nx_conn Filled with all the details of the connection (name,
 * server, username and fullscreen are all in here)
 */
void read_nx_popup (gchar * name, gchar * server, gchar * uname, int * fullscreen,
		    struct nx_connection * nx_conn)
{
	GtkWidget * widget;
	GtkTextBuffer * buffer;
	GtkTextIter start, end;
	GtkTreeIter iter;
	GtkListStore * nxList = NULL;
	gchar * str;
	gboolean strAllocatedMan = FALSE; // True if str has been allocated manually
	gboolean strAllocatedGtk = FALSE; // True if str has been allocated within gtk fn

	/* Used to get the connection capacity hscale value */
	gdouble speed;
	guint speed_as_int;

	// Either gtk_tree_model_get will malloc str or we manually malloc it
	//str = g_malloc0 (512 * sizeof (gchar));

	/*
	 * The Network Tab.
	 */

	widget = glade_xml_get_widget(xml_glob, "entry_nx_connection_name");
	strncpy (name, gtk_entry_get_text (GTK_ENTRY (widget)), NX_FIELDLEN);
	strncpy (nx_conn->ConnectionName, 
		 gtk_entry_get_text (GTK_ENTRY (widget)), NX_FIELDLEN);


	widget = glade_xml_get_widget(xml_glob, "entry_nx_server");
	strncpy (server, gtk_entry_get_text (GTK_ENTRY (widget)), NX_FIELDLEN);
	strncpy (nx_conn->ServerHost,
		 gtk_entry_get_text (GTK_ENTRY (widget)), NX_FIELDLEN);


	widget = glade_xml_get_widget (xml_glob, "spinbutton_nx_port");
	nx_conn->ServerPort = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (widget));

	widget = glade_xml_get_widget(xml_glob, "entry_nx_user");
	strncpy (nx_conn->User, 
		 gtk_entry_get_text (GTK_ENTRY (widget)), NX_FIELDLEN);
	strncpy (uname, gtk_entry_get_text (GTK_ENTRY (widget)), NX_FIELDLEN);

	widget = glade_xml_get_widget(xml_glob, "checkbutton_nx_save_password");
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (widget))) {
		nx_conn->RememberPassword = TRUE;
	} else {
		nx_conn->RememberPassword = FALSE;
	}

	widget = glade_xml_get_widget(xml_glob, "checkbutton_nx_disable_no_delay");
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (widget))) {
		nx_conn->DisableNoDelay = TRUE;
	} else {
		nx_conn->DisableNoDelay = FALSE;
	}

	widget = glade_xml_get_widget(xml_glob, "checkbutton_nx_disable_zlib");
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (widget))) {
		nx_conn->DisableZLIB = TRUE;
	} else {
		nx_conn->DisableZLIB = FALSE;
	}

	widget = glade_xml_get_widget(xml_glob, "checkbutton_nx_enable_ssl_for_all");
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (widget))) {
		nx_conn->EnableSSLOnly = TRUE;
	} else {
		nx_conn->EnableSSLOnly = FALSE;
	}

	widget = glade_xml_get_widget(xml_glob, "hscale_nx_connection_capacity");
	speed = gtk_range_get_value (GTK_RANGE (widget));
	if (speed > 5 || speed < 0) {
		/* error set to default of adsl (middle of the range) */
		strncpy (nx_conn->LinkSpeed, "adsl", NX_FIELDLEN);
	} else {
		speed_as_int = (guint)speed;
		switch (speed_as_int) {
		case 1:
			strncpy (nx_conn->LinkSpeed, "modem", NX_FIELDLEN);
			break;
		case 2:
			strncpy (nx_conn->LinkSpeed, "isdn", NX_FIELDLEN);
			break;
		case 3:
			strncpy (nx_conn->LinkSpeed, "adsl", NX_FIELDLEN);
			break;
		case 4:
			strncpy (nx_conn->LinkSpeed, "wan", NX_FIELDLEN);
			break;
		case 5:
			strncpy (nx_conn->LinkSpeed, "lan", NX_FIELDLEN);
			break;
		default:
			strncpy (nx_conn->LinkSpeed, "adsl", NX_FIELDLEN);
			break;
		}
	}


	/*
	 * The Desktop Tab 
	 */

	widget = glade_xml_get_widget (xml_glob, "combobox_nx_desktop_session");
	nxList = GTK_LIST_STORE (gtk_combo_box_get_model (GTK_COMBO_BOX (widget)));
	printerr ("Getting active iter for desktop session list..\n");

	if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (widget), &iter)) {		
		gtk_tree_model_get (GTK_TREE_MODEL (nxList), &iter, PROGRAM, &str, -1);
		strAllocatedGtk = TRUE;
	} else {
		if (strAllocatedMan == FALSE || strAllocatedGtk == TRUE) {
			str = g_realloc ((gpointer)str, NX_FIELDLEN * sizeof (gchar));
			strAllocatedMan = TRUE;
			strAllocatedGtk = FALSE;
		}
		/* If we can't get the string from the tree_model, then set it manually to a default */
		snprintf (str, 6, "%s", "GNOME");
	}

	if (strstr (str, _("KDE"))) {
		strncpy (nx_conn->Desktop, "kde", NX_FIELDLEN);
		strncpy (nx_conn->Session, "unix", NX_FIELDLEN);
		strncpy (nx_conn->CustomUnixDesktop, "console", NX_FIELDLEN);

	} else if (strstr (str, _("GNOME"))) {
		strncpy (nx_conn->Desktop, "gnome", NX_FIELDLEN);
		strncpy (nx_conn->Session, "unix", NX_FIELDLEN);
		strncpy (nx_conn->CustomUnixDesktop, "console", NX_FIELDLEN);

	} else if (strstr (str, _("CDE"))) {
		strncpy (nx_conn->Desktop, "cde", NX_FIELDLEN);
		strncpy (nx_conn->Session, "unix", NX_FIELDLEN);
		strncpy (nx_conn->CustomUnixDesktop, "console", NX_FIELDLEN);

	} else if (strstr (str, _("XDM"))) {
		strncpy (nx_conn->Desktop, "xdm", NX_FIELDLEN);
		strncpy (nx_conn->Session, "unix", NX_FIELDLEN);
		strncpy (nx_conn->CustomUnixDesktop, "console", NX_FIELDLEN);

	} else if (strstr (str, _("Console"))) {
		strncpy (nx_conn->Desktop, "console", NX_FIELDLEN);
		strncpy (nx_conn->Session, "unix", NX_FIELDLEN);
		strncpy (nx_conn->CustomUnixDesktop, "console", NX_FIELDLEN);

	} else if (strstr (str, _("Default X client script on server"))) {
		strncpy (nx_conn->Desktop, "console", NX_FIELDLEN);
		strncpy (nx_conn->Session, "unix", NX_FIELDLEN);
		strncpy (nx_conn->CustomUnixDesktop, "default", NX_FIELDLEN);

	} else if (strstr (str, _("Custom command"))) {
		strncpy (nx_conn->Desktop, "console", NX_FIELDLEN);
		strncpy (nx_conn->Session, "unix", NX_FIELDLEN);
		strncpy (nx_conn->CustomUnixDesktop, "application", NX_FIELDLEN);

	} else if (strstr (str, _("Windows"))) {
		strncpy (nx_conn->Desktop, "console", NX_FIELDLEN);
		strncpy (nx_conn->Session, "windows", NX_FIELDLEN);
		strncpy (nx_conn->CustomUnixDesktop, "application", NX_FIELDLEN);

	} else if (strstr (str, _("VNC"))) {
		strncpy (nx_conn->Desktop, "console", NX_FIELDLEN);
		strncpy (nx_conn->Session, "vnc", NX_FIELDLEN);
		strncpy (nx_conn->CustomUnixDesktop, "application", NX_FIELDLEN);

	} else if (strstr (str, _("Shadow"))) {
		strncpy (nx_conn->Desktop, "console", NX_FIELDLEN);
		strncpy (nx_conn->Session, "shadow", NX_FIELDLEN);
		strncpy (nx_conn->CustomUnixDesktop, "application", NX_FIELDLEN);

	} else {
		strncpy (nx_conn->Desktop, "kde", NX_FIELDLEN);
		strncpy (nx_conn->Session, "unix", NX_FIELDLEN);
		strncpy (nx_conn->CustomUnixDesktop, "console", NX_FIELDLEN);
	}

	widget = glade_xml_get_widget(xml_glob, "entry_nx_custom_command");
	strncpy (nx_conn->CommandLine, gtk_entry_get_text (GTK_ENTRY (widget)), CMDLINELEN);

	widget = glade_xml_get_widget(xml_glob, "radiobutton_nx_new_virtual_desktop");
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (widget))) {
		nx_conn->VirtualDesktop = TRUE;
	} else {
		nx_conn->VirtualDesktop = FALSE;
	}

	widget = glade_xml_get_widget(xml_glob, "checkbutton_nx_disable_x_agent");
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (widget))) {
		nx_conn->XAgentEncoding = FALSE;
	} else {
		nx_conn->XAgentEncoding = TRUE;
	}

	widget = glade_xml_get_widget(xml_glob, "checkbutton_nx_disable_x_replies");
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (widget))) {
		nx_conn->UseTaint = FALSE;
	} else {
		nx_conn->UseTaint = TRUE;
	}
	
	widget = glade_xml_get_widget (xml_glob, "combobox_nx_xdm_mode");
	nxList = GTK_LIST_STORE (gtk_combo_box_get_model (GTK_COMBO_BOX (widget)));
	printerr ("Getting active iter for xdm mode..\n");
	if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (widget), &iter)) {		
		gtk_tree_model_get (GTK_TREE_MODEL (nxList), &iter, PROGRAM, &str, -1);
		strAllocatedGtk = TRUE;
	} else {
		if (strAllocatedMan == FALSE || strAllocatedGtk == TRUE) {
			str = g_realloc ((gpointer)str, NX_FIELDLEN * sizeof (gchar));
			strAllocatedMan = TRUE;
			strAllocatedGtk = FALSE;
		}
		/* If we can't get the string from the tree_model, then set it manually to a default */
		snprintf (str, NX_FIELDLEN, "%s", _("Let NX server decide"));
	}

	if (strstr (str, _("Let NX server decide"))) {
		strncpy (nx_conn->XdmMode, "server decide", NX_FIELDLEN);

	} else if (strstr (str, _("Send an XDM query to a host"))) {
		strncpy (nx_conn->XdmMode, "query", NX_FIELDLEN);

	} else if (strstr (str, _("Broadcast an XDM query"))) {
		strncpy (nx_conn->XdmMode, "broadcast", NX_FIELDLEN);

	} else if (strstr (str, _("Get a list of available X display managers"))) {
		strncpy (nx_conn->XdmMode, "list", NX_FIELDLEN);

	} else {
		strncpy (nx_conn->XdmMode, "server decide", NX_FIELDLEN);
	}

	widget = glade_xml_get_widget(xml_glob, "entry_nx_xdm_host");
	strncpy (nx_conn->XdmHost, gtk_entry_get_text (GTK_ENTRY (widget)), NX_FIELDLEN);

	widget = glade_xml_get_widget (xml_glob, "spinbutton_nx_xdm_port");
	nx_conn->XdmPort = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (widget));

	widget = glade_xml_get_widget (xml_glob, "combobox_nx_window_size");
	nxList = GTK_LIST_STORE (gtk_combo_box_get_model (GTK_COMBO_BOX (widget)));
	printerr ("Getting active iter for nx window size..\n");
	if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (widget), &iter)) {		
		gtk_tree_model_get (GTK_TREE_MODEL (nxList), &iter, PROGRAM, &str, -1);
		strAllocatedGtk = TRUE;
	} else {
		if (strAllocatedMan == FALSE || strAllocatedGtk == TRUE) {
			str = g_realloc ((gpointer)str, NX_FIELDLEN * sizeof (gchar));
			strAllocatedMan = TRUE;
			strAllocatedGtk = FALSE;
		}
		/* If we can't get the string from the tree_model, then set it manually to a default */
		snprintf (str, NX_FIELDLEN, "%s", _("Full Screen"));
	}

	if (strstr (str, "640 x 480")) {
		nx_conn->FullScreen = FALSE;
		*fullscreen = 0;
		nx_conn->ResolutionWidth = 640;
		nx_conn->ResolutionHeight = 480;

	} else if (strstr (str, "800 x 600")) {
		nx_conn->FullScreen = FALSE;
		*fullscreen = 0;
		nx_conn->ResolutionWidth = 800;
		nx_conn->ResolutionHeight = 600;

	} else if (strstr (str, "1024 x 768")) {
		nx_conn->FullScreen = FALSE;
		*fullscreen = 0;
		nx_conn->ResolutionWidth = 1024;
		nx_conn->ResolutionHeight = 768;

	} else if (strstr (str, "1280 x 1024")) {
		nx_conn->FullScreen = FALSE;
		*fullscreen = 0;
		nx_conn->ResolutionWidth = 1280;
		nx_conn->ResolutionHeight = 1024;

	} else if (strstr (str, _("Custom"))) {
		nx_conn->FullScreen = FALSE;
		*fullscreen = 0;
		/* Set from widgets */
		widget = glade_xml_get_widget (xml_glob, "spinbutton_nx_width");
		nx_conn->ResolutionWidth = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (widget));
		widget = glade_xml_get_widget (xml_glob, "spinbutton_nx_height");
		nx_conn->ResolutionHeight = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (widget));

	} else if (strstr (str, _("Full Screen"))) {
		nx_conn->FullScreen = TRUE;
		*fullscreen = 1;
		nx_conn->ResolutionWidth = 800;
		nx_conn->ResolutionHeight = 600;

	} else {
		nx_conn->FullScreen = TRUE;
		*fullscreen = 1;
		nx_conn->ResolutionWidth = 800;
		nx_conn->ResolutionHeight = 600;
	}

	widget = glade_xml_get_widget (xml_glob, "combobox_nx_image_encoding");
	nxList = GTK_LIST_STORE (gtk_combo_box_get_model (GTK_COMBO_BOX (widget)));
	printerr ("Getting active iter for image encoding..\n");
	if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (widget), &iter)) {		
		gtk_tree_model_get (GTK_TREE_MODEL (nxList), &iter, PROGRAM, &str, -1);
		strAllocatedGtk = TRUE;
	} else {
		if (strAllocatedMan == FALSE || strAllocatedGtk == TRUE) {
			str = g_realloc ((gpointer)str, NX_FIELDLEN * sizeof (gchar));
			strAllocatedMan = TRUE;
			strAllocatedGtk = FALSE;
		}
		/* If we can't get the string from the tree_model, then set it manually to a default */
		snprintf (str, NX_FIELDLEN, "%s", _("Default"));
	}

	if (strstr (str, _("Default"))) {
		nx_conn->ImageEncoding = 0;
	} else if (strstr (str, _("PNG"))) {
		nx_conn->ImageEncoding = 1;
	} else if (strstr (str, _("JPEG"))) {
		nx_conn->ImageEncoding = 2;
	} else if (strstr (str, _("X Bitmap"))) {
		nx_conn->ImageEncoding = 3;
	} else {
		nx_conn->ImageEncoding = 0;
	}

	widget = glade_xml_get_widget (xml_glob, "spinbutton_nx_jpeg_quality");
	nx_conn->JPEGQuality = gtk_spin_button_get_value_as_int (GTK_SPIN_BUTTON (widget));

	/*
	 * Free up str here before using it below
	 */
	if (strAllocatedMan == TRUE || strAllocatedGtk == TRUE) {
		g_free (str);
	}

	/*
	 * The SSH Public Key
	 *
	 * We'll read this from the relevant window, which should default to the 
	 * standard NX public key.
	 */
	widget = glade_xml_get_widget (xml_glob, "textview_nx_key");
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (widget));
	printerr ("gtk_text_buffer_get_start_iter (buffer, &start);\n");
	gtk_text_buffer_get_start_iter (buffer, &start);
	printerr ("gtk_text_buffer_get_end_iter (buffer, &end);\n");
	gtk_text_buffer_get_end_iter (buffer, &end);

	str = NULL;
	printerr ("gtk_text_buffer_get_text (buffer, &start, &end, FALSE);\n");
	str = gtk_text_buffer_get_text (buffer, &start, &end, FALSE);
	if (str != NULL) {
		strncpy (nx_conn->PublicKey,
			 str,
			 SSLKEYLEN-1);
	} else {
		strncpy (nx_conn->PublicKey, NXDefaultKey, SSLKEYLEN-1);
	}

	/* Enable Sound Checkbutton */
	widget = glade_xml_get_widget(xml_glob, "checkbutton_nx_enable_sound");
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (widget))) {
		nx_conn->enableSound = TRUE;
	} else {
		nx_conn->enableSound = FALSE;
	}

	return;
}


/**
 * check that we have ~/.nx/config and ~/.nx/config/nxclient.conf 
 * 
 * This function gets called when hcconnect boots.
 */
void nx_check_config (void)
{
	struct stat * buf;
	buf = g_malloc0 (sizeof (struct stat));
	gchar tmp[1024];
	char* homepath;

	homepath = getenv("HOME");

	/* Check we have the required config files */
	snprintf (tmp, 1023, "%s/.nx/config", homepath);
	stat (tmp, buf);
	if (!S_ISDIR (buf->st_mode)) {
		snprintf (tmp, 1023, "mkdir -p %s/.nx/config", homepath);
		system (tmp);
		nx_writeout_nxclient_conf ();
	} else {
		snprintf (tmp, 1023, "%s/.nx/config/nxclient.conf", homepath);
		stat (tmp, buf);
		if (!S_ISREG (buf->st_mode)) {
			nx_writeout_nxclient_conf ();
		}
	}

	g_free (buf);

	return;
}


/**
 * Just writes out the nxclient.conf file
 */
gint nx_writeout_nxclient_conf (void)
{
	FILE * fp;
	char* homepath;
	gchar path[1024];

	homepath = getenv("HOME");
	snprintf (path, 1023, "%s/.nx/nxclient.conf", homepath);
	fp = fopen (path, "w");
	if (fp) {
		fwrite (nxclient_conf, sizeof (gchar), strlen (nxclient_conf),  fp); 
		fclose (fp);
		return 0;
	}

	return -1;
}


/*
 * Next functions are callbacks to handle the correct greying out
 * of controls, for example, greying out the extra settings, when a
 * GNOME desktop session is chosen.
 */


void set_custom_settings_sensitivity (void)
{
	GtkWidget * widget;
	GtkTreeIter iter;
	GtkListStore * nxList;
	gchar * str;
	gboolean rtn;

	widget = glade_xml_get_widget (xml_glob, "combobox_nx_desktop_session");
	nxList = GTK_LIST_STORE (gtk_combo_box_get_model (GTK_COMBO_BOX (widget)));
	rtn = gtk_combo_box_get_active_iter (GTK_COMBO_BOX (widget), &iter);
	if (!rtn) {
		return;
	}
	gtk_tree_model_get (GTK_TREE_MODEL (nxList), &iter, PROGRAM, &str, -1);

	if (strstr (str, _("KDE"))) {
		grey_all_custom_settings ();

	} else if (strstr (str, _("GNOME"))) {
		grey_all_custom_settings ();

	} else if (strstr (str, _("CDE"))) {
		grey_all_custom_settings ();

	} else if (strstr (str, _("Console"))) {
		/* ungrey options, grey command box */
		ungrey_custom_options ();
		widget = glade_xml_get_widget (xml_glob, "entry_nx_custom_command");
		gtk_widget_set_sensitive (widget, FALSE);
		widget = glade_xml_get_widget (xml_glob, "label_nx_custom_command");
		gtk_widget_set_sensitive (widget, FALSE);

	} else if (strstr (str, _("Default X client script on server"))) {
		/* ungrey options, grey command box */
		ungrey_custom_options ();
		widget = glade_xml_get_widget (xml_glob, "entry_nx_custom_command");
		gtk_widget_set_sensitive (widget, FALSE);
		widget = glade_xml_get_widget (xml_glob, "label_nx_custom_command");
		gtk_widget_set_sensitive (widget, FALSE);

	} else if (strstr (str, _("Custom command"))) {
		/* ungrey all custom settings */
		ungrey_custom_options ();
		widget = glade_xml_get_widget (xml_glob, "entry_nx_custom_command");
		gtk_widget_set_sensitive (widget, TRUE);
		widget = glade_xml_get_widget (xml_glob, "label_nx_custom_command");
		gtk_widget_set_sensitive (widget, TRUE);


	} else {
		grey_all_custom_settings ();
	}

	return;
}

void grey_all_custom_settings (void)
{
	GtkWidget * widget;

	widget = glade_xml_get_widget (xml_glob, "radiobutton_nx_floating_window");
	gtk_widget_set_sensitive (widget, FALSE);
	widget = glade_xml_get_widget (xml_glob, "radiobutton_nx_new_virtual_desktop");
	gtk_widget_set_sensitive (widget, FALSE);
	widget = glade_xml_get_widget (xml_glob, "checkbutton_nx_disable_x_agent");
	gtk_widget_set_sensitive (widget, FALSE);
	widget = glade_xml_get_widget (xml_glob, "checkbutton_nx_disable_x_replies");
	gtk_widget_set_sensitive (widget, FALSE);
	widget = glade_xml_get_widget (xml_glob, "frame_nx_custom_settings");
	gtk_widget_set_sensitive (widget, FALSE);

	return;
}

void ungrey_custom_options (void) 
{
	GtkWidget * widget;

	widget = glade_xml_get_widget (xml_glob, "frame_nx_custom_settings");
	gtk_widget_set_sensitive (widget, TRUE);
	widget = glade_xml_get_widget (xml_glob, "label_nx_custom_settings");
	gtk_widget_set_sensitive (widget, TRUE);
	widget = glade_xml_get_widget (xml_glob, "radiobutton_nx_floating_window");
	gtk_widget_set_sensitive (widget, TRUE);
	widget = glade_xml_get_widget (xml_glob, "radiobutton_nx_new_virtual_desktop");
	gtk_widget_set_sensitive (widget, TRUE);

	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget))) {
		/* desensitise checkbutton_nx_disable_x_agent 
		   and checkbutton_nx_disable_x_replies */
		widget = glade_xml_get_widget (xml_glob, "checkbutton_nx_disable_x_agent");
		gtk_widget_set_sensitive (widget, FALSE);
		widget = glade_xml_get_widget (xml_glob, "checkbutton_nx_disable_x_replies");
		gtk_widget_set_sensitive (widget, FALSE);
	} else {
		widget = glade_xml_get_widget (xml_glob, "checkbutton_nx_disable_x_agent");
		gtk_widget_set_sensitive (widget, TRUE);
		if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (widget))) {
			widget = glade_xml_get_widget (xml_glob, "checkbutton_nx_disable_x_replies");
			gtk_widget_set_sensitive (widget, TRUE);
		}
	}	
}

void set_xdm_settings (void)
{
	GtkWidget * widget;
	GtkTreeIter iter1;
	GtkListStore * nxList1;
	gchar * str1;
	gboolean rtn = FALSE;

	widget = glade_xml_get_widget (xml_glob, "frame_nx_xdm");
	gtk_widget_set_sensitive (widget, TRUE);
	widget = glade_xml_get_widget (xml_glob, "label_nx_xdm");
	gtk_widget_set_sensitive (widget, TRUE);

	widget = glade_xml_get_widget (xml_glob, "combobox_nx_xdm_mode");
	gtk_widget_set_sensitive (widget, TRUE);
	nxList1 = GTK_LIST_STORE (gtk_combo_box_get_model (GTK_COMBO_BOX (widget)));
	rtn = gtk_combo_box_get_active_iter (GTK_COMBO_BOX (widget), &iter1);
	if (!rtn) {
		return;
	} 

	gtk_tree_model_get (GTK_TREE_MODEL (nxList1), &iter1, PROGRAM, &str1, -1);

	if (strstr (str1, _("Let NX server decide"))) {
		widget = glade_xml_get_widget (xml_glob, "entry_nx_xdm_host");
		gtk_widget_set_sensitive (widget, FALSE);
		widget = glade_xml_get_widget (xml_glob, "spinbutton_nx_xdm_port");
		gtk_widget_set_sensitive (widget, FALSE);
		widget = glade_xml_get_widget (xml_glob, "label_nx_xdm_host");
		gtk_widget_set_sensitive (widget, FALSE);
		widget = glade_xml_get_widget (xml_glob, "label_nx_xdm_port");
		gtk_widget_set_sensitive (widget, FALSE);

	} else if (strstr (str1, _("Send an XDM query to a host"))) {
		widget = glade_xml_get_widget (xml_glob, "entry_nx_xdm_host");
		gtk_widget_set_sensitive (widget, TRUE);
		widget = glade_xml_get_widget (xml_glob, "spinbutton_nx_xdm_port");
		gtk_widget_set_sensitive (widget, TRUE);
		widget = glade_xml_get_widget (xml_glob, "label_nx_xdm_host");
		gtk_widget_set_sensitive (widget, TRUE);
		widget = glade_xml_get_widget (xml_glob, "label_nx_xdm_port");
		gtk_widget_set_sensitive (widget, TRUE);

	} else if (strstr (str1, _("Broadcast an XDM query"))) {
		widget = glade_xml_get_widget (xml_glob, "entry_nx_xdm_host");
		gtk_widget_set_sensitive (widget, FALSE);
		widget = glade_xml_get_widget (xml_glob, "spinbutton_nx_xdm_port");
		gtk_widget_set_sensitive (widget, TRUE);
		widget = glade_xml_get_widget (xml_glob, "label_nx_xdm_host");
		gtk_widget_set_sensitive (widget, FALSE);
		widget = glade_xml_get_widget (xml_glob, "label_nx_xdm_port");
		gtk_widget_set_sensitive (widget, TRUE);

	} else if (strstr (str1, _("Get a list of available X display managers"))) {
		widget = glade_xml_get_widget (xml_glob, "entry_nx_xdm_host");
		gtk_widget_set_sensitive (widget, TRUE);
		widget = glade_xml_get_widget (xml_glob, "spinbutton_nx_xdm_port");
		gtk_widget_set_sensitive (widget, TRUE);
		widget = glade_xml_get_widget (xml_glob, "label_nx_xdm_host");
		gtk_widget_set_sensitive (widget, TRUE);
		widget = glade_xml_get_widget (xml_glob, "label_nx_xdm_port");
		gtk_widget_set_sensitive (widget, TRUE);

	} else {
		widget = glade_xml_get_widget (xml_glob, "entry_nx_xdm_host");
		gtk_widget_set_sensitive (widget, FALSE);
		widget = glade_xml_get_widget (xml_glob, "spinbutton_nx_xdm_port");
		gtk_widget_set_sensitive (widget, FALSE);
		widget = glade_xml_get_widget (xml_glob, "label_nx_xdm_host");
		gtk_widget_set_sensitive (widget, FALSE);
		widget = glade_xml_get_widget (xml_glob, "label_nx_xdm_port");
		gtk_widget_set_sensitive (widget, FALSE);
	}

	return;
}


int set_xdm_settings_sensitivity_do_work (void)
{
	GtkWidget * widget;
	GtkTreeIter iter;
	GtkListStore * nxList;
	gchar * str;
	gint rtn = 0;
	
	widget = glade_xml_get_widget (xml_glob, "combobox_nx_desktop_session");
	nxList = GTK_LIST_STORE (gtk_combo_box_get_model (GTK_COMBO_BOX (widget)));
	if (gtk_combo_box_get_active_iter (GTK_COMBO_BOX (widget), &iter)) {
		gtk_tree_model_get (GTK_TREE_MODEL (nxList), &iter, PROGRAM, &str, -1);
	} else {
		str = g_malloc (NX_FIELDLEN * sizeof (gchar));
		snprintf (str, NX_FIELDLEN, "%s", _("GNOME"));
	}

	if (strstr (str, _("KDE"))) {
		grey_all_xdm_settings ();

	} else if (strstr (str, _("GNOME"))) {
		grey_all_xdm_settings ();

	} else if (strstr (str, _("CDE"))) {
		grey_all_xdm_settings ();

	} else if (strstr (str, _("XDM"))) {
		rtn = 10;

	} else if (strstr (str, _("Console"))) {
		grey_all_xdm_settings ();

	} else if (strstr (str, _("Default X client script on server"))) {
		grey_all_xdm_settings ();

	} else if (strstr (str, _("Custom command"))) {
		grey_all_xdm_settings ();
	} else {
		grey_all_xdm_settings ();
	}

	g_free (str);

	return rtn;
}

void set_xdm_settings_sensitivity (void)
{
	if (set_xdm_settings_sensitivity_do_work () == 10) {
		set_xdm_settings ();
	}
	return;
}


void grey_all_xdm_settings (void)
{
	GtkWidget * widget;

	widget = glade_xml_get_widget (xml_glob, "combobox_nx_xdm_mode");
	gtk_widget_set_sensitive (widget, FALSE);
	widget = glade_xml_get_widget (xml_glob, "entry_nx_xdm_host");
	gtk_widget_set_sensitive (widget, FALSE);
	widget = glade_xml_get_widget (xml_glob, "spinbutton_nx_xdm_port");
	gtk_widget_set_sensitive (widget, FALSE);
	widget = glade_xml_get_widget (xml_glob, "label_nx_xdm_host");
	gtk_widget_set_sensitive (widget, FALSE);
	widget = glade_xml_get_widget (xml_glob, "label_nx_xdm_port");
	gtk_widget_set_sensitive (widget, FALSE);
	widget = glade_xml_get_widget (xml_glob, "frame_nx_xdm");
	gtk_widget_set_sensitive (widget, FALSE);

	return;
}

void on_combobox_nx_desktop_session_changed (GtkComboBox * box)
{
	set_xdm_settings_sensitivity ();
	set_custom_settings_sensitivity ();
	return;
}

void on_combobox_nx_xdm_mode_changed (GtkComboBox * box)
{
	set_xdm_settings_sensitivity ();
}

void on_radiobutton_nx_new_virtual_desktop_toggled (GtkToggleButton * button)
{
	set_custom_settings_sensitivity ();
}

void on_checkbutton_nx_disable_x_agent_toggled (GtkToggleButton * button)
{
	GtkWidget * widget;

	widget = glade_xml_get_widget(xml_glob, "checkbutton_nx_disable_x_agent");
	gtk_widget_set_sensitive (widget, TRUE);
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (widget))) {
		widget = glade_xml_get_widget(xml_glob, "checkbutton_nx_disable_x_replies");
		gtk_widget_set_sensitive (widget, TRUE);
	} else {
		widget = glade_xml_get_widget(xml_glob, "checkbutton_nx_disable_x_replies");
		gtk_widget_set_sensitive (widget, FALSE);
	}
}

void on_combobox_nx_window_size_changed (GtkComboBox * box)
{
	GtkWidget * widget;
	GtkTreeIter iter;
	GtkListStore * nxList;
	gchar * str;
	
	//str = g_malloc0 (512 * sizeof (gchar));

	nxList = GTK_LIST_STORE (gtk_combo_box_get_model (box));
	if (gtk_combo_box_get_active_iter (box, &iter)) {
		gtk_tree_model_get (GTK_TREE_MODEL (nxList), &iter, PROGRAM, &str, -1);
	} else {
		str = g_malloc0 (NX_FIELDLEN * sizeof (gchar));
		snprintf (str, NX_FIELDLEN, "%s", _("Full Screen"));
	}

	if (strstr (str, "640 x 480")) {
		widget =  glade_xml_get_widget (xml_glob, "spinbutton_nx_width");
		gtk_widget_set_sensitive (widget, FALSE);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (widget), (gdouble)640);
		widget =  glade_xml_get_widget (xml_glob, "spinbutton_nx_height");
		gtk_widget_set_sensitive (widget, FALSE);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (widget), (gdouble)480);

	} else if (strstr (str, "800 x 600")) {
		widget =  glade_xml_get_widget (xml_glob, "spinbutton_nx_width");
		gtk_widget_set_sensitive (widget, FALSE);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (widget), (gdouble)800);
		widget =  glade_xml_get_widget (xml_glob, "spinbutton_nx_height");
		gtk_widget_set_sensitive (widget, FALSE);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (widget), (gdouble)600);

	} else if (strstr (str, "1024 x 768")) {
		widget =  glade_xml_get_widget (xml_glob, "spinbutton_nx_width");
		gtk_widget_set_sensitive (widget, FALSE);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (widget), (gdouble)1024);
		widget =  glade_xml_get_widget (xml_glob, "spinbutton_nx_height");
		gtk_widget_set_sensitive (widget, FALSE);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (widget), (gdouble)768);

	} else if (strstr (str, "1280 x 1024")) {
		widget =  glade_xml_get_widget (xml_glob, "spinbutton_nx_width");
		gtk_widget_set_sensitive (widget, FALSE);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (widget), (gdouble)1280);
		widget =  glade_xml_get_widget (xml_glob, "spinbutton_nx_height");
		gtk_widget_set_sensitive (widget, FALSE);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (widget), (gdouble)1024);

	} else if (strstr (str, _("Custom"))) {
		widget =  glade_xml_get_widget (xml_glob, "spinbutton_nx_width");
		gtk_widget_set_sensitive (widget, TRUE);
		widget =  glade_xml_get_widget (xml_glob, "spinbutton_nx_height");
		gtk_widget_set_sensitive (widget, TRUE);

	} else if (strstr (str, _("Full Screen"))) {
		widget =  glade_xml_get_widget (xml_glob, "spinbutton_nx_width");
		gtk_widget_set_sensitive (widget, FALSE);
		widget =  glade_xml_get_widget (xml_glob, "spinbutton_nx_height");
		gtk_widget_set_sensitive (widget, FALSE);

	} else {

	}

	g_free (str);

	return;
}


void on_combobox_nx_image_encoding_changed (GtkComboBox * box)
{
	GtkWidget * widget;
	GtkTreeIter iter;
	GtkListStore * nxList;
	gchar * str;

	nxList = GTK_LIST_STORE (gtk_combo_box_get_model (box));
	if (gtk_combo_box_get_active_iter (box, &iter)) {
		gtk_tree_model_get (GTK_TREE_MODEL (nxList), &iter, PROGRAM, &str, -1);
	} else {
		str = g_malloc0 (8 * sizeof (gchar));
		snprintf (str, 5, "%s", "null");
	}

	if (strstr (str, "JPEG")) {
		widget =  glade_xml_get_widget (xml_glob, "spinbutton_nx_jpeg_quality");
		gtk_widget_set_sensitive (widget, TRUE);
		widget =  glade_xml_get_widget (xml_glob, "label_nx_jpeg_quality");
		gtk_widget_set_sensitive (widget, TRUE);

	} else {
		widget =  glade_xml_get_widget (xml_glob, "spinbutton_nx_jpeg_quality");
		gtk_widget_set_sensitive (widget, FALSE);
		widget =  glade_xml_get_widget (xml_glob, "label_nx_jpeg_quality");
		gtk_widget_set_sensitive (widget, FALSE);

	}
	
	g_free (str);
}


void on_hscale_nx_connection_capacity_value_changed (GtkWidget * w)
{
	GtkWidget * label;
	gdouble speed;
	guint speed_as_int;

	speed = gtk_range_get_value (GTK_RANGE (w));
	label = glade_xml_get_widget(xml_glob, "label_nx_connection_capacity");

	if (speed > 5 || speed < 0) {
		/* error set to default of adsl (middle of the range) */
		gtk_label_set_text (GTK_LABEL (label), _("Connection capacity: ADSL"));
	} else {
		speed_as_int = (guint)speed;
		switch (speed_as_int) {
		case 1:
			gtk_label_set_text (GTK_LABEL (label), _("Connection capacity: Modem"));
			break;
		case 2:
			gtk_label_set_text (GTK_LABEL (label), _("Connection capacity: ISDN"));
			break;
		case 3:
			gtk_label_set_text (GTK_LABEL (label), _("Connection capacity: ADSL"));
			break;
		case 4:
			gtk_label_set_text (GTK_LABEL (label), _("Connection capacity: WAN"));
			break;
		case 5:
			gtk_label_set_text (GTK_LABEL (label), _("Connection capacity: LAN"));
			break;
		default:
			gtk_label_set_text (GTK_LABEL (label), _("Connection capacity: ADSL"));
			break;
		}
	}	
}

void on_button_nx_revert_ssl_key_clicked (GtkButton * button)
{
	GtkWidget * widget;
	GtkTextBuffer * buffer;
	GtkTextIter start, end;

	widget = glade_xml_get_widget (xml_glob, "textview_nx_key");
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (widget));

	/* Delete the text first*/
	gtk_text_buffer_get_start_iter (buffer, &start);
	gtk_text_buffer_get_end_iter (buffer, &end);
	gtk_text_buffer_delete (buffer, &start, &end);

	gtk_text_buffer_set_text (buffer, NXDefaultKey, -1);
	return;
}

void on_nx_key_file_chosen (GtkButton * button)
{
	GtkWidget * widget, * fdlg;
	GtkTextBuffer * buffer;
	GtkTextIter start, end;
	FILE * fp;
	gchar * key;
	size_t count;

	fdlg = glade_xml_get_widget (xml_glob, "filechooserdialog_nx_key");	

	/* open the file, copy contents to window, close filechooserdialog */
	fp = fopen (gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (fdlg)), "r");
	if (!fp) {
		/* Couldn't open the file */
		nx_general_error ("Couldn't open file", "File Error");
		return;
	}
	
	key = g_malloc0 (SSLKEYLEN * sizeof (gchar));

	count = fread (key, 1, SSLKEYLEN-1, fp);
	/* debug */
	printerr ("read %d chars\n", count);

	if (ferror (fp) != 0) {
		nx_general_error ("File input/output error", "File Error");	
		fclose (fp);
		return;
	}

	fclose (fp);

	widget = glade_xml_get_widget (xml_glob, "textview_nx_key");
	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (widget));

	/* set new key */
	gtk_text_buffer_get_start_iter (buffer, &start);
	gtk_text_buffer_get_end_iter (buffer, &end);
	gtk_text_buffer_delete (buffer, &start, &end);
	gtk_text_buffer_set_text (buffer, key, -1);

	g_free (key);

	gtk_widget_hide (fdlg);

	return;
}




