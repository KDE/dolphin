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
#include "konqsidebartest.moc"
#include <kinstance.h>

extern "C"
{
    KDE_EXPORT void* create_konq_sidebartest(KInstance *instance,QObject *par,QWidget *widp,QString &desktopname,const char *name)
    {
        return new SidebarTest(instance,par,widp,desktopname,name);
    }
};
