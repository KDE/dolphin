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

#include <qpopupmenu.h>

#include <kaction.h>
#include <klocale.h>

#include <konq_historymgr.h>
#include <konq_drag.h>
#include <konq_tree.h>

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
    
    m_collection = new KActionCollection( this, "history actions" );
    (void) new KAction( i18n("&Remove entry"), 0, this,
			SLOT( slotRemoveEntry() ), m_collection, "remove");
    (void) new KAction( i18n("C&lear History"), 0, manager,
			SLOT( emitClear() ), m_collection, "clear");
    (void) new KAction( i18n("Preferences..."), 0, this, 
			SLOT( slotPreferences()), m_collection, "preferences");
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

void KonqHistoryModule::addTopLevelItem( KonqTreeTopLevelItem * item )
{
    m_topLevelItem = item;
    slotCreateItems(); // hope this is correct here, but it should
}

void KonqHistoryModule::showPopupMenu( KonqHistoryItem * /*item*/ )
{
    qDebug("*** coll: %i", m_collection->count());
    
    QPopupMenu *menu = new QPopupMenu;
    m_collection->action("remove")->plug( menu );
    m_collection->action("clear")->plug( menu );
    menu->insertSeparator();
    m_collection->action("preferences")->plug( menu );
    
    menu->popup( QCursor::pos() );
}

void KonqHistoryModule::slotRemoveEntry()
{
    KonqHistoryItem *item = static_cast<KonqHistoryItem*>(tree()->selectedItem());
    if ( !item )
	return;
    
    KonqHistoryManager::self()->emitRemoveFromHistory( item->url() );
}

void KonqHistoryModule::slotPreferences()
{
    
}

#include "history_module.moc"
