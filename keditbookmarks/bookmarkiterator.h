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

#ifndef __bookmarkiterator_h
#define __bookmarkiterator_h

#include <qobject.h>
#include <kbookmark.h>

class KEBListViewItem;
class BookmarkIteratorHolder;

class BookmarkIterator : public QObject
{
   Q_OBJECT

public:
   BookmarkIterator(QValueList<KBookmark> bks);
   virtual ~BookmarkIterator();
   virtual BookmarkIteratorHolder* holder() = 0;

public slots:
   void nextOne();
   void delayedEmitNextOne();
   void slotCancelTest(BookmarkIterator *t);

signals:
   void deleteSelf(BookmarkIterator *);

protected:
   virtual void doAction() = 0;
   virtual bool isApplicable(const KBookmark &bk) = 0;

   KEBListViewItem* curItem();

   KBookmark m_book;
   KEBListViewItem *m_cur_item;
   QValueList<KBookmark> m_bks;
   QString m_url;
};

class BookmarkIteratorHolder
{
public:
   void cancelAllItrs();
   void removeItr(BookmarkIterator*);
   void insertItr(BookmarkIterator*);
protected:
   BookmarkIteratorHolder();
   virtual ~BookmarkIteratorHolder() {};
   virtual void doItrListChanged() = 0;
   QPtrList<BookmarkIterator> m_itrs;
};

#endif
