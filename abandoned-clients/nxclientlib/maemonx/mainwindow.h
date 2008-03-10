/***************************************************************************
                                mainwindow.h
                             -------------------
    begin                : Saturday 17th February 2007
    copyright            : (C) 2007 by George Wright
    email                : gwright@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

/* The main function which creates all the GUI components */

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <gtk/gtk.h>

void setup_gui(GtkWidget *window);
void setup_config(GtkWidget *window);

/* Callback functions */

static void config_clicked(GtkWidget *widget, gpointer data);
static void login_clicked(GtkWidget *widget, gpointer data);

#endif
