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
#include <qtimer.h>

#include <klocale.h>
#include <kdebug.h>
#include <kstandarddirs.h>
#include <kglobal.h>
#include <konq_pixmapprovider.h>


KonqSideBarWebModule::KonqSideBarWebModule(KInstance *instance, QObject *parent, QWidget *widgetParent, QString &desktopName, const char* name)
	: KonqSidebarPlugin(instance, parent, widgetParent, desktopName, name)
{
	_htmlPart = new KHTMLSideBar;
	connect(_htmlPart, SIGNAL(reload()), this, SLOT(reload()));
	connect(_htmlPart, SIGNAL(completed()), this, SLOT(pageLoaded()));
	connect(_htmlPart,
		SIGNAL(setWindowCaption(const QString&)),
		this,
		SLOT(setTitle(const QString&)));
	connect(_htmlPart,
		SIGNAL(openURLRequest(const QString&, KParts::URLArgs)),
		this,
		SLOT(urlClicked(const QString&, KParts::URLArgs)));
	connect(_htmlPart,
		SIGNAL(openURLNewWindow(const QString&, KParts::URLArgs)),
		this,
		SLOT(urlNewWindow(const QString&, KParts::URLArgs)));
	connect(_htmlPart,
		SIGNAL(submitFormRequest(const char*,const QString&,const QByteArray&,const QString&,const QString&,const QString&)),
		this,
		SIGNAL(submitFormRequest(const char*,const QString&,const QByteArray&,const QString&,const QString&,const QString&)));

	_desktopName = desktopName;

	KSimpleConfig ksc(_desktopName);
	ksc.setGroup("Desktop Entry");

	_url = ksc.readPathEntry("URL");
	_htmlPart->openURL(_url );
	// Must load this delayed
	QTimer::singleShot(0, this, SLOT(loadFavicon()));
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


void KonqSideBarWebModule::urlNewWindow(const QString& url, KParts::URLArgs args)
{
	emit createNewWindow(KURL( url ), args);
}


void KonqSideBarWebModule::urlClicked(const QString& url, KParts::URLArgs args) 
{
	emit openURLRequest(KURL( url ), args);
}


void KonqSideBarWebModule::loadFavicon() {
	QString icon = KonqPixmapProvider::iconForURL(_url.url());
	if (icon.isEmpty()) {
		KonqFavIconMgr::downloadHostIcon(_url);
		icon = KonqPixmapProvider::iconForURL(_url.url());
	}

	if (!icon.isEmpty()) {
		emit setIcon(icon);

		KSimpleConfig ksc(_desktopName);
		ksc.setGroup("Desktop Entry");
		if (icon != ksc.readPathEntry("Icon")) {
			ksc.writePathEntry("Icon", icon);
		}
	}
}


void KonqSideBarWebModule::reload() {
	_htmlPart->openURL(_url);
}


void KonqSideBarWebModule::setTitle(const QString& title) {
	if (!title.isEmpty()) {
		emit setCaption(title);

		KSimpleConfig ksc(_desktopName);
		ksc.setGroup("Desktop Entry");
		if (title != ksc.readPathEntry("Name")) {
			ksc.writePathEntry("Name", title);
		}
	}
}


void KonqSideBarWebModule::pageLoaded() {
}


extern "C" {
	KonqSidebarPlugin* create_konqsidebar_web(KInstance *instance, QObject *parent, QWidget *widget, QString &desktopName, const char *name) {
		return new KonqSideBarWebModule(instance, parent, widget, desktopName, name);
	}
}


extern "C" {
	bool add_konqsidebar_web(QString* fn, QString* param, QMap<QString,QString> *map) {
		Q_UNUSED(param);
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

