/* This file is part of the KDE project
   Copyright (C) 2000 David Faure <faure@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License version 2 as published by the Free Software Foundation.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "bookmark_module.h"
#include "bookmark_item.h"
#include <kbookmarkmanager.h>

KonqBookmarkModule::KonqBookmarkModule( KonqTree * parentTree )
    : QObject( 0L ), KonqTreeModule( parentTree ),
      m_topLevelItem( 0L )
{
    connect( KBookmarkManager::self(), SIGNAL(changed(const QString &, const QString &) ),
             SLOT( slotBookmarksChanged(const QString &) ) );
}

KonqBookmarkModule::~KonqBookmarkModule()
{
}

void KonqBookmarkModule::addTopLevelItem( KonqTreeTopLevelItem * item )
{
    m_topLevelItem = item;
    fillListView();
}

void KonqBookmarkModule::slotBookmarksChanged( const QString & groupAddress )
{
    // update the right part of the tree
    KBookmarkGroup group = KBookmarkManager::self()->findByAddress( groupAddress ).toGroup();
    KonqBookmarkItem * item = findByAddress( groupAddress );
    ASSERT(!group.isNull());
    ASSERT(item);
    if (!group.isNull() && item)
    {
        // Delete all children of item
        QListViewItem * child = item->firstChild();
        while( child ) {
            QListViewItem * next = child->nextSibling();
            delete child;
            child = next;
        }
        fillGroup( item, group );
    }
}

void KonqBookmarkModule::fillListView()
{
    KBookmarkGroup root = KBookmarkManager::self()->root();
    fillGroup( m_topLevelItem, root );
}

void KonqBookmarkModule::fillGroup( KonqTreeItem * parentItem, KBookmarkGroup group )
{
    int n = 0;
    for ( KBookmark bk = group.first() ; !bk.isNull() ; bk = group.next(bk), ++n )
    {
        if ( !bk.isSeparator() ) // Separators don't look good in the tree IMHO
        {
            KonqBookmarkItem * item = new KonqBookmarkItem( parentItem, m_topLevelItem, bk, n );
            if ( bk.isGroup() )
            {
                KBookmarkGroup grp = bk.toGroup();
                fillGroup( item, grp );
                // Not sure we want to open those which are open in the editor :-)
                //if (grp.isOpen())
                //item->setOpen(true);
                if ( item->childCount() == 0 )
                    item->setExpandable( false );
            }
            else
                item->setExpandable( false );
        }
    }
}

// Borrowed from KEditBookmarks
KonqBookmarkItem * KonqBookmarkModule::findByAddress( const QString & address ) const
{
    QListViewItem * item = m_topLevelItem;
    // The address is something like /5/10/2
    QStringList addresses = QStringList::split('/',address);
    for ( QStringList::Iterator it = addresses.begin() ; it != addresses.end() ; ++it )
    {
        uint number = (*it).toUInt();
        item = item->firstChild();
        for ( uint i = 0 ; i < number ; ++i )
            item = item->nextSibling();
    }
    ASSERT(item);
    return static_cast<KonqBookmarkItem *>(item);
}

#include "bookmark_module.moc"
