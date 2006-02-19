/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "konq_listview.h"
#include "konq_textviewitem.h"

#include <q3header.h>

#include <kdebug.h>
#include <kdirlister.h>

#include <stdlib.h>
#include <assert.h>


KonqTextViewWidget::KonqTextViewWidget( KonqListView *parent, QWidget *parentWidget )
:KonqBaseListViewWidget(parent,parentWidget)
{
   kDebug(1202) << "+KonqTextViewWidget" << endl;
   m_filenameColumn=1;

   // David: This breaks dropping things towards the current directory
   // using the columns != Name.
   // I know, but I want to have it this way and I use it all the time.
   // If I want to have free space, I disable some columns.
   // If people don't like it, they can use a different view type. Alex
   setAllColumnsShowFocus(true);
   setRootIsDecorated(false);

   colors[KTVI_REGULAR]=Qt::black;
   colors[KTVI_EXEC]=QColor(0,170,0);
   colors[KTVI_REGULARLINK]=Qt::black;
   colors[KTVI_DIR]=Qt::black;
   colors[KTVI_DIRLINK]=Qt::black;
   colors[KTVI_BADLINK]=Qt::red;
   colors[KTVI_SOCKET]=Qt::magenta;
   colors[KTVI_FIFO]=Qt::magenta;
   colors[KTVI_UNKNOWN]=Qt::red;
   colors[KTVI_CHARDEV]=Qt::blue;
   colors[KTVI_BLOCKDEV]=Qt::blue;

   m_showIcons=false;
}

KonqTextViewWidget::~KonqTextViewWidget()
{}

void KonqTextViewWidget::createColumns()
{
   if (columns()<2)
   {
      addColumn( i18n("Name"), m_filenameColumnWidth );
      addColumn( " ", fontMetrics().width("@") + 2 );
      setColumnAlignment( 1, Qt::AlignRight );
      //this way the column with the name has the index 0 and
      //so the "jump to filename beginning with this character" works
      header()->moveSection( 0, 2 );
   }
   KonqBaseListViewWidget::createColumns();
}

void KonqTextViewWidget::slotNewItems( const KFileItemList & entries )
{
   //kDebug(1202) << k_funcinfo << entries.count() << endl;

  KFileItemList::const_iterator kit = entries.begin();
  const KFileItemList::const_iterator kend = entries.end();
  for (; kit != kend; ++kit )
  {
      KonqTextViewItem *tmp = new KonqTextViewItem( this, *kit );
      if ( !m_itemFound && tmp->text(0) == m_itemToGoTo )
      {
         setCurrentItem( tmp );
         m_itemFound = true;
      }
      if ( !m_itemsToSelect.isEmpty() ) {
         QStringList::Iterator tsit = m_itemsToSelect.find( (*kit)->name() );
         if ( tsit != m_itemsToSelect.end() ) {
            m_itemsToSelect.remove( tsit );
            setSelected( tmp, true );
         }
      }

   }

   m_pBrowserView->newItems( entries );

   if ( !viewport()->isUpdatesEnabled() )
   {
      viewport()->setUpdatesEnabled( true );
      setUpdatesEnabled( true );
      triggerUpdate();
   }
   slotUpdateBackground();
}

void KonqTextViewWidget::setComplete()
{
   kDebug(1202) << k_funcinfo << "Update Contents Pos: "
                 << m_bUpdateContentsPosAfterListing << endl;

   m_bTopLevelComplete = true;

   // Alex: this flag is set when we are just finishing a voluntary listing,
   // so do the go-to-item thing only under here. When we update the
   // current directory automatically (e.g. after a file has been deleted),
   // we don't want to go to the first item ! (David)
   if ( m_bUpdateContentsPosAfterListing )
   {
      m_bUpdateContentsPosAfterListing = false;

      if ( !m_itemFound )
         setCurrentItem( firstChild() );

      if ( !m_restored && !m_pBrowserView->extension()->urlArgs().reload )
         ensureItemVisible( currentItem() );
      else
         setContentsPos( m_pBrowserView->extension()->urlArgs().xOffset,
                         m_pBrowserView->extension()->urlArgs().yOffset );

      activateAutomaticSelection();
      emit selectionChanged();
   }

   m_itemToGoTo = "";
   m_restored = false;

   // Show "cut" icons as such
   m_pBrowserView->slotClipboardDataChanged();
   // Show totals
   slotOnViewport();

   if ( !isUpdatesEnabled() || !viewport()->isUpdatesEnabled() )
   {
      viewport()->setUpdatesEnabled( true );
      setUpdatesEnabled( true );
      triggerUpdate();
   }
}

bool KonqTextViewWidget::isExecuteArea( const QPoint& point )
{
   if (!itemAt( point ) )
      return false;
   int x=point.x();

   int offset = 0;
   int width = columnWidth( 0 );
   int pos = header()->mapToIndex( 0 );

   for ( int index = 0; index < pos; index++ )
      offset += columnWidth( header()->mapToSection( index ) );

   return ( x > offset && x < ( offset + width ) );
}

/*void KonqTextViewWidget::viewportDragMoveEvent( QDragMoveEvent *_ev )
{
   KonqBaseListViewItem *item =
       isNameColumn( _ev->pos() ) ? (KonqBaseListViewItem*)itemAt( _ev->pos() ) : 0L;

   if ( !item )
   {
      if ( m_dragOverItem )
         setSelected( m_dragOverItem, false );
      _ev->accept();
      return;
   }

   if ( m_dragOverItem == item )
       return; // No change

   if ( m_dragOverItem != 0L )
      setSelected( m_dragOverItem, false );

   if ( item->item()->acceptsDrops() )
   {
      _ev->accept();
      setSelected( item, true );
      m_dragOverItem = item;
   }
   else
   {
      _ev->ignore();
      m_dragOverItem = 0L;
   }
}

void KonqTextViewWidget::viewportDropEvent( QDropEvent *ev  )
{
   if ( m_dirLister->url().isEmpty() )
      return;
   kDebug() << "KonqTextViewWidget::viewportDropEvent" << endl;
   if ( m_dragOverItem != 0L )
      setSelected( m_dragOverItem, false );
   m_dragOverItem = 0L;

   ev->accept();

   // We dropped on an item only if we dropped on the Name column.
   KonqBaseListViewItem *item =
       isNameColumn( ev->pos() ) ? (KonqBaseListViewItem*)itemAt( ev->pos() ) : 0;

   KFileItem * destItem = (item) ? item->item() : m_dirLister->rootItem(); // may be 0
   KonqOperations::doDrop( destItem, destItem ? destItem->url() : url(), ev, this );
}*/


#include "konq_textviewwidget.moc"
