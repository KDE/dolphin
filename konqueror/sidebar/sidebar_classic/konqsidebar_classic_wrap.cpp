/***************************************************************************
                        konqsidebar_classic_wrap.cpp
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
#include "konqsidebar_classic_wrap.h"
#include <klocale.h>
#include "konqsidebar_classic_wrap.moc"

extern "C"
{
    void* create_konqsidebar_classic_wrap(QObject *par,QWidget *widp,QString &desktopname,const char *name)
    {
        return new SidebarClassic(par,widp,desktopname,name);
    }
};

extern "C"
{
   bool add_konqsidebar_classic_wrap(QString* fn, QString*, QMap<QString,QString> *map)
   {
        map->insert("Type","Link");
	map->insert("Icon","view_sidetree");
	map->insert("Name",i18n("Classic Sidebar"));
 	map->insert("Open","false");
	map->insert("X-KDE-KonqSidebarModule","konqsidebar_classic_wrap");
	fn->setLatin1("sidebar_classic%1.desktop");
        return true;
   }
};
