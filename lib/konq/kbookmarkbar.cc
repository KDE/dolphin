/* This file is part of the KDE project
   Copyright (C) 1999 Kurt Granroth <granroth@kde.org>
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/
#include <kbookmarkbar.h>

#include <kaction.h>
#include <kbookmark.h>

#include <ktoolbar.h>
#include <ktoolbarbutton.h>
#include <kmessagebox.h>

#include <kglobal.h>
#include <klocale.h>
#include <kiconloader.h>
#include <kurl.h>

KBookmarkBar::KBookmarkBar( KBookmarkOwner *_owner, KToolBar *_toolBar,
                            QActionCollection *_collec )
    : m_pOwner(_owner), m_toolBar(_toolBar), m_actionCollection(_collec)
{
    // force the "icon to the left of the text" look
    m_toolBar->setIconText(1);

    connect(KBookmarkManager::self(), SIGNAL(changed()),
            this,                     SLOT(slotBookmarksChanged()));
    slotBookmarksChanged();
}

KBookmarkBar::~KBookmarkBar()
{
}

void KBookmarkBar::slotBookmarksChanged()
{
    m_toolBar->clear();

    { // KToolBar needs a setMinimumSize method similar to this
    KToolBarButton *separ = new KToolBarButton(m_toolBar);

    m_toolBar->insertWidget(-1, 1, separ);
    separ->resize(1, 26);
    separ->hide();
    }

    KGlobal::iconLoader()->setIconType("icon");
    fillBookmarkBar(KBookmarkManager::self()->toolbar());
    KGlobal::iconLoader()->setIconType("toolbar"); // restore default
}

void KBookmarkBar::fillBookmarkBar(KBookmark *parent)
{
    if (!parent)
        return;

    KBookmark *bm;
    for (bm = parent->children()->first(); bm; bm = parent->children()->next())
    {
        if (bm->type() == KBookmark::URL)
        {
            // create a normal URL item, with ID as a name
            QPixmap pix(KGlobal::iconLoader()->loadApplicationIcon(
                            bm->pixmapFile(), KIconLoader::Small));

            KAction *action;
            action = new KAction(bm->text(), QIconSet(pix), 0,
                                 this, SLOT(slotBookmarkSelected()),
                                 m_actionCollection,
                                 QString("bookmark%1").arg(bm->id()));
            action->plug(m_toolBar);
        }
        // don't handle folders quite yet.. probably the best way
        // would be to have a pull down button
    }
}

void KBookmarkBar::slotBookmarkSelected()
{
    if (!m_pOwner) return; // this view doesn't handle bookmarks...

    int id = QString(sender()->name() + 8).toInt(); // skip "bookmark"
    KBookmark *bm = KBookmarkManager::self()->findBookmark(id);

    if (bm)
    {
        KURL u(bm->url());
        if (u.isMalformed())
        {
            QString tmp = i18n("Malformed URL\n%1").arg(bm->url());
            KMessageBox::error(0L, tmp);
            return;
        }

        m_pOwner->openBookmarkURL(bm->url());
    }
}

#include "kbookmarkbar.moc"
