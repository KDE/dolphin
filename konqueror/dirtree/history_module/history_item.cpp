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
#include <konq_drag.h>

#include "konq_treepart.h"
#include "history_item.h"
#include "history_module.h"

#define MYMODULE static_cast<KonqHistoryModule*>(module())
#define MYGROUP static_cast<KonqHistoryGroupItem*>(parent())

KonqHistoryItem::KonqHistoryItem( const KonqHistoryEntry *entry,
				  KonqTreeItem * parentItem,
				  KonqTreeTopLevelItem *topLevelItem )
    : KonqTreeItem( parentItem, topLevelItem ),
      m_entry( entry )
{
    setExpandable( false );
    update( entry );
    MYGROUP->itemUpdated( this );
}

KonqHistoryItem::~KonqHistoryItem()
{
}

void KonqHistoryItem::update( const KonqHistoryEntry *entry )
{
    QString title( entry->title );
    if ( !title.isEmpty() && title != entry->url.url() )
	setText( 0, title );
    else {
	QString path( entry->url.path() );
	if ( path.isEmpty() )
	    path += '/';
	setText( 0, path );
    }

    MYGROUP->itemUpdated( this ); // update for sorting
}

void KonqHistoryItem::itemSelected()
{
    tree()->part()->extension()->enableActions( true, true, false,
                                                false, false, false );
}

void KonqHistoryItem::rightButtonPressed()
{
    MYMODULE->showPopupMenu();
}

QDragObject * KonqHistoryItem::dragObject( QWidget * parent, bool move )
{
    KURL::List lst;
    lst.append( externalURL() );

    KonqDrag * drag = KonqDrag::newDrag( lst, false, parent );
    drag->setMoveSelection( move );
    return drag;
}

// new items go on top
QString KonqHistoryItem::key( int column, bool ascending ) const
{
    if ( MYMODULE->sortsByName() )
	return KonqTreeItem::key( column, ascending );

    QString tmp;
    tmp.sprintf( "%08d", m_entry->lastVisited.secsTo(MYMODULE->currentTime()));
    return tmp;
}

QString KonqHistoryItem::toolTipText() const
{
    // ### date, etc. all in richtext :)
    return m_entry->url.url();
}


///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////


KonqHistoryGroupItem::KonqHistoryGroupItem( const QString& host,
					    KonqTreeTopLevelItem *topLevelItem )
    : KonqTreeItem( topLevelItem, topLevelItem ),
      m_host( host )
{
    setText( 0, host );
}

// the group item itself will be removed automatically,
// when the last child is removed
void KonqHistoryGroupItem::remove()
{
    KonqHistoryManager *manager = KonqHistoryManager::self();
    KonqHistoryItem *child = static_cast<KonqHistoryItem*>( firstChild() );
    while( child ) {
	manager->emitRemoveFromHistory( child->externalURL() );
	child = static_cast<KonqHistoryItem*>( child->nextSibling() );
    }
}

KonqHistoryItem * KonqHistoryGroupItem::findChild(const KonqHistoryEntry *entry) const
{
    const KURL& url = entry->url;
    QListViewItem *child = firstChild();
    KonqHistoryItem *item = 0L;

    while ( child ) {
	item = static_cast<KonqHistoryItem *>( child );
	if ( item->url() == url )
	    return item;

	child = child->nextSibling();
    }

    return 0L;
}

void KonqHistoryGroupItem::itemSelected()
{
    tree()->part()->extension()->enableActions( false, false, false,
                                                false, false, false );
}

void KonqHistoryGroupItem::rightButtonPressed()
{
    MYMODULE->showPopupMenu();
}

// let the module change our pixmap (opened/closed)
void KonqHistoryGroupItem::setOpen( bool open )
{
    MYMODULE->groupOpened( this, open );
    KonqTreeItem::setOpen( open );
}

// new items go on top
QString KonqHistoryGroupItem::key( int column, bool ascending ) const
{
    if ( !m_lastVisited.isValid() || MYMODULE->sortsByName() )
	return KonqTreeItem::key( column, ascending );

    QString tmp;
    tmp.sprintf( "%08d", m_lastVisited.secsTo( MYMODULE->currentTime() ));
    return tmp;
}

void KonqHistoryGroupItem::itemUpdated( KonqHistoryItem *item )
{
    if ( !m_lastVisited.isValid() || m_lastVisited < item->lastVisited() )
	m_lastVisited = item->lastVisited();
}
