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

#include "mozilla_module.h"
#include <qfileinfo.h>

#include <khtml_part.h>
#include <klocale.h>
#include <kdebug.h>
#include <kstddirs.h>
#include <kglobal.h>


KonqSidebarMozillaModule::KonqSidebarMozillaModule(KInstance *instance, QObject *parent, QWidget *widgetParent, QString &desktopName, const char* name)
	: KonqSidebarPlugin(instance, parent, widgetParent, desktopName, name)
{
	_htmlPart = new KHTMLPart;
//	connect(_htmlPart, SIGNAL(setWindowCaption(const QString&)), this, SLOT());
//	connect(_htmlPart, SIGNAL(completed()), this, SLOT());
	KSimpleConfig ksc(desktopName);
	ksc.setGroup("Desktop Entry");

	_htmlPart->openURL(ksc.readEntry("URL"));
}


KonqSidebarMozillaModule::~KonqSidebarMozillaModule() {
	delete _htmlPart;
	_htmlPart = 0L;
}


QWidget *KonqSidebarMozillaModule::getWidget() {
	return _htmlPart->widget();
}


void *KonqSidebarMozillaModule::provides(const QString &) {
	return 0L;
}


void KonqSidebarMozillaModule::handleURL(const KURL &) {
}


extern "C" {
	KonqSidebarPlugin* create_konqsidebar_mozilla(KInstance *instance, QObject *parent, QWidget *widget, QString &desktopName, const char *name) {
		return new KonqSidebarMozillaModule(instance, parent, widget, desktopName, name);
	} 
}


extern "C" {
	bool add_konqsidebar_mozilla(QString* fn, QString* param, QMap<QString,QString> *map) {
		KGlobal::dirs()->addResourceType("mozillasidebardata", KStandardDirs::kde_default("data") + "konqsidebartng/mozillasidebar");
		KURL url;
		url.setProtocol("file");
		QStringList paths = KGlobal::dirs()->resourceDirs("mozillasidebardata");
		for (QStringList::Iterator i = paths.begin(); i != paths.end(); ++i) {
			if (QFileInfo(*i + "mozillasidebar.html").exists()) {
				url.setPath(*i + "mozillasidebar.html");
				break;
			}
		}

		if (url.path().isEmpty())
			return false;
		map->insert("Type", "Link");
		map->insert("URL", url.url());
		map->insert("Icon", "netscape");
		map->insert("Name", i18n("Mozilla Sidebar Plugin"));
		map->insert("Open", "true");
		map->insert("X-KDE-KonqSidebarModule","konqsidebar_mozilla");
		fn->setLatin1("mozillaplugin%1.desktop");
		return true;
	}
}


#include "mozilla_module.moc"

