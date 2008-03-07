/***************************************************************************
                               qtnxwindow.h
                             -------------------
    begin                : Thursday August 3rd 2006
    copyright            : (C) 2006 by George Wright
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

#ifndef _QTNXWINDOW_H_
#define _QTNXWINDOW_H_

#include <QMainWindow>

#include "ui_mainwindow.h"

#include "nxclientlib.h"

class QtNXWindow : public QMainWindow
{
	Q_OBJECT
	public:
		QtNXWindow();
		~QtNXWindow();
	public slots:
		void connectPressed();
	private:
		Ui::MainWindow ui_mw;
		NXClientLib m_lib;
		NXSessionData *session;
};

#endif
