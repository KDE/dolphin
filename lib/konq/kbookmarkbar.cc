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
#include <kbookmarkmanager.h>
#include <kbookmarkmenu.h>

#include <ktoolbar.h>
#include <ktoolbarbutton.h>
#include <kmessagebox.h>

#include <kglobal.h>
#include <kconfig.h>
#include <klocale.h>
#include <kiconloader.h>
#include <kurl.h>
#include <kpopupmenu.h>

#include <qiconset.h>

KBookmarkBar::KBookmarkBar( KBookmarkOwner *_owner, KToolBar *_toolBar,
                            KActionCollection *,
                            QObject *parent, const char *name )
    : QObject( parent, name ), m_pOwner(_owner), m_toolBar(_toolBar),
      m_actionCollection( 0L /* see KBookmarkMenu */ )
{
    m_lstSubMenus.setAutoDelete( true );

    connect( KBookmarkManager::self(), SIGNAL( changed(const QString &, const QString &) ),
             SLOT( slotBookmarksChanged(const QString &) ) );

    KBookmarkGroup toolbar = KBookmarkManager::self()->toolbar();
    fillBookmarkBar( toolbar );
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
        //m_actionCollection->take( it.current() );
    }

    m_actions.setAutoDelete( true );
    m_actions.clear();
    m_actions.setAutoDelete( false );

    if ( m_toolBar )
        m_toolBar->clear();
}

void KBookmarkBar::slotBookmarksChanged( const QString & group )
{
    KBookmarkGroup tb = KBookmarkManager::self()->toolbar();
    if ( tb.isNull() )
        return;
    if ( tb.address() == group )
    {
        clear();

        fillBookmarkBar( tb );
    } else
    {
        // Iterate recursively into child menus
        QListIterator<KBookmarkMenu> it( m_lstSubMenus );
        for (; it.current(); ++it )
        {
            it.current()->slotBookmarksChanged( group );
        }
    }

}

void KBookmarkBar::fillBookmarkBar(KBookmarkGroup & parent)
{
    if (parent.isNull())
        return;

    for (KBookmark bm = parent.first(); !bm.isNull(); bm = parent.next(bm))
    {
        if (!bm.isGroup())
        {
            if ( bm.isSeparator() )
                m_toolBar->insertLineSeparator();
            else
            {
                KAction *action;
                // create a normal URL item, with ID as a name
                action = new KAction(bm.text(), bm.icon(), 0,
                                     this, SLOT(slotBookmarkSelected()),
                                     m_actionCollection,
                                     bm.url().url().utf8());
                action->plug(m_toolBar);
                m_actions.append( action );
            }
        }
        else
        {
            KActionMenu *action = new KActionMenu(bm.text(), bm.icon(),
                                                  m_actionCollection, "bookmarkbar-actionmenu");
            action->setDelayed(false);

            // this flag doesn't have any UI yet
            KGlobal::config()->setGroup( "Settings" );
            bool addEntriesBookmarkBar = KGlobal::config()->readBoolEntry("AddEntriesBookmarkBar",true);

            KBookmarkMenu *menu = new KBookmarkMenu(m_pOwner, action->popupMenu(),
                                                    m_actionCollection, false, addEntriesBookmarkBar,
                                                    bm.address());
            //menu->fillBookmarkMenu();
            action->plug(m_toolBar);
            m_actions.append( action );
            m_lstSubMenus.append( menu );
        }
    }
}

void KBookmarkBar::slotBookmarkSelected()
{
    if (!m_pOwner) return; // this view doesn't handle bookmarks...

    m_pOwner->openBookmarkURL(QString::fromUtf8(sender()->name()));
}

#include "kbookmarkbar.moc"
