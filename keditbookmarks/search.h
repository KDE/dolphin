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

#include "bookmarkiterator.h"
#include "listview.h"

class SearchItrHolder : public BookmarkIteratorHolder {
public:
   static SearchItrHolder* self() { 
      if (!s_self) { s_self = new SearchItrHolder(); }; return s_self; 
   }
   void addFind(KEBListViewItem *item);
   void slotFindNext();
protected:
   virtual void doItrListChanged();
private:
   SearchItrHolder();
   static SearchItrHolder *s_self;
   QPtrList<KEBListViewItem> m_foundlist;
};

class SearchItr : public BookmarkIterator
{
   Q_OBJECT

public:
   SearchItr(QValueList<KBookmark> bks);
   ~SearchItr();
   
   virtual BookmarkIteratorHolder* holder() { return SearchItrHolder::self(); }
   void setSearch(int options, const QString& pattern);

private:
   virtual void doAction();
   virtual bool isApplicable(const KBookmark &bk);

   int m_showstatuscounter;
   KEBListViewItem *m_statusitem;
   QString m_text;
};

#endif
