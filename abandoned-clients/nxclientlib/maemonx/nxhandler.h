/***************************************************************************
                                nxhandler.h
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

/*
 * An accessor class that handles NXClientLib and manages the session and
 * configuration data
 */

#ifndef NXHANDLER_H
#define NXHANDLER_H

#include <QObject>

#include "nxclientlib.h"
#include "nxdata.h"

class NXHandler : public QObject
{
    Q_OBJECT
    public:
        NXHandler();
        ~NXHandler();

        void startConnect();

    private:
        NXSessionData m_session;
        NXConfigData m_config;
        NXClientLib m_nxclient;

};
#endif

