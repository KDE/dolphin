/* This file is part of the KDE project
   Copyright (C) 2000,2001 Carsten Pfeiffer <pfeiffer@kde.org>

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
#include <kbookmarkdrag.h>
#include <kbookmark.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kprotocolinfo.h>
#include <konq_faviconmgr.h>

#include "konq_treepart.h"
#include "history_item.h"
#include "history_module.h"
#include "history_settings.h"

#define MYMODULE static_cast<KonqHistoryModule*>(module())
#define MYGROUP static_cast<KonqHistoryGroupItem*>(parent())

KonqHistorySettings * KonqHistoryItem::s_settings = 0L;

KonqHistoryItem::KonqHistoryItem( const KonqHistoryEntry *entry,
				  KonqTreeItem * parentItem,
				  KonqTreeTopLevelItem *topLevelItem )
    : KonqTreeItem( parentItem, topLevelItem )
{
    setExpandable( false );
    update( entry );
}

KonqHistoryItem::~KonqHistoryItem()
{
}

void KonqHistoryItem::update( const KonqHistoryEntry *entry )
{
    m_entry = entry;

    QString title( entry->title );
    if ( !title.stripWhiteSpace().isEmpty() &&
	 title != entry->url.url() )
	setText( 0, title );
    else {
	QString path( entry->url.path() );
	if ( path.isEmpty() )
	    path += '/';
	setText( 0, path );
    }

    KonqHistoryGroupItem *group = MYGROUP;
    QString path = entry->url.path();
    if ( group->hasFavIcon() && (path.isNull() || path == "/") )
	setPixmap( 0, *(group->pixmap(0)) );
    else
	setPixmap( 0, SmallIcon(KProtocolInfo::icon( entry->url.protocol() )));

    group->itemUpdated( this ); // update for sorting
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

QDragObject * KonqHistoryItem::dragObject( QWidget * parent, bool /*move*/ )
{
    QString icon = KonqFavIconMgr::iconForURL( m_entry->url.url() );
    KBookmark bookmark = KBookmark::standaloneBookmark( m_entry->title,
                                                        m_entry->url, icon );
    KBookmarkDrag *drag = KBookmarkDrag::newDrag( bookmark, parent );
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
    if ( s_settings->m_detailedTips ) {
        // this weird ordering of %4, %1, %2, %3 is due to the reason, that some 
        // urls seem to contain %N, which would get substituted in the next
        // .arg() calls. So to fix this, we first substitute the last items
        // and then put in the url.
	QString tip = i18n("<qt><center><b>%4</b></center><hr>Last visited: %1<br>First visited: %2<br>Number of times visited: %3</qt>");
	return tip.arg( KGlobal::locale()->formatDateTime( m_entry->lastVisited ) ).arg( KGlobal::locale()->formatDateTime( m_entry->firstVisited ) ).arg( m_entry->numberOfTimesVisited ).arg( m_entry->url.url() );
    }

    return m_entry->url.url();
}

void KonqHistoryItem::paintCell( QPainter *p, const QColorGroup & cg,
				 int column, int width, int alignment )
{
    QDateTime dt;
    QDateTime current = QDateTime::currentDateTime();

    if ( s_settings->m_metricYoungerThan == KonqHistorySettings::DAYS )
	dt = current.addDays( - s_settings->m_valueYoungerThan );
    else
	dt = current.addSecs( - (s_settings->m_valueYoungerThan * 60) );

    if ( m_entry->lastVisited > dt )
	p->setFont( s_settings->m_fontYoungerThan );

    else {
	if ( s_settings->m_metricOlderThan == KonqHistorySettings::DAYS )
	    dt = current.addDays( - s_settings->m_valueOlderThan );
	else
	    dt = current.addSecs( - (s_settings->m_valueOlderThan * 60) );
	
	if ( m_entry->lastVisited < dt )
	    p->setFont( s_settings->m_fontOlderThan );
    }

    KonqTreeItem::paintCell( p, cg, column, width, alignment );
}

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////


KonqHistoryGroupItem::KonqHistoryGroupItem( const KURL& url,
					    KonqTreeTopLevelItem *topLevelItem)
    : KonqTreeItem( topLevelItem, topLevelItem ),
      m_hasFavIcon( false ),
      m_url( url )
{
}

void KonqHistoryGroupItem::setFavIcon( const QPixmap& pix )
{
    setPixmap( 0, pix );
    m_hasFavIcon = true;
}

// the group item itself will be removed automatically,
// when the last child is removed
void KonqHistoryGroupItem::remove()
{
    KURL::List list;
    KonqHistoryItem *child = static_cast<KonqHistoryItem*>( firstChild() );
    while( child ) {
	list.append( child->externalURL() );
	child = static_cast<KonqHistoryItem*>( child->nextSibling() );
    }

    if ( !list.isEmpty() )
	KonqHistoryManager::kself()->emitRemoveFromHistory( list );
}

KonqHistoryItem * KonqHistoryGroupItem::findChild(const KonqHistoryEntry *entry) const
{
    QListViewItem *child = firstChild();
    KonqHistoryItem *item = 0L;

    while ( child ) {
	item = static_cast<KonqHistoryItem *>( child );
	if ( item->entry() == entry )
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

QDragObject * KonqHistoryGroupItem::dragObject( QWidget *parent, bool /*move*/)
{
    QString icon = KonqFavIconMgr::iconForURL( m_url.url() );
    KBookmark bookmark = KBookmark::standaloneBookmark( QString::null, m_url,
							icon );
    KBookmarkDrag *drag = KBookmarkDrag::newDrag( bookmark, parent );
    return drag;
}
