/* This file is part of the KDE project
   Copyright (C) 2001 Joseph Wenninger <jowenn@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/
#ifndef _konqsidebarplugin_h_
#define _konqsidebarplugin_h_
#include <qwidget.h>
#include <qobject.h>
#include <kurl.h>
#include <qstring.h>
#include <kparts/part.h>
#include <kparts/browserextension.h>
#include <kio/job.h>

class KonqSidebar_PluginInterface;

class KonqSidebarPlugin : public QObject
{
	Q_OBJECT
	public:
		KonqSidebarPlugin(QObject *parent,QWidget *widgetParent,QString &desktopName_, const char* name=0):QObject(parent,name),desktopName(desktopName_){;}
		~KonqSidebarPlugin(){;}
		virtual QWidget *getWidget()=0;
		virtual void *provides(const QString &)=0;
                KonqSidebar_PluginInterface *getInterfaces();   
	protected:
		virtual void handleURL(const KURL &url)=0;
		QString desktopName;
	signals:
		void requestURL(KURL&);
		void started(KIO::Job *);
		void completed();
		
	public slots:
		void openURL(const KURL& url){handleURL(url);}
};

class KonqSidebar_PluginInterface
	{
		public:
		KonqSidebar_PluginInterface(){;}
		virtual ~KonqSidebar_PluginInterface(){;}
		virtual KParts::ReadOnlyPart *getPart()=0;
		virtual KParts::BrowserExtension *getExtension()=0;
  		virtual KInstance *getInstance()=0; 
	};

#endif
