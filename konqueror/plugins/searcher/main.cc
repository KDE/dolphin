/*  This file is part of the KDE project
    Copyright (C) 1999 Simon Hausmann <hausmann@kde.org>
    Internet Keywords support (C) 1999 Yves Arrouye <yves@realnames.com>
    Current maintainer Yves Arrouye <yves@realnames.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#include "main.h"
#include "configwidget.h"
#include "enginecfg.h"

#include <konq_events.h>

#include <qregexp.h>
#include <qxembed.h>
#include <qapplication.h>

#include <kurl.h>
#include <kprotocolmanager.h>
#include <kinstance.h>
#include <kglobal.h>
#include <klocale.h>

#include <iostream.h>

KInstance *KonqSearcherFactory::s_instance = 0L;

KonqSearcher::KonqSearcher(QObject *parent) : QObject(parent, "KonqSearcher") {
    parent->installEventFilter(this);
}

KonqSearcher::~KonqSearcher() {
}

bool KonqSearcher::eventFilter(QObject *, QEvent *ev) {
    if (KonqURLEnterEvent::test(ev)) {
	QString url = ((KonqURLEnterEvent *)ev)->url();

	if (EngineCfg::self()->verbose()) {
	    cerr << "filtering " << url.ascii();
	}

	KURL kurl(url);

	// Is this URL a andidate for filtering?

	if (kurl.isMalformed() || !KProtocolManager::self().protocols().contains(kurl.protocol())) {
	    QString query = QString::null;

	    // See if it's a searcher prefix. If not, use Internet Keywords
	    // if we can. Note that we want a colon to match a searcher
	    // prefix.

	    int pos = url.find(':');
	    if (pos >= 0) {
	        QString key = url.left(pos);
	        query = EngineCfg::self()->searchQuery(key);
	    }
	    if (query == QString::null) {
		query = EngineCfg::self()->navQuery();
	    }

	    // Substitute the variable part in the query we found.

	    if (query != QString::null) {
		QString newurl = query;

		int pct;

		if ((pct = newurl.find("\\2")) >= 0) {
		    QString charsetname = KGlobal::locale()->charset();
		    if (charsetname == "us-ascii") {
			charsetname = "iso-8859-1";
		    }
		    KURL::encode(charsetname);
		    newurl = newurl.replace(pct, 2, charsetname);
		}

		QString userquery = url.mid(pos+1).replace(QRegExp(" "), "+");
		KURL::encode(userquery);
		if ((pct = newurl.find("\\1")) >= 0) {
		    newurl = newurl.replace(pct, 2, userquery);
		}

		if (EngineCfg::self()->verbose()) {
		    cerr << " to " << newurl.ascii() << endl;
		}

		KonqURLEnterEvent e(newurl);
		QApplication::sendEvent(parent(), &e);
	
		return true;
	    }
	}

	if (EngineCfg::self()->verbose()) {
	    cerr << endl;
	}
    }

    return false;
}

KonqSearcherFactory::KonqSearcherFactory(QObject *parent, const char *name) : KLibFactory(parent, name) {
    s_instance = new KInstance("konq_searcher");
}

KonqSearcherFactory::~KonqSearcherFactory() {
    delete s_instance;
}

QObject *KonqSearcherFactory::create( QObject *parent, const char *, const char*, const QStringList & )
{
  return new KonqSearcher( parent );
}

KInstance *KonqSearcherFactory::instance() {
    return s_instance;
}

extern "C" {

void *init_libkonqsearcher() {
    return new KonqSearcherFactory;
}

}

/*
int main(int argc, char **argv)
{
  KOMApplication app(argc, argv, "konq_searcher");

  KOMAutoLoader<KonqSearcherFactory> pluginLoader("IDL:KOM/PluginFactory:1.0", "KonqSearcher");

  ConfigWidget *w = new ConfigWidget;

  if (!QXEmbed::processClientCmdline(w, argc, argv))
    delete w;

  app.exec();
  return 0;
}
*/
#include "main.moc"
