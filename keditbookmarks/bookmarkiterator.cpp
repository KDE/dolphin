// -*- indent-tabs-mode:nil -*-
// vim: set ts=4 sts=4 sw=4 et:
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
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "bookmarkiterator.h"

#include "toplevel.h"
#include "listview.h"

#include <kdebug.h>
#include <qtimer.h>
#include <assert.h>

BookmarkIterator::BookmarkIterator(QVector<KBookmark> bks) : m_bklist(bks) {
    connect(this, SIGNAL( deleteSelf(BookmarkIterator *) ), 
            SLOT( slotCancelTest(BookmarkIterator *) ));
    delayedEmitNextOne();
}

BookmarkIterator::~BookmarkIterator() {
    ;
}

void BookmarkIterator::delayedEmitNextOne() {
    QTimer::singleShot(1, this, SLOT( nextOne() ));
}

void BookmarkIterator::slotCancelTest(BookmarkIterator *test) {
    holder()->removeItr(test);
}

const KBookmark BookmarkIterator::curBk() const {
    assert(m_bk.hasParent());
    return m_bk;
}

void BookmarkIterator::nextOne() {
    // kdDebug() << "BookmarkIterator::nextOne" << endl;

    if (m_bklist.isEmpty()) {
        emit deleteSelf(this);
        return;
    }

    QVector<KBookmark>::iterator head = m_bklist.begin(); //FIXME using vector is bad here
    KBookmark bk = (*head);

    bool viable = bk.hasParent() && isApplicable(bk);

    if (viable) {
        m_bk = bk;
        doAction();
    }

    m_bklist.erase(head);

    if (!viable)
        delayedEmitNextOne();
}

/* --------------------------- */

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
    m_itrs.clear();
    doItrListChanged();
}

#include "bookmarkiterator.moc"
