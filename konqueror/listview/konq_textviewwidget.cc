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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "konq_listview.h"
#include "konq_textviewwidget.h"
#include "konq_textviewitem.h"

#include <qdragobject.h>
#include <qheader.h>

#include <kdebug.h>
#include <konq_dirlister.h>
#include <kglobal.h>
#include <kio/job.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kprotocolmanager.h>

#include <konq_settings.h>
#include "konq_propsview.h"

#include <stdlib.h>
#include <assert.h>

KonqTextViewWidget::KonqTextViewWidget( KonqListView *parent, QWidget *parentWidget )
:KonqBaseListViewWidget(parent,parentWidget)
//,timer()
{
   kdDebug(1202) << "+KonqTextViewWidget" << endl;
   m_filenameColumn=1;

   // David: This breaks dropping things towards the current directory
   // using the columns != Name.
   // I know, but I want to have it this way and I use it all the time.
   // If I want to have free space, I disable some columns.
   // If people don't like it, they can use a different view type. Alex
   setAllColumnsShowFocus(TRUE);
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

   highlight[KTVI_REGULAR]=Qt::white;
   highlight[KTVI_EXEC]=colors[KTVI_EXEC].light(200);
   highlight[KTVI_REGULARLINK]=Qt::white;
   highlight[KTVI_DIR]=Qt::white;
   highlight[KTVI_DIRLINK]=Qt::white;
   highlight[KTVI_BADLINK]=colors[KTVI_BADLINK].light();
   highlight[KTVI_SOCKET]=colors[KTVI_SOCKET].light();
   highlight[KTVI_FIFO]=colors[KTVI_FIFO].light();
   highlight[KTVI_UNKNOWN]=colors[KTVI_UNKNOWN].light();
   highlight[KTVI_CHARDEV]=colors[KTVI_CHARDEV].light(180);
   highlight[KTVI_BLOCKDEV]=colors[KTVI_BLOCKDEV].light(180);
   //timer.start();
   m_showIcons=FALSE;
}

KonqTextViewWidget::~KonqTextViewWidget()
{}

void KonqTextViewWidget::createColumns()
{
   //the textview has fixed size columns
   //these both columns are always required, so add them
   if (columns()<2)
   {
      addColumn(i18n("Name"),fontMetrics().width("_a_quite_long_filename_"));
      addColumn(" ",fontMetrics().width("@")+2);
      setColumnAlignment(1,AlignRight);
      //this way the column with the name has the index 0 and
      //so the "jump to filename beginning with this character" works
      header()->moveSection(0,2);
   };
   setSorting(0,TRUE);

   //remove all but the first two columns
   for (int i=columns()-1; i>1; i--)
      removeColumn(i);

   int currentColumn(m_filenameColumn+1);
   //now add the checked columns
   for (int i=0; i<NumberOfAtoms; i++)
   {
      if ((confColumns[i].displayThisOne) && (confColumns[i].displayInColumn==currentColumn))
      {
         if (sortedByColumn==confColumns[i].desktopFileName)
            setSorting(currentColumn,m_bAscending);
         ColumnInfo *tmpColumn=&confColumns[i];
         QCString tmpName=tmpColumn->name.utf8();
         if (tmpColumn->udsId==KIO::UDS_SIZE)
         {
            addColumn(i18n(tmpName),fontMetrics().width(KGlobal::locale()->formatNumber(888888888, 0)+"  "));
            setColumnAlignment(currentColumn,AlignRight);
         }
         else if ((tmpColumn->udsId==KIO::UDS_MODIFICATION_TIME)
                  || (tmpColumn->udsId==KIO::UDS_ACCESS_TIME)
                  || (tmpColumn->udsId==KIO::UDS_CREATION_TIME))
         {
            QDateTime dt(QDate(2000,10,10),QTime(20,20,20));
            //addColumn(i18n(tmpName),fontMetrics().width(KGlobal::locale()->formatDateTime(dt)+"--"));
            addColumn(i18n(tmpName),fontMetrics().width(KGlobal::locale()->formatDate(dt.date(),TRUE)+KGlobal::locale()->formatTime(dt.time())+"----"));
            setColumnAlignment(currentColumn,AlignCenter);
         }
         else if (tmpColumn->udsId==KIO::UDS_ACCESS)
            addColumn(i18n(tmpName),fontMetrics().width("--Permissions--"));
         else if (tmpColumn->udsId==KIO::UDS_USER)
            addColumn(i18n(tmpName),fontMetrics().width("a_long_username"));
         else if (tmpColumn->udsId==KIO::UDS_GROUP)
            addColumn(i18n(tmpName),fontMetrics().width("a_groupname"));
         else if (tmpColumn->udsId==KIO::UDS_LINK_DEST)
            addColumn(i18n(tmpName),fontMetrics().width("_a_quite_long_filename_"));
         else if (tmpColumn->udsId==KIO::UDS_FILE_TYPE)
            addColumn(i18n(tmpName),fontMetrics().width("a_comment_for_mimetype_"));
         else if (tmpColumn->udsId==KIO::UDS_MIME_TYPE)
            addColumn(i18n(tmpName),fontMetrics().width("_a_long_/_mimetype_"));
         else if (tmpColumn->udsId==KIO::UDS_URL)
            addColumn(i18n(tmpName),fontMetrics().width("_a_long_lonq_long_url_"));

         i=-1;
         currentColumn++;
      };
   };
   if (sortedByColumn=="FileName")
      setSorting(0,m_bAscending);

};

