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

KonqHistoryItem::KonqHistoryItem( const KonqHistoryEntry *entry,
				  KonqTreeItem * parentItem,
				  KonqTreeTopLevelItem *topLevelItem )
    : KonqTreeItem( parentItem, topLevelItem ),
      m_entry( entry )
{
    setExpandable( false );
    update( entry );
}

KonqHistoryItem::~KonqHistoryItem()
{
}

void KonqHistoryItem::update( const KonqHistoryEntry */*entry*/ )
{
    QString title( m_entry->title );
    if ( !title.isEmpty() && title != m_entry->url.url() )
	setText( 0, title );
    else {
	QString path( m_entry->url.path() );
	if ( path.isEmpty() )
	    path += '/';
	setText( 0, path );
    }
}

void KonqHistoryItem::itemSelected()
{
    tree()->part()->extension()->enableActions( true, true, false,
                                                false, false, false );
}


void KonqHistoryItem::setOpen( bool open )
{
    // ### TODO: lazy filling?
    KonqTreeItem::setOpen( open );
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

    const QPixmap *pix = pixmap(0);
    if ( pix ) {
	QPoint hotspot;
	hotspot.setX( pix->width() / 2 );
	hotspot.setY( pix->height() / 2 );
	drag->setPixmap( *pix, hotspot );
    }

    return drag;
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
    KonqHistoryItem *child = static_cast<KonqHistoryItem *>( firstChild() );
    while ( child ) {
	if ( child->url() == url )
	    return child;

	child = static_cast<KonqHistoryItem *>( child->nextSibling() );
    }

    return 0L;
}

void KonqHistoryGroupItem::itemSelected()
{
    KParts::BrowserExtension * ext = tree()->part()->extension();
    emit ext->enableAction( "copy", false );
    emit ext->enableAction( "cut", false );
    emit ext->enableAction( "paste", false );
}

void KonqHistoryGroupItem::rightButtonPressed()
{
    MYMODULE->showPopupMenu();
}
