/* This file is part of the KDE project
   Copyright (C) 2000 Carsten Pfeiffer <pfeiffer@kde.org>
                 2000 David Faure <faure@kde.org>

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
#include <kmimetype.h>
#include <kprotocolinfo.h>

#include <konq_drag.h>
#include <konq_historymgr.h>
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

    m_folderPixmap = KMimeType::mimeType( "inode/directory" )->pixmap( KIcon::NoGroup, KIcon::SizeSmall );
}

void KonqHistoryModule::slotCreateItems()
{
    clearAll();

    KonqHistoryEntry *entry;
    KonqHistoryItem *item;
    KonqHistoryList entries( KonqHistoryManager::self()->entries() );
    KonqHistoryIterator it( entries );

    QString host;
    
    while ( (entry = it.current()) ) {
	QString host( entry->url.host() );
	KonqHistoryGroupItem *group = m_dict.find( host );
	if ( !group ) {
	    group = new KonqHistoryGroupItem( host, m_topLevelItem );
	    group->setPixmap( 0, m_folderPixmap );
	    m_dict.insert( host, group );
	}

	item = new KonqHistoryItem( entry, group, m_topLevelItem );
	item->setPixmap( 0, SmallIcon( KProtocolInfo::icon( entry->url.protocol() )));
	
	++it;
    }
}

// deletes the listview items but does not affect the history backend
void KonqHistoryModule::clearAll()
{
    m_dict.clear();
}

void KonqHistoryModule::slotEntryAdded( const KonqHistoryEntry *entry )
{
    QString host( entry->url.host() );
    KonqHistoryGroupItem *group = m_dict.find( host );
    if ( !group ) {
	group = new KonqHistoryGroupItem( host, m_topLevelItem );
	group->setPixmap( 0, m_folderPixmap );
	m_dict.insert( host, group );
    }

    KonqHistoryItem *item = group->findChild( entry );
    if ( !item ) {
	item = new KonqHistoryItem( entry, group, m_topLevelItem );
	item->setPixmap( 0, SmallIcon( KProtocolInfo::icon( entry->url.protocol() )));
    }
    else
	item->update( entry );
}

void KonqHistoryModule::slotEntryRemoved( const KonqHistoryEntry *entry )
{
    QString host( entry->url.host() );
    KonqHistoryGroupItem *group = m_dict.find( host );
    if ( !group )
	return;

    delete group->findChild( entry );
    
    if ( group->childCount() == 0 )
	m_dict.remove( host );
}

void KonqHistoryModule::addTopLevelItem( KonqTreeTopLevelItem * item )
{
    m_topLevelItem = item;
    slotCreateItems(); // hope this is correct here, but it should
}

void KonqHistoryModule::showPopupMenu()
{
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

    KonqHistoryManager::self()->emitRemoveFromHistory( item->externalURL() );
}

void KonqHistoryModule::slotPreferences()
{
    // ### TODO
}

#include "history_module.moc"
