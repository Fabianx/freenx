/***************************************************************************
                               qtnxsettings.h
                             -------------------
    begin                : Saturday August 12th 2006
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

#ifndef _QTNXSETTINGS_H_
#define _QTNXSETTINGS_H_

#include <QDialog>

#include "nxdata.h"

#include "ui_settingsdialog.h"
#include "ui_keydialog.h"

class QtNXSettings : public QDialog
{
	Q_OBJECT
	public:
		QtNXSettings(QString);
		~QtNXSettings();
		void parseFile();
	public slots:
		void resolutionChanged(QString);
		void compressionChanged(QString);
		void platformChanged(QString);
		void typeChanged(QString);
		void keyChanged(int);
		void applyPressed();
		void cancelPressed();
		void okPressed();
		void setData(NXConfigData data) { config = data; };
		void authKeyPressed();
		void keyDialogAccept();
		void keyDialogReject();
	signals:
		void closing();
	private:
		Ui::SettingsDialog ui_sd;
		Ui::KeyDialog ui_kd;
		QDialog *keyDialog;
		NXConfigData config;
		QString fileName;
		QString filedesc;

};

#endif
