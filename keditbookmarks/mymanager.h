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

#ifndef __mymanager_h
#define __mymanager_h

#include <kbookmarkmanager.h>

class QObject;
class QString;

class MyManager {
   friend class BkManagerAccessor;
public:
   static MyManager* self() { if (!s_mgr) { s_mgr = new MyManager(); } return s_mgr; }
   void createManager(QObject *top, QString filename);
   void doExport(bool moz);
   void notifyManagers();
   bool managerSave() { return mgr()->save(); }
   void saveAs(const QString &fileName) { mgr()->saveAs(fileName); }
   void setUpdate(bool update) { mgr()->setUpdate(update); }
   QString path() { return mgr()->path(); }
   bool showNSBookmarks() { return mgr()->showNSBookmarks(); }
   QString correctAddress(const QString &address);
protected:
   KBookmarkManager* mgr() const { return m_mgr; }
private:
   MyManager() {
      m_mgr = 0;
   };
   KBookmarkManager *m_mgr;
   static MyManager *s_mgr;
};

class BkManagerAccessor {
public:
   static KBookmarkManager* mgr() { return MyManager::self()->mgr(); }
};

#endif

