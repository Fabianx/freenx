/***************************************************************************
                               qtnxsessions.h
                             -------------------
    begin                : Wednesday August 16th 2006
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

#ifndef _QTNXSESSIONS_H_
#define _QTNXSESSIONS_H_

#include <QList>
#include <QTreeWidget>
#include <QTreeWidgetItem>

#include "nxdata.h"

#include "ui_sessionsdialog.h"

class QtNXSessions : public QDialog
{
	Q_OBJECT
	public:
		QtNXSessions(QList<NXResumeData>);
		~QtNXSessions();
	public slots:
		void pressedNew();
		void pressedResume();
	signals:
		void newPressed();
		void resumePressed(QString);
	private:
		void empty();
		
		Ui_SessionsDialog ui_sd;
		QList<QTreeWidgetItem*> sessionItems;
};

#endif
