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

#include <kparts/browserextension.h>

#include "konq_sidebartreeitem.h"
#include "konq_sidebartree.h"
//#include "konq_sidebartreepart.h"
#include "konq_sidebartreetoplevelitem.h"

KonqSidebarTreeItem::KonqSidebarTreeItem( KonqSidebarTreeItem *parentItem, KonqSidebarTreeTopLevelItem *topLevelItem )
    : QListViewItem( parentItem )
{
    initItem( topLevelItem );
}

KonqSidebarTreeItem::KonqSidebarTreeItem( KonqSidebarTree *parent, KonqSidebarTreeTopLevelItem *topLevelItem )
    : QListViewItem( parent )
{
    initItem( topLevelItem );
}

void KonqSidebarTreeItem::initItem( KonqSidebarTreeTopLevelItem *topLevelItem )
{
    m_topLevelItem = topLevelItem;
    m_bListable = true;
    m_bClickable = true;

    setExpandable( true );
}

void KonqSidebarTreeItem::middleButtonPressed()
{
    emit tree()->part()->getInterfaces()->getExtension()->createNewWindow( externalURL() );
}

KonqSidebarTreeModule * KonqSidebarTreeItem::module() const
{
    return m_topLevelItem->module();
}

KonqSidebarTree * KonqSidebarTreeItem::tree() const
{
    return static_cast<KonqSidebarTree *>(listView());
}
