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

#include <qregexp.h>
#include <qtimer.h>

#include <kdebug.h>
#include <klocale.h>

#include <kbookmarkmanager.h>

#include "toplevel.h"
#include "listview.h"
#include "search.h"
#include "bookmarkiterator.h"

SearchItrHolder *SearchItrHolder::s_self = 0;

SearchItrHolder::SearchItrHolder() 
   : BookmarkIteratorHolder() {
   ;
}

void SearchItrHolder::doItrListChanged() {
   KEBApp::self()->setCancelSearchEnabled(m_itrs.count() > 0);
}

void SearchItrHolder::slotFindNext() {
   KEBListViewItem *item = m_foundlist.next();
   if (!item) {
      return;
   }
   ListView::self()->clearSelection();
   ListView::self()->setCurrent(item);
   item->setSelected(true);
}

void SearchItrHolder::addFind(KEBListViewItem *item) {
   bool wasEmpty = m_foundlist.isEmpty();
   if (wasEmpty) {
      ListView::self()->clearSelection();
      ListView::self()->setCurrent(item);
      item->setSelected(true);
   };
   m_foundlist.append(item);
   m_foundlist.first();
}

/* -------------------------------------- */

SearchItr::SearchItr(QValueList<KBookmark> bks)
   : BookmarkIterator(bks), m_showstatuscounter(0), m_statusitem(0)  {
   ListView::self()->clearSelection();
}

SearchItr::~SearchItr() {
   if (m_statusitem) {
      m_statusitem->restoreStatus();
   }
}

#include <kfind.h>
#include <kfinddialog.h>

void SearchItr::setSearch(int /*options*/, const QString& pattern) {
   m_text = pattern;
   /*
   m_find = new KFind( pattern, options, this );

   connect(m_find, SIGNAL( highlight(const QString &, int, int) ),
           this,   SLOT( searchHighlight( const QString &, int, int) ));
   connect(m_find, SIGNAL( findNext() ), 
           this, SLOT( slotFindNext() ) );

   m_findItem = (BugLVI *)m_listBugs->firstChild();

   if (options & KFindDialog::FromCursor 
    && m_listBugs->currentItem() ) {
      m_findItem = (BugLVI *)m_listBugs->currentItem();
   }

   slotFindNext();
   */
}

bool SearchItr::isApplicable(const KBookmark &bk) {
   return (!bk.isSeparator());
}

void SearchItr::doAction() {
   // kdDebug() << "doAction()" << m_statusitem << endl;

   KEBListViewItem* new_statusitem = 0;
   KEBListViewItem* openparent = ListView::self()->findOpenParent(curItem());
   if (openparent != curItem()->parent()) {
      new_statusitem = openparent;
   } else if ((m_showstatuscounter++ % 4) == 0) {
      new_statusitem = curItem();
   }

   if (new_statusitem && new_statusitem != m_statusitem) {
      if (m_statusitem) {
         m_statusitem->restoreStatus();
      }
      m_statusitem = new_statusitem;
      m_statusitem->setTmpStatus(i18n("Searching..."));
   }

   QString text = m_book.url().url() + m_book.fullText();
   if (text.find(m_text, 0, FALSE) != -1) {
      // kdDebug() << m_book.url().url() << endl;
      SearchItrHolder::self()->addFind(curItem());
      // curItem()->setSelected(true); // only if no current selection?
      ListView::self()->openParents(curItem());
      // ListView::self()->updateListView();
      // set paintstyle also, thats most important, but will require 
      // refactoring stuff from items/testlink into itr base
   }

   // statusbar stuff???

   delayedEmitNextOne();
}

#include "search.moc"
