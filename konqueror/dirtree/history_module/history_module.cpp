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
#include <kconfig.h>
#include <klocale.h>
#include <kprotocolinfo.h>

#include <konq_drag.h>
#include <konq_historymgr.h>
#include <konq_tree.h>

#include "history_module.h"

KonqHistoryModule::KonqHistoryModule( KonqTree * parentTree, const char *name )
    : QObject( 0L, name ), KonqTreeModule( parentTree ),
      m_dict( 3001 ),
      m_topLevelItem( 0L ),
      m_initialized( false )
{
    m_dict.setAutoDelete( true );
    m_currentTime = QDateTime::currentDateTime();
    KConfig *kc = KGlobal::config();
    KConfigGroupSaver cs( kc, "HistorySettings" );
    m_sortsByName = kc->readEntry( "SortHistory", "byName" ) == "byName";

    KonqHistoryManager *manager = KonqHistoryManager::self();

    connect( manager, SIGNAL( loadingFinished() ), SLOT( slotCreateItems() ));
    connect( manager, SIGNAL( cleared() ), SLOT( clear() ));

    connect( manager, SIGNAL( entryAdded( const KonqHistoryEntry * ) ),
	     SLOT( slotEntryAdded( const KonqHistoryEntry * ) ));
    connect( manager, SIGNAL( entryRemoved( const KonqHistoryEntry *) ),
	     SLOT( slotEntryRemoved( const KonqHistoryEntry *) ));

    connect( parentTree, SIGNAL( expanded( QListViewItem * )),
	     SLOT( slotItemExpanded( QListViewItem * )));

    m_collection = new KActionCollection( this, "history actions" );
    (void) new KAction( i18n("&Remove entry"), 0, this,
			SLOT( slotRemoveEntry() ), m_collection, "remove");
    (void) new KAction( i18n("C&lear History"), 0, manager,
			SLOT( emitClear() ), m_collection, "clear");
    (void) new KAction( i18n("&Preferences..."), 0, this,
			SLOT( slotPreferences()), m_collection, "preferences");

    KRadioAction *sort;
    sort = new KRadioAction( i18n("By &Name"), 0, this,
			     SLOT( slotSortByName() ), m_collection, "byName");
    sort->setExclusiveGroup("SortGroup");
    sort->setChecked( m_sortsByName );

    sort = new KRadioAction( i18n("By &Date"), 0, this,
			     SLOT( slotSortByDate() ), m_collection, "byDate");
    sort->setExclusiveGroup("SortGroup");
    sort->setChecked( !m_sortsByName );

    m_folderClosed = SmallIcon( "folder" );
    m_folderOpen = SmallIcon( "folder_open" );
}

KonqHistoryModule::~KonqHistoryModule()
{
    HistoryItemIterator it( m_dict );
    QStringList openGroups;
    while ( it.current() ) {
	if ( it.current()->isOpen() )
	    openGroups.append( it.currentKey() );
	++it;
    }

    KConfig *kc = KGlobal::config();
    KConfigGroupSaver cs( kc, "HistorySettings" );
    kc->writeEntry("OpenGroups", openGroups);
    kc->sync();
}

void KonqHistoryModule::slotCreateItems()
{
    clear();

    KonqHistoryItem *item;
    KonqHistoryEntry *entry;
    KonqHistoryList entries( KonqHistoryManager::self()->entries() );
    KonqHistoryIterator it( entries );
    m_currentTime = QDateTime::currentDateTime();

    QString host;

    while ( (entry = it.current()) ) {
	host = entry->url.host();
	KonqHistoryGroupItem *group = m_dict.find( host );
	if ( !group ) {
	    group = new KonqHistoryGroupItem( host, m_topLevelItem );
	    group->setPixmap( 0, m_folderClosed );
	    m_dict.insert( host, group );
	}

	item = new KonqHistoryItem( entry, group, m_topLevelItem );
	item->setPixmap( 0, SmallIcon( KProtocolInfo::icon( entry->url.protocol() )));
	
	++it;
    }

    KConfig *kc = KGlobal::config();
    KConfigGroupSaver cs( kc, "HistorySettings" );
    QStringList openGroups = kc->readListEntry("OpenGroups");
    QStringList::Iterator it2 = openGroups.begin();
    KonqHistoryGroupItem *group;
    while ( it2 != openGroups.end() ) {
	group = m_dict.find( *it2 );
	if ( group )
	    group->setOpen( true );
	
	++it2;
    }

    m_topLevelItem->sort();
    m_initialized = true;
}

