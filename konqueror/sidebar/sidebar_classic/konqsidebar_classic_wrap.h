/***************************************************************************
                         konqsidebar_classic_wrap.h
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
#ifndef _konq_sidebar_test_h_
#define _konq_sidebar_test_h_
#include <konqsidebarplugin.h>
#include <qlabel.h>
#include <qlayout.h>
#include <kparts/part.h>
#include <kparts/factory.h>
#include <kparts/browserextension.h>
#include <klibloader.h>
#include <kxmlgui.h>
class SidebarClassic : public KonqSidebarPlugin
	{
		Q_OBJECT
		public:
		SidebarClassic(QObject *parent,QWidget *widgetParent, QString &desktopName_, const char* name=0):
                   KonqSidebarPlugin(parent,widgetParent,desktopName_,name)
		{

		    KLibFactory *lfactory = KLibLoader::self()->factory("libkonqtree");
		    if (lfactory)
		    {
			dirtree = static_cast<KParts::ReadOnlyPart *>
			(static_cast<KParts::Factory *>(lfactory)->createPart(widgetParent,"sidebar classic",parent));
        	}
	}
		~SidebarClassic(){;}
		virtual void *provides(const QString &pro)
		{
			if (pro=="KParts::ReadOnlyPart") return dirtree;
			if (pro=="KParts::BrowserExtension") return KParts::BrowserExtension::childObject(dirtree);  
			return 0;
		}
                virtual QWidget *getWidget(){return dirtree->widget();}   
		protected:
			KParts::ReadOnlyPart *dirtree;
			virtual void handleURL(const KURL &url)
				{
					dirtree->openURL(url);
//					widget->setText(url.url());
				}
	};

#endif
