/***************************************************************************
                             konqsidebartest.cpp
                             -------------------
    begin                : Sat June 2 16:25:27 CEST 2001
    copyright            : (C) 2001 Joseph Wenninger
    email                : jowenn@kde.org
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/
#include "konqsidebartest.h"
#include "konqsidebartest.moc"

extern "C"
{
    void* create_konqsidebartest(QObject *par,QWidget *widp,QString &desktopname,const char *name)
    {
        return new SidebarTest(par,widp,desktopname,name);
    }
};
