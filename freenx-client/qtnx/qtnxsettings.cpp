/***************************************************************************
  qtnxsettings.cpp
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

#include <QFile>
#include <QDir>

#include "qtnxsettings.h"

#include "nxdata.h"
#include "nxparsexml.h"
#include "nxwritexml.h"

using namespace nxcl;

QtNXSettings::QtNXSettings(QString sessionName) : QDialog()
{
    filedesc = sessionName;
    keyDialog = 0;

    if (!sessionName.isEmpty())
        fileName = QDir::homePath() + "/.qtnx/" + sessionName + ".nxml";
    else
        fileName = "";

    ui_sd.setupUi(this);
    ui_sd.sessionName->setText(sessionName);

    parseFile();

    connect(ui_sd.resolution, SIGNAL(currentIndexChanged(QString)), this, SLOT(resolutionChanged(QString)));
    connect(ui_sd.imageCompressionType, SIGNAL(currentIndexChanged(QString)), this, SLOT(compressionChanged(QString)));
    connect(ui_sd.defaultKey, SIGNAL(stateChanged(int)), this, SLOT(keyChanged(int)));
    connect(ui_sd.applyButton, SIGNAL(pressed()), this, SLOT(applyPressed()));
    connect(ui_sd.okButton, SIGNAL(pressed()), this, SLOT(okPressed()));
    connect(ui_sd.cancelButton, SIGNAL(pressed()), this, SLOT(cancelPressed()));
    connect(ui_sd.setAuthKeyButton, SIGNAL(pressed()), this, SLOT(authKeyPressed()));
}

QtNXSettings::~QtNXSettings()
{
}

void QtNXSettings::parseFile()
{
    if (!fileName.isEmpty()) {
        NXParseXML handler;
        handler.setData(&config);

        QFile file(fileName);
        QXmlInputSource inputSource(&file);

        QXmlSimpleReader reader;
        reader.setContentHandler(&handler);
        reader.setErrorHandler(&handler);
        reader.parse(inputSource);

        ui_sd.hostname->setText(QString::fromStdString(config.serverHost));
        ui_sd.port->setValue(config.serverPort);

        if (config.key.empty())
            ui_sd.defaultKey->setChecked(true);
        else {
            ui_sd.defaultKey->setChecked(false);
            ui_sd.setAuthKeyButton->setEnabled(true);
        }

        if (config.sessionType == "unix-kde") {
            ui_sd.platform->setCurrentIndex(ui_sd.platform->findText(tr("UNIX")));
            ui_sd.type->setCurrentIndex(ui_sd.type->findText(tr("KDE")));
        } else if (config.sessionType == "unix-gnome") {
            ui_sd.platform->setCurrentIndex(ui_sd.platform->findText(tr("UNIX")));
            ui_sd.type->setCurrentIndex(ui_sd.type->findText(tr("GNOME")));
        } else if (config.sessionType == "unix-cde") {
            ui_sd.platform->setCurrentIndex(ui_sd.platform->findText(tr("UNIX")));
            ui_sd.type->setCurrentIndex(ui_sd.type->findText(tr("CDE")));
        } else if (config.sessionType == "unix-application") {
            ui_sd.platform->setCurrentIndex(ui_sd.platform->findText(tr("UNIX")));
            ui_sd.type->setCurrentIndex(ui_sd.type->findText(tr("Custom")));
            ui_sd.desktopSettingButton->setEnabled(true);
        }

        if (config.linkType == "modem")
            ui_sd.link->setCurrentIndex(ui_sd.link->findText(tr("Modem")));
        else if (config.linkType == "isdn")
            ui_sd.link->setCurrentIndex(ui_sd.link->findText(tr("ISDN")));
        else if (config.linkType == "adsl")
            ui_sd.link->setCurrentIndex(ui_sd.link->findText(tr("ADSL")));
        else if (config.linkType == "wan")
            ui_sd.link->setCurrentIndex(ui_sd.link->findText(tr("WAN")));
        else if (config.linkType == "lan")
            ui_sd.link->setCurrentIndex(ui_sd.link->findText(tr("LAN")));

        if (config.imageCompressionMethod == -1) {
            ui_sd.imageCompressionType->setCurrentIndex(ui_sd.imageCompressionType->findText(tr("JPEG")));
            ui_sd.imageQualityLevel->setValue(config.imageCompressionLevel);
            ui_sd.imageQualityLevel->setEnabled(true);
        } else if (config.imageCompressionMethod == 2)
            ui_sd.imageCompressionType->setCurrentIndex(ui_sd.imageCompressionType->findText(tr("PNG")));
        else if (config.imageCompressionMethod == 0)
            ui_sd.imageCompressionType->setCurrentIndex(ui_sd.imageCompressionType->findText(tr("Raw X11")));

        if (config.geometry == "640x480+0+0")
            ui_sd.resolution->setCurrentIndex(ui_sd.resolution->findText("640x480"));
        else if (config.geometry == "800x600+0+0")
            ui_sd.resolution->setCurrentIndex(ui_sd.resolution->findText("800x600"));
        else if (config.geometry == "1024x768+0+0")
            ui_sd.resolution->setCurrentIndex(ui_sd.resolution->findText("1024x768"));
        else {
            if (config.fullscreen) {
                ui_sd.resolution->setCurrentIndex(ui_sd.resolution->findText(tr("Fullscreen")));
            } else {
                ui_sd.resolution->setCurrentIndex(ui_sd.resolution->findText(tr("Custom")));

                QString res;
                res = QString::fromStdString(config.geometry).left(config.geometry.length() - 4);
                ui_sd.width->setValue(res.split('x').at(0).toInt());
                ui_sd.height->setValue(res.split('x').at(1).toInt());
                ui_sd.width->setEnabled(true);
                ui_sd.height->setEnabled(true);
            }
        }

        ui_sd.encryption->setChecked(config.encryption);
        ui_sd.memoryCache->setValue(config.cache);
        ui_sd.diskCache->setValue(config.images);

        ui_sd.render->setChecked(config.render);
    }
}

void QtNXSettings::resolutionChanged(QString text)
{
    if (text == tr("Custom")) {
        ui_sd.width->setEnabled(true);
        ui_sd.height->setEnabled(true);
    } else {
        ui_sd.width->setEnabled(false);
        ui_sd.height->setEnabled(false);
    }
}

void QtNXSettings::compressionChanged(QString text)
{
    if (text == tr("JPEG")) {
        ui_sd.imageQualityLevel->setEnabled(true);
    } else {
        ui_sd.imageQualityLevel->setEnabled(false);
    }
}

void QtNXSettings::platformChanged(QString text)
{
}

void QtNXSettings::typeChanged(QString text)
{
}

void QtNXSettings::keyChanged(int state)
{
    if (state == Qt::Checked) {
        config.key = "";
        ui_sd.setAuthKeyButton->setEnabled(false);
    } else
        ui_sd.setAuthKeyButton->setEnabled(true);
}

void QtNXSettings::cancelPressed()
{
    close();
}

void QtNXSettings::okPressed()
{
    applyPressed();
    emit closing(ui_sd.sessionName->text());
    close();
}

void QtNXSettings::authKeyPressed()
{
    keyDialog = 0;
    delete keyDialog;
    keyDialog = new QDialog(this);
    ui_kd.setupUi(keyDialog);
    keyDialog->show();
    QTextDocument *doc_key = new QTextDocument(QString::fromStdString(config.key));
    ui_kd.key->setDocument(doc_key);

    connect(keyDialog, SIGNAL(accepted()), this, SLOT(keyDialogAccept()));
}

void QtNXSettings::keyDialogAccept()
{

    config.key = ui_kd.key->document()->toPlainText().toStdString();
}

void QtNXSettings::keyDialogReject()
{
}

void QtNXSettings::applyPressed()
{
    // File has been renamed, remove old one
    if (filedesc != ui_sd.sessionName->text()) {
        QFile temp(QDir::homePath() + "/.qtnx/" + filedesc + ".nxml");
        temp.remove();
    }

    QDir configDir(QDir::homePath() + "/.qtnx/");
    configDir.mkpath(QDir::homePath() + "/.qtnx/");

    config.sessionName = ui_sd.sessionName->text().toStdString();
    config.serverHost = ui_sd.hostname->text().toStdString();
    config.serverPort = ui_sd.port->value();

    // TODO: Add keyboard selection support
    config.keyboard = "defkeymap";
    config.kbtype = "pc102/defkeymap";

    if (ui_sd.platform->currentText() == tr("UNIX")) {
        if (ui_sd.type->currentText() == tr("KDE"))
            config.sessionType = "unix-kde";
        else if (ui_sd.type->currentText() == tr("GNOME"))
            config.sessionType = "unix-gnome";
        else if (ui_sd.type->currentText() == tr("CDE"))
            config.sessionType = "unix-cde";
        else if (ui_sd.type->currentText() == tr("Custom"))
            config.sessionType = "unix-application";
    }

    if (ui_sd.link->currentText() == tr("Modem"))
        config.linkType = "modem";
    else if (ui_sd.link->currentText() == tr("ISDN"))
        config.linkType = "isdn";
    else if (ui_sd.link->currentText() == tr("ADSL"))
        config.linkType = "adsl";
    else if (ui_sd.link->currentText() == tr("WAN"))
        config.linkType = "wan";
    else if (ui_sd.link->currentText() == tr("LAN"))
        config.linkType = "lan";

    if (ui_sd.imageCompressionType->currentText() == tr("JPEG")) {
        config.imageCompressionMethod = -1;
        config.imageCompressionLevel = ui_sd.imageQualityLevel->value();
    } else if (ui_sd.imageCompressionType->currentText() == tr("PNG"))
        config.imageCompressionMethod = 2;
    else if (ui_sd.imageCompressionType->currentText() == tr("Raw X11"))
        config.imageCompressionMethod = 0;

    if (ui_sd.resolution->currentText() == tr("Fullscreen"))
        config.fullscreen = true;
    else if (ui_sd.resolution->currentText() == tr("Custom")) {
        config.fullscreen = false;
        config.geometry = (QString::number(ui_sd.width->value()) + "x" + QString::number(ui_sd.height->value()) + "+0+0").toStdString();
    } else {
        config.fullscreen = false;
        config.geometry = (ui_sd.resolution->currentText() + "+0+0").toStdString();
    }

    if (ui_sd.encryption->checkState() == Qt::Checked)
        config.encryption = true;
    else
        config.encryption = false;

    config.cache = ui_sd.memoryCache->value();
    config.images = ui_sd.diskCache->value();

    if (ui_sd.render->checkState() == Qt::Checked)
        config.render = true;
    else
        config.render = false;

    NXWriteXML writeData;
    writeData.setSessionData(config);
    writeData.write(QDir::homePath() + "/.qtnx/" + ui_sd.sessionName->text() + ".nxml");
}
