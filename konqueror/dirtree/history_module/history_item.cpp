/* This file is part of the KDE project
   Copyright (C) 2000 Carsten Pfeiffer <pfeiffer@kde.org>

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

#include "konq_treepart.h"
#include "history_item.h"
#include "history_module.h"

#define MYMODULE static_cast<KonqHistoryModule*>(module())

KonqHistoryItem::KonqHistoryItem( KonqTree * /*parent*/,
				  KonqTreeTopLevelItem *topLevelItem,
				  const KonqHistoryEntry *entry)
    : KonqTreeItem( topLevelItem, 0L ),
      m_entry( entry )
{
    setText( 0, m_entry->url );
    setExpandable( false );
}

KonqHistoryItem::~KonqHistoryItem()
{
}

void KonqHistoryItem::update( const KonqHistoryEntry *entry )
{
    setText( 0, m_entry->url );
}

void KonqHistoryItem::itemSelected()
{
    KParts::BrowserExtension * ext = tree()->part()->extension();
    emit ext->enableAction( "copy", true );
    emit ext->enableAction( "cut", true );
    emit ext->enableAction( "paste", false );
}

void KonqHistoryItem::setOpen( bool open )
{
//     if ( open & !childCount() && m_bListable )
//         MYMODULE->openSubFolder( this, m_topLevelItem );

    KonqTreeItem::setOpen( open );
}

void KonqHistoryItem::rightButtonPressed()
{
}
