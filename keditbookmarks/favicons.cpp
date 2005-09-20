// -*- indent-tabs-mode:nil -*-
// vim: set ts=4 sts=4 sw=4 et:
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
   the Free Software Foundation, Inc., 51 Franklin Steet, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "favicons.h"

#include "bookmarkiterator.h"
#include "toplevel.h"
#include "updater.h"

#include <kdebug.h>
#include <klocale.h>
#include <kapplication.h>

FavIconsItrHolder *FavIconsItrHolder::s_self = 0;

FavIconsItrHolder::FavIconsItrHolder() 
    : BookmarkIteratorHolder() {
    // do stuff
}

void FavIconsItrHolder::doItrListChanged() {
    kdDebug()<<"FavIconsItrHolder::doItrListChanged() "<<count()<<" iterators"<<endl;
    KEBApp::self()->setCancelFavIconUpdatesEnabled(count() > 0);
    if(count() == 0)
    {
        kdDebug()<<"Notifing managers "<<m_affectedBookmark<<endl;
        CurrentMgr::self()->notifyManagers(CurrentMgr::bookmarkAt(m_affectedBookmark).toGroup());
        m_affectedBookmark = QString::null;
    }
}

void FavIconsItrHolder::addAffectedBookmark( const QString & address )
{
    kdDebug()<<"addAffectedBookmark "<<address<<endl;
    if(m_affectedBookmark.isNull())
        m_affectedBookmark = address;
    else
        m_affectedBookmark = KBookmark::commonParent(m_affectedBookmark, address);
    kdDebug()<<" m_affectedBookmark is now "<<m_affectedBookmark<<endl;
}

/* -------------------------- */

FavIconsItr::FavIconsItr(QVector<KBookmark> bks)
    : BookmarkIterator(bks) {
    m_updater = 0;
}

FavIconsItr::~FavIconsItr() {
//FIXME    if (curItem())
//        curItem()->restoreStatus();
    delete m_updater;
}

void FavIconsItr::slotDone(bool succeeded) {
    // kdDebug() << "FavIconsItr::slotDone()" << endl;
    //FIXME curItem()->setTmpStatus(succeeded ? i18n("OK") : i18n("No favicon found"));
    Q_UNUSED(succeeded);
    holder()->addAffectedBookmark(KBookmark::parentAddress(curBk().address()));
    delayedEmitNextOne();
}

bool FavIconsItr::isApplicable(const KBookmark &bk) const {
    return (!bk.isGroup() && !bk.isSeparator());
}

void FavIconsItr::doAction() {
    // kdDebug() << "FavIconsItr::doAction()" << endl;
    //FIXME curItem()->setTmpStatus(i18n("Updating favicon..."));
    if (!m_updater) {
        m_updater = new FavIconUpdater(kapp, "FavIconUpdater");
        connect(m_updater, SIGNAL( done(bool) ),
                this,      SLOT( slotDone(bool) ) );
    }
    if (curBk().url().protocol().startsWith("http")) {
        m_updater->downloadIcon(curBk());
    } else {
        //FIXME curItem()->setTmpStatus(i18n("Local file"));
        delayedEmitNextOne();
    }
}

#include "favicons.moc"
