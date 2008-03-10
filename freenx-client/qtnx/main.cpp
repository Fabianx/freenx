/***************************************************************************
                                  main.cpp
                            -------------------
        begin                : Thursday August 3rd 2006
        copyright            : (C) 2006 by George Wright
                               (C) 2007 Defuturo Ltd
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
#include <QApplication>

#include "qtnxwindow.h"

ofstream debugLogFile;

int main(int argc, char *argv[])
{
    debugLogFile.open("/tmp/nxcl.log", ios::out|ios::trunc);

    QApplication app(argc, argv);

    QtNXWindow *mw = new QtNXWindow();
    mw->show();

    return app.exec();
}

