/* This file is part of the KDE project
   Copyright (C) 2003 Alexander Kellett <lypanov@kde.org>

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

#ifndef __search_h
#define __search_h

#include <qobject.h>
#include <kbookmark.h>

class KBookmarkTextMap;

class Searcher : public QObject {
   Q_OBJECT
protected slots:
   void slotSearchTextChanged(const QString & text);
public:
   static Searcher* self() { 
      if (!s_self) { s_self = new Searcher(); }; return s_self; 
   }
private:
   Searcher() { m_bktextmap = 0; }
   static Searcher *s_self;
   KBookmark m_last_search_result;
   KBookmarkTextMap *m_bktextmap;
};

#endif
