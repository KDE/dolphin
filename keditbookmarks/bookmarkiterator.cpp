/* This file is part of the KDE project
   Copyright (C) 2000 David Faure <faure@kde.org>
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

#include <kdebug.h>

#include <qtimer.h>

#include "toplevel.h"
#include "listview.h"
#include "bookmarkiterator.h"

BookmarkIterator::BookmarkIterator(QValueList<KBookmark> bks) : m_bks(bks) {
   connect(this, SIGNAL( deleteSelf(BookmarkIterator *) ), 
                 SLOT( slotCancelTest(BookmarkIterator *) ));
   delayedEmitNextOne();
}

BookmarkIterator::~BookmarkIterator() {
}

void BookmarkIterator::delayedEmitNextOne() {
   QTimer::singleShot(1, this, SLOT( nextOne() ));
}

void BookmarkIterator::slotCancelTest(BookmarkIterator *test) {
   holder()->removeItr(test);
}

KEBListViewItem* BookmarkIterator::curItem() {
   return ListView::self()->getItemAtAddress(m_book.address());
}

void BookmarkIterator::nextOne() {
   // kdDebug() << "BookmarkIterator::nextOne" << endl;

   if (m_bks.count() == 0) {
      emit deleteSelf(this);
      return;
   }

   QValueListIterator<KBookmark> head = m_bks.begin();
   KBookmark bk = (*head);

   if (isBlahable(bk) && bk.address() != "ERROR") {
      m_url = bk.url().url();

      // kdDebug() << "BookmarkIterator::nextOne " << m_url << " : " << bk.address() << "\n";

      m_book = bk;

      doBlah();

      m_bks.remove(head);

   } else {
      m_bks.remove(head);
      emit nextOne();
   }
}

BookmarkIteratorHolder::BookmarkIteratorHolder() {
   m_itrs.setAutoDelete(true);
}

void BookmarkIteratorHolder::insertItr(BookmarkIterator *itr) {
   m_itrs.insert(0, itr);
   doItrListChanged();
}

void BookmarkIteratorHolder::removeItr(BookmarkIterator *itr) {
   m_itrs.remove(itr);
   doItrListChanged();
}

void BookmarkIteratorHolder::cancelAllItrs() {
   BookmarkIterator *itr;
   // DESIGN - use an iterator (it), umm... confusing :)
   for (itr = m_itrs.first(); itr != 0; itr = m_itrs.next()) {
      removeItr(itr);
   }
}

#include "bookmarkiterator.moc"
