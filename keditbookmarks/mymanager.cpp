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

#include <qclipboard.h>
#include <qpopupmenu.h>
#include <qpainter.h>

#include <klocale.h>
#include <dcopref.h>
#include <kdebug.h>
#include <kapplication.h>

#include <kaction.h>
#include <kstdaction.h>
#include <kedittoolbar.h>
#include <kfiledialog.h>
#include <kkeydialog.h>
#include <kmessagebox.h>
#include <krun.h>

#include <kicondialog.h>
#include <kiconloader.h>

#include <kbookmarkdrag.h>
#include <kbookmarkmanager.h>
#include <kbookmarkexporter.h>
#include <kbookmarkimporter.h>
#include <kbookmarkimporter_ie.h>
#include <kbookmarkimporter_opera.h>

#include <klineeditdlg.h>

#include "toplevel.h"
#include "commands.h"
#include "importers.h"
#include "core.h"
#include "favicons.h"
#include "testlink.h"
#include "listview.h"
#include "mymanager.h"

MyManager *MyManager::s_mgr = 0;

void MyManager::createManager(KEBTopLevel *top, QString filename) {
   if (m_mgr) {
      QObject::disconnect(m_mgr, 0, 0, 0);
   }

   // TODO - delete m_mgr, and do testing

   m_mgr = KBookmarkManager::managerForFile(filename, false);

   // if someone plays with konq's bookmarks while we're open, update. (when applicable)
   QObject::connect(m_mgr, SIGNAL( changed(const QString &, const QString &) ),
                    top,   SLOT( slotBookmarksChanged(const QString &, const QString &) ));
}

void MyManager::notifyManagers() {
   QCString objId("KBookmarkManager-");
   objId += mgr()->path().utf8();
   DCOPRef("*", objId).send("notifyCompleteChange", QString::fromLatin1(kapp->name()));
}

void MyManager::doExport(QString path, bool moz) {
   if (!path.isEmpty()) {
      KNSBookmarkExporter exporter(mgr(), path);
      exporter.write(moz);
   }
}

// TODO - less than simplistic, but try to remove anyways

void MyManager::flipShowNSFlag() {
   mgr()->setShowNSBookmarks(!mgr()->showNSBookmarks());
}

QString MyManager::correctAddress(QString address) {
   return mgr()->findByAddress(address, true).address();
}

// TODO - simplistic forwards, remove or inline in .h

void MyManager::saveAs(QString fileName) {
   mgr()->saveAs(fileName);
}

bool MyManager::managerSave() {
   return mgr()->save();
}

bool MyManager::showNSBookmarks() {
   return mgr()->showNSBookmarks();
}

QString MyManager::path() {
   return mgr()->path();
}

void MyManager::setUpdate(bool update) {
   mgr()->setUpdate(update); 
}

