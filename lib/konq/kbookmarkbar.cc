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
#include <kbookmarkmenu.h>

#include <ktoolbar.h>
#include <ktoolbarbutton.h>
#include <kmessagebox.h>

#include <kglobal.h>
#include <klocale.h>
#include <kiconloader.h>
#include <kurl.h>
#include <kpopupmenu.h>

#include <qiconset.h>

KBookmarkBar::KBookmarkBar( KBookmarkOwner *_owner, KToolBar *_toolBar,
                            KActionCollection *_collec, QObject *parent, const char *name )
    : QObject( parent, name ), m_pOwner(_owner), m_toolBar(_toolBar), m_actionCollection(_collec)
{
    // force the "icon to the left of the text" look
    m_toolBar->setIconText(KToolBar::IconTextRight);

    m_lstSubMenus.setAutoDelete( true );

    connect(KBookmarkManager::self(), SIGNAL(changed()),
            this,                     SLOT(slotBookmarksChanged()));
    slotBookmarksChanged();
}

KBookmarkBar::~KBookmarkBar()
{
    clear();
}

void KBookmarkBar::clear()
{
    m_lstSubMenus.clear();

    QListIterator<KAction> it( m_actions );
    for (; it.current(); ++it )
    {
        it.current()->unplugAll();
        m_actionCollection->take( it.current() );
    }

    m_actions.setAutoDelete( true );
    m_actions.clear();
    m_actions.setAutoDelete( false );

    // remove that strange separator thing..
    if (m_toolBar)
        m_toolBar->clear();
}

void KBookmarkBar::slotBookmarksChanged()
{
    clear();

    // ### What's this?? Is that still really needed with QToolBar? (Simon)
    { // KToolBar needs a setMinimumSize method similar to this
    KToolBarButton *separ = new KToolBarButton(m_toolBar);

    m_toolBar->insertWidget(-1, 1, separ);
    separ->resize(1, 26);
    separ->hide();
    }

    fillBookmarkBar(KBookmarkManager::self()->toolbar());
}

void KBookmarkBar::fillBookmarkBar(KBookmark *parent)
{
    if (!parent)
        return;

    KBookmark *bm;
    for (bm = parent->first(); bm; bm = parent->next())
    {
        QString pix = bm->pixmapFile();

        if (bm->type() == KBookmark::URL)
        {
            KAction *action;
            // create a normal URL item, with ID as a name
            action = new KAction(bm->text(), pix, 0,
                                 this, SLOT(slotBookmarkSelected()),
                                 m_actionCollection,
                                 QCString().sprintf("bookmark%d", bm->id()));
            action->plug(m_toolBar);
            m_actions.append( action );
        }
        else
        {
            KActionMenu *action;
            action = new KActionMenu(bm->text(), pix, this);

            KBookmarkMenu *menu;
            menu = new KBookmarkMenu(m_pOwner, action->popupMenu(),
                                     m_actionCollection, false, false);
            menu->fillBookmarkMenu( bm );
            action->plug(m_toolBar);
            m_actions.append( action );
            m_lstSubMenus.append( menu );
        }
    }
}

void KBookmarkBar::slotBookmarkSelected()
{
    if (!m_pOwner) return; // this view doesn't handle bookmarks...

    int id = QCString(sender()->name() + 8).toInt(); // skip "bookmark"
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
