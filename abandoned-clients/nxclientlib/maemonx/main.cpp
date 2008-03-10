/***************************************************************************
                                  main.cpp
                             -------------------
    begin                : Saturday February 17th 2007
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

#include <hildon-widgets/hildon-program.h>
#include <gtk/gtkmain.h>

#include "mainwindow.h"

static void destroy(GtkWidget *widget, gpointer data);

int main(int argc, char **argv)
{
    HildonProgram *program;
    HildonWindow *window;

    gtk_init(&argc, &argv);

    program = HILDON_PROGRAM(hildon_program_get_instance());
    g_set_application_name("NX Client");

    window = HILDON_WINDOW(hildon_window_new());
    g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(destroy), NULL);

    hildon_program_add_window(program, window);

    setup_gui(GTK_WIDGET(window));

    gtk_widget_show_all(GTK_WIDGET(window));

    gtk_main();

    return 0;
}

static void destroy(GtkWidget *widget, gpointer data)
{
    gtk_main_quit();
}
