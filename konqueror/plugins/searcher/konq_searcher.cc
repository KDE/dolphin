/*  This file is part of the KDE project
    Copyright (C) 1999 Simon Hausmann <hausmann@kde.org>
    Copyright (C) 2000 Yves Arrouye <yves@realnames.com>

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

#include <qregexp.h>
#include <qxembed.h>
#include <qapplication.h>

#include <kurl.h>
#include <kprotocolmanager.h>
#include <kinstance.h>
#include <kglobal.h>
#include <klocale.h>

#include <kurifilter.h>

#include <konq_events.h>

#include "konq_searcher.h"

KInstance *KonqSearcherFactory::s_instance = 0L;

KonqSearcher::KonqSearcher(QObject *parent) : QObject(parent, "KonqSearcher") {
    parent->installEventFilter(this);
}

KonqSearcher::~KonqSearcher() {
}

bool KonqSearcher::eventFilter(QObject *, QEvent *ev) {
    if (KonqURLEnterEvent::test(ev)) {
	QString url = ((KonqURLEnterEvent *)ev)->url();

	if (KURIFilter::self()->filterURI(url)) {
	    KonqURLEnterEvent e(url);
	    QApplication::sendEvent(parent(), &e);
	
	    return true;
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
  QObject *obj = new KonqSearcher( parent );
  emit objectCreated( obj );
  return obj;
}

KInstance *KonqSearcherFactory::instance() {
    return s_instance;
}

extern "C" {

void *init_libkonqsearcher() {
    return new KonqSearcherFactory;
}

}

#include "konq_searcher.moc"

