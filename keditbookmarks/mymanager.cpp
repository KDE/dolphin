/* This file is part of the KDE project
   Copyright (C) 2002-2003 Alexander Kellett <lypanov@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License version 2 as published by the Free Software Foundation.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include <stdlib.h>

#include <klocale.h>
#include <kdebug.h>
#include <kapplication.h>
#include <dcopref.h>

#include <kbookmarkmanager.h>
#include <kbookmarkimporter.h>

#include "kebbookmarkexporter.h"
#include "toplevel.h"

#include "mymanager.h"

CurrentMgr *MyManager::s_mgr = 0;

void CurrentMgr::createManager(QObject *top, QString filename) {
   if (m_mgr) {
      QObject::disconnect(m_mgr, 0, 0, 0);
   }

   // TODO - delete m_mgr, and do testing

   m_mgr = KBookmarkManager::managerForFile(filename, false);

   // if someone plays with konq's bookmarks while we're open, update. (when applicable)
   QObject::connect(m_mgr, SIGNAL( changed(const QString &, const QString &) ),
                    top,   SLOT( slotBookmarksChanged(const QString &, const QString &) ));
}

void CurrentMgr::notifyManagers() {
   QCString objId("KBookmarkManager-");
   objId += mgr()->path().utf8();
   DCOPRef("*", objId).send("notifyCompleteChange", QString::fromLatin1(kapp->name()));
}

void CurrentMgr::doExport(bool moz) {
   QString path = 
          (moz)
        ? KNSBookmarkImporter::mozillaBookmarksFile(true)
        : KNSBookmarkImporter::netscapeBookmarksFile(true);
   if (!path.isEmpty()) {
      KEBNSBookmarkExporter exporter(mgr(), path);
      exporter.write(moz);
   }
}

// TODO - less than simplistic, but try to remove anyways

QString CurrentMgr::correctAddress(const QString &address) {
   return mgr()->findByAddress(address, true).address();
}