// deletes the listview items but does not affect the history backend
void KonqHistoryModule::clear()
{
    m_dict.clear();
}

void KonqHistoryModule::slotEntryAdded( const KonqHistoryEntry *entry )
{
    if ( !m_initialized )
	return;

    m_currentTime = QDateTime::currentDateTime();
    QString host( entry->url.host() );
    KonqHistoryGroupItem *group = m_dict.find( host );
    if ( !group ) {
	group = new KonqHistoryGroupItem( host, m_topLevelItem );
	group->setPixmap( 0, m_folderClosed );
	m_dict.insert( host, group );
    }

    KonqHistoryItem *item = group->findChild( entry );
    if ( !item ) {
	item = new KonqHistoryItem( entry, group, m_topLevelItem );
	item->setPixmap( 0, SmallIcon( KProtocolInfo::icon( entry->url.protocol() )));
    }
    else
	item->update( entry );

    group->sort();
    m_topLevelItem->sort();
}

void KonqHistoryModule::slotEntryRemoved( const KonqHistoryEntry *entry )
{
    if ( !m_initialized )
	return;

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
}

void KonqHistoryModule::showPopupMenu()
{
    QPopupMenu *sortMenu = new QPopupMenu;
    m_collection->action("byName")->plug( sortMenu );
    m_collection->action("byDate")->plug( sortMenu );

    QPopupMenu *menu = new QPopupMenu;
    m_collection->action("remove")->plug( menu );
    m_collection->action("clear")->plug( menu );
    menu->insertSeparator();
    menu->insertItem( i18n("Sort"), sortMenu );
    menu->insertSeparator();
    m_collection->action("preferences")->plug( menu );

    menu->exec( QCursor::pos() );
    delete menu;
    delete sortMenu;
}

void KonqHistoryModule::slotRemoveEntry()
{
    QListViewItem *item = tree()->selectedItem();
    KonqHistoryItem *hi = dynamic_cast<KonqHistoryItem*>( item );
    if ( hi ) // remove a single entry
	KonqHistoryManager::self()->emitRemoveFromHistory( hi->externalURL());

    else { // remove a group of entries
	KonqHistoryGroupItem *gi = dynamic_cast<KonqHistoryGroupItem*>( item );
	if ( gi )
	    gi->remove();
    }
}

void KonqHistoryModule::slotPreferences()
{
    // ### TODO
}

void KonqHistoryModule::slotSortByName()
{
    m_sortsByName = true;
    sortingChanged();
}

void KonqHistoryModule::slotSortByDate()
{
    m_sortsByName = false;
    sortingChanged();
}

void KonqHistoryModule::sortingChanged()
{
    m_topLevelItem->sort();
    KConfig *kc = KGlobal::config();
    KConfigGroupSaver cs( kc, "HistorySettings" );
    kc->writeEntry( "SortHistory", m_sortsByName ? "byName" : "byDate" );
    kc->sync();
}

void KonqHistoryModule::slotItemExpanded( QListViewItem *item )
{
    if ( item == m_topLevelItem && !m_initialized )
	slotCreateItems();
}

void KonqHistoryModule::groupOpened( KonqHistoryGroupItem *item, bool open )
{
    if ( open )
	item->setPixmap( 0, m_folderOpen );
    else
	item->setPixmap( 0, m_folderClosed );
}

#include "history_module.moc"
