/* This file is part of the KDE project
   Copyright (C) 2000 David Faure <faure@kde.org>
                 2000 Carsten Pfeiffer <pfeiffer@kde.org>

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

#include <konq_historymgr.h>
#include <konq_drag.h>

#include "history_module.h"

class KonqHistoryItem;

KonqHistoryModule::KonqHistoryModule( KonqTree * parentTree, const char *name )
    : QObject( 0L, name ), KonqTreeModule( parentTree ),
      m_dict( 3001 ),
      m_topLevelItem( 0L )
{
    m_dict.setAutoDelete( true );
    KonqHistoryManager *manager = KonqHistoryManager::self();

    connect( manager, SIGNAL( loadingFinished() ), SLOT( slotCreateItems() ));
    connect( manager, SIGNAL( cleared() ), SLOT( clearAll() ));

    connect( manager, SIGNAL( entryAdded( const KonqHistoryEntry * ) ),
	     SLOT( slotEntryAdded( const KonqHistoryEntry * ) ));
    connect( manager, SIGNAL( entryRemoved( const KonqHistoryEntry *) ),
	     SLOT( slotEntryRemoved( const KonqHistoryEntry *) ));
}

void KonqHistoryModule::slotCreateItems()
{
    clearAll();

    KonqHistoryItem *item;
    KonqHistoryEntry *entry;
    KonqHistoryList entries( KonqHistoryManager::self()->entries() );
    KonqHistoryIterator it( entries );

    while ( (entry = it.current()) ) {
	item = new KonqHistoryItem( tree(), m_topLevelItem, entry );
	m_dict.insert( entry, item );
	
	++it;
    }
}

void KonqHistoryModule::clearAll()
{
    m_dict.clear(); // also deletes the listview items
}

void KonqHistoryModule::slotEntryAdded( const KonqHistoryEntry *entry )
{
    KonqHistoryItem *item = m_dict.find( (void*) entry );
    if( !item ) {
	item = new KonqHistoryItem( tree(), m_topLevelItem, entry );
	m_dict.insert( (void*) entry, item );
    }
    else
	item->update( entry );
}

void KonqHistoryModule::slotEntryRemoved( const KonqHistoryEntry *entry )
{
    m_dict.remove( (void*) entry );
}



QDragObject * KonqHistoryModule::dragObject( QWidget * parent, bool move )
{
    KonqHistoryItem *item = static_cast<KonqHistoryItem*>( tree()->selectedItem() );

    if ( !item )
        return 0L;

    KURL::List lst;
    lst.append( item->externalURL() );

    QDragObject * drag = KonqDrag::newDrag( lst, false, parent );

    QPoint hotspot;
    hotspot.setX( item->pixmap( 0 )->width() / 2 );
    hotspot.setY( item->pixmap( 0 )->height() / 2 );
    drag->setPixmap( *(item->pixmap( 0 )), hotspot );

    return drag;
}

void KonqHistoryModule::addTopLevelItem( KonqTreeTopLevelItem * item )
{
    m_topLevelItem = item;
    slotCreateItems(); // hope this is correct here, but it should
    
//    KDesktopFile cfg( item->path(), true );
//     QString icon = cfg.readIcon();
//     stripIcon( icon );
}

#include "history_module.moc"