void KonqTextViewWidget::slotNewItems( const KFileItemList & entries )
{
   for( QListIterator<KFileItem> kit (entries); kit.current(); ++kit )
   {
      KonqTextViewItem *tmp=new KonqTextViewItem( this,static_cast<KonqFileItem*> (*kit));
      if (m_goToFirstItem==false)
         if (m_itemFound==false)
            if (tmp->text(0)==m_itemToGoTo)
            {
               setCurrentItem(tmp);
               ensureItemVisible(tmp);
               emit selectionChanged();
               selectCurrentItemAndEnableSelectedBySimpleMoveMode();
               m_itemFound=true;
            };
   };
   m_pBrowserView->newItems( entries );
   //kdDebug(1202)<<"::slotNewItem: received: "<<entries.count()<<endl;

   if ( !viewport()->isUpdatesEnabled() )
   {
      viewport()->setUpdatesEnabled( true );
      setUpdatesEnabled( true );
      triggerUpdate();
   }
}

void KonqTextViewWidget::setComplete()
{
   m_bTopLevelComplete = true;

   // Alex: this flag is set when we are just finishing a voluntary listing,
   // so do the go-to-item thing only under here. When we update the
   // current directory automatically (e.g. after a file has been deleted),
   // we don't want to go to the first item ! (David)
   if ( m_bUpdateContentsPosAfterListing )
   {
      kdDebug() << "KonqTextViewWidget::setComplete m_bUpdateContentsPosAfterListing=true" << endl;
      m_bUpdateContentsPosAfterListing = false;

      setContentsPos( m_pBrowserView->extension()->urlArgs().xOffset,
                      m_pBrowserView->extension()->urlArgs().yOffset );

      if ((m_goToFirstItem==true) || (m_itemFound==false))
      {
          kdDebug() << "going to first item" << endl;
          setCurrentItem(firstChild());
          selectCurrentItemAndEnableSelectedBySimpleMoveMode();
      }
      ensureItemVisible(currentItem());
   }
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


bool KonqTextViewWidget::isNameColumn(const QPoint& point )
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

void KonqTextViewWidget::viewportDragMoveEvent( QDragMoveEvent *_ev )
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
   kdDebug() << "KonqTextViewWidget::viewportDropEvent" << endl;
   if ( m_dragOverItem != 0L )
      setSelected( m_dragOverItem, false );
   m_dragOverItem = 0L;

   ev->accept();

   // We dropped on an item only if we dropped on the Name column.
   KonqBaseListViewItem *item =
       isNameColumn( ev->pos() ) ? (KonqBaseListViewItem*)itemAt( ev->pos() ) : 0;

   KonqFileItem * destItem = (item) ? item->item() : static_cast<KonqFileItem *>(m_dirLister->rootItem());
   KonqOperations::doDrop( destItem /*may be 0L*/, destItem ? destItem->url() : url(), ev, this );
}


#include "konq_textviewwidget.moc"
