/* This file is part of the KDE project
   Copyright (C) 2003, George Staikos <staikos@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "web_module.h"
#include <qfileinfo.h>

#include <khtml_part.h>
#include <klocale.h>
#include <kdebug.h>
#include <kstddirs.h>
#include <kglobal.h>


KonqSideBarWebModule::KonqSideBarWebModule(KInstance *instance, QObject *parent, QWidget *widgetParent, QString &desktopName, const char* name)
	: KonqSidebarPlugin(instance, parent, widgetParent, desktopName, name)
{
	_htmlPart = new KHTMLPart;
//	connect(_htmlPart, SIGNAL(setWindowCaption(const QString&)), this, SLOT());
//	connect(_htmlPart, SIGNAL(completed()), this, SLOT());
	KSimpleConfig ksc(desktopName);
	ksc.setGroup("Desktop Entry");

	_htmlPart->openURL(ksc.readEntry("URL"));
}


KonqSideBarWebModule::~KonqSideBarWebModule() {
	delete _htmlPart;
	_htmlPart = 0L;
}


QWidget *KonqSideBarWebModule::getWidget() {
	return _htmlPart->widget();
}


void *KonqSideBarWebModule::provides(const QString &) {
	return 0L;
}


void KonqSideBarWebModule::handleURL(const KURL &) {
}


extern "C" {
	KonqSidebarPlugin* create_konqsidebar_web(KInstance *instance, QObject *parent, QWidget *widget, QString &desktopName, const char *name) {
		return new KonqSideBarWebModule(instance, parent, widget, desktopName, name);
	} 
}


extern "C" {
	bool add_konqsidebar_web(QString* fn, QString* param, QMap<QString,QString> *map) {
		KGlobal::dirs()->addResourceType("websidebardata", KStandardDirs::kde_default("data") + "konqsidebartng/websidebar");
		KURL url;
		url.setProtocol("file");
		QStringList paths = KGlobal::dirs()->resourceDirs("websidebardata");
		for (QStringList::Iterator i = paths.begin(); i != paths.end(); ++i) {
			if (QFileInfo(*i + "websidebar.html").exists()) {
				url.setPath(*i + "websidebar.html");
				break;
			}
		}

		if (url.path().isEmpty())
			return false;
		map->insert("Type", "Link");
		map->insert("URL", url.url());
		map->insert("Icon", "netscape");
		map->insert("Name", i18n("Web SideBar Plugin"));
		map->insert("Open", "true");
		map->insert("X-KDE-KonqSidebarModule","konqsidebar_web");
		fn->setLatin1("websidebarplugin%1.desktop");
		return true;
	}
}


#include "web_module.moc"

