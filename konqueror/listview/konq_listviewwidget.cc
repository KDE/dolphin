/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>
                 2001, 2002 Michael Brade <brade@kde.org>

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

#include <qtimer.h>
#include <qevent.h>
#include <qcursor.h>

#include <konq_settings.h>

#include <kdebug.h>
#include <kdirlister.h>
#include <klocale.h>
#include <kprotocolinfo.h>
#include <kaction.h>
#include <kurldrag.h>
#include <kmessagebox.h>
#include <qheader.h>
#include <qpainter.h>
#include <qstyle.h>

#include <stdlib.h>
#include <assert.h>
#include <kiconloader.h>
#include <qfile.h>

ColumnInfo::ColumnInfo()
   :displayInColumn(-1)
   ,name()
   ,desktopFileName()
   ,udsId(0)
   ,displayThisOne(FALSE)
   ,toggleThisOne(0)
{}


ColumnInfo::ColumnInfo(const QString& n, const QString& desktopName, int kioUds,int count,bool enabled,KToggleAction* someAction)
   :displayInColumn(count)
   ,name(n)
   ,desktopFileName(desktopName)
   ,udsId(kioUds)
   ,displayThisOne(enabled)
   ,toggleThisOne(someAction)
{}

void ColumnInfo::setData(const QString& n, const QString& desktopName, int kioUds,int count,bool enabled,KToggleAction* someAction)
{
   displayInColumn=count;
   name=n;
   desktopFileName=desktopName;
   udsId=kioUds;
   displayThisOne=enabled;
   toggleThisOne=someAction;
}


KonqBaseListViewWidget::KonqBaseListViewWidget( KonqListView *parent, QWidget *parentWidget)
    :KListView(parentWidget)
,sortedByColumn(0)
,m_pBrowserView(parent)
,m_dirLister(new KDirLister( true /*m_showIcons==FALSE*/))
,m_dragOverItem(0)
,m_activeItem(0)
,m_selected(0)
,m_scrollTimer(0)
,m_rubber(0)
,m_showIcons(true)
,m_bCaseInsensitive(true)
,m_bUpdateContentsPosAfterListing(false)
,m_bAscending(true)
,m_itemFound(false)
,m_restored(false)
,m_filenameColumn(0)
,m_itemToGoTo("")
,m_backgroundTimer(0)
{
   kdDebug(1202) << "+KonqBaseListViewWidget" << endl;

   m_dirLister->setMainWindow(topLevelWidget());

   m_bTopLevelComplete  = true;

   //Adjust KListView behaviour
   setMultiSelection(true);
   setSelectionModeExt( FileManager );
   setDragEnabled(true);
   setItemsMovable(false);

   initConfig();
#if 0
   connect( this, SIGNAL(rightButtonPressed(QListViewItem*,const QPoint&,int)),
            this, SLOT(slotRightButtonPressed(QListViewItem*,const QPoint&,int)));
#endif
   connect( this, SIGNAL(returnPressed(QListViewItem*)),
            this, SLOT(slotReturnPressed(QListViewItem*)));
   connect( this, SIGNAL( mouseButtonClicked(int, QListViewItem*, const QPoint&, int)),
            this, SLOT( slotMouseButtonClicked(int, QListViewItem*, const QPoint&, int)) );
   connect( this, SIGNAL(executed(QListViewItem* )),
            this, SLOT(slotExecuted(QListViewItem*)));

   connect( this, SIGNAL(currentChanged(QListViewItem*)),
            this, SLOT(slotCurrentChanged(QListViewItem*)));
   connect( this, SIGNAL(itemRenamed(QListViewItem*, const QString &, int)),
            this, SLOT(slotItemRenamed(QListViewItem*, const QString &, int)));
   connect( this, SIGNAL(contextMenuRequested(QListViewItem*, const QPoint&, int)),
            this, SLOT(slotPopupMenu(QListViewItem*, const QPoint&, int)));
   connect( this, SIGNAL(selectionChanged()),
            this, SLOT(updateSelectedFilesInfo()));

   connect( horizontalScrollBar(), SIGNAL(valueChanged(int)),
            this, SIGNAL(viewportAdjusted()));
   connect( verticalScrollBar(), SIGNAL(valueChanged(int)),
            this, SIGNAL(viewportAdjusted()));

   // Connect the directory lister
   connect( m_dirLister, SIGNAL( started( const KURL & ) ),
            this, SLOT( slotStarted() ) );
   connect( m_dirLister, SIGNAL( completed() ), this, SLOT( slotCompleted() ) );
   connect( m_dirLister, SIGNAL( canceled() ), this, SLOT( slotCanceled() ) );
   connect( m_dirLister, SIGNAL( clear() ), this, SLOT( slotClear() ) );
   connect( m_dirLister, SIGNAL( newItems( const KFileItemList & ) ),
            this, SLOT( slotNewItems( const KFileItemList & ) ) );
   connect( m_dirLister, SIGNAL( deleteItem( KFileItem * ) ),
            this, SLOT( slotDeleteItem( KFileItem * ) ) );
   connect( m_dirLister, SIGNAL( refreshItems( const KFileItemList & ) ),
            this, SLOT( slotRefreshItems( const KFileItemList & ) ) );
   connect( m_dirLister, SIGNAL( redirection( const KURL & ) ),
            this, SLOT( slotRedirection( const KURL & ) ) );
   connect( m_dirLister, SIGNAL( itemsFilteredByMime( const KFileItemList & ) ),
            m_pBrowserView, SIGNAL( itemsFilteredByMime( const KFileItemList & ) ) );

   connect( m_dirLister, SIGNAL( infoMessage( const QString& ) ),
            m_pBrowserView->extension(), SIGNAL( infoMessage( const QString& ) ) );
   connect( m_dirLister, SIGNAL( percent( int ) ),
            m_pBrowserView->extension(), SIGNAL( loadingProgress( int ) ) );
   connect( m_dirLister, SIGNAL( speed( int ) ),
            m_pBrowserView->extension(), SIGNAL( speedProgress( int ) ) );

   connect( header(), SIGNAL( sizeChange( int, int, int ) ), SLOT( slotUpdateBackground() ) );

   viewport()->setMouseTracking( true );
   viewport()->setFocusPolicy( QWidget::WheelFocus );
   setFocusPolicy( QWidget::WheelFocus );
   setAcceptDrops( true );

   //looks better with the statusbar
   setFrameStyle( QFrame::StyledPanel | QFrame::Sunken );
   setShowSortIndicator(true);
}

KonqBaseListViewWidget::~KonqBaseListViewWidget()
{
   kdDebug(1202) << "-KonqBaseListViewWidget" << endl;

   delete m_selected; m_selected = 0;

   // TODO: this is a hack, better fix the connections of m_dirLister if possible!
   m_dirLister->disconnect( this );
   delete m_dirLister;
}

void KonqBaseListViewWidget::readProtocolConfig( const QString & protocol )
{
   KConfig * config = KGlobal::config();
   if ( config->hasGroup( "ListView_" + protocol ) )
      config->setGroup( "ListView_" + protocol );
   else
      config->setGroup( "ListView_default" );

   sortedByColumn=config->readEntry("SortBy","FileName");
   m_bAscending=config->readBoolEntry("SortOrder",TRUE);

   bool defaultColumns = false;
   QStringList lstColumns = config->readListEntry( "Columns" );
   if (lstColumns.isEmpty())
   {
      // Default column selection
      defaultColumns = true;
      lstColumns.append( "Size" );
      lstColumns.append( "File Type" );
      lstColumns.append( "Modified" );
      lstColumns.append( "Permissions" );
      lstColumns.append( "Owner" );
      lstColumns.append( "Group" );
      lstColumns.append( "Link" );
   }
   // Default number of columns
   NumberOfAtoms = 11;
   int extraIndex = NumberOfAtoms;

   // Check for any extra data
   KURL url;
   url.setProtocol(protocol);
   KProtocolInfo::ExtraFieldList extraFields = KProtocolInfo::extraFields(url);
   NumberOfAtoms += extraFields.count();
   confColumns.resize( NumberOfAtoms );

   KProtocolInfo::ExtraFieldList::Iterator extraFieldsIt = extraFields.begin();
   if ( defaultColumns )
   {
       int num = 1;
       for ( ; extraFieldsIt != extraFields.end(); ++extraFieldsIt, ++num )
       {
           QString column = (*extraFieldsIt).name;
           lstColumns << column;
           QString type = (*extraFieldsIt).type; // ## TODO use when sorting
           confColumns[extraIndex++].setData( column, QString("Extra%1").arg(num), KIO::UDS_EXTRA, -1, FALSE, 0);
       }
   }


   //disable everything
   for (unsigned int i=0; i<NumberOfAtoms; i++)
   {
      confColumns[i].displayThisOne=FALSE;
      confColumns[i].displayInColumn=-1;
      if ( confColumns[i].toggleThisOne )
      {
          confColumns[i].toggleThisOne->setChecked(FALSE);
          confColumns[i].toggleThisOne->setEnabled(TRUE);
      }
   }
   int currentColumn = m_filenameColumn+1;
   //check all columns in lstColumns
   for (unsigned int i=0; i<lstColumns.count(); i++)
   {
      //search the column in confColumns
      for (unsigned int j=0; j<NumberOfAtoms; j++)
      {
         if (confColumns[j].name==*lstColumns.at(i))
         {
            confColumns[j].displayThisOne=TRUE;
            confColumns[j].displayInColumn=currentColumn;
            if ( confColumns[j].toggleThisOne )
                confColumns[j].toggleThisOne->setChecked(TRUE);
            currentColumn++;
            break;
         }
      }
   }
   //check what the protocol provides
   QStringList listingList=KProtocolInfo::listing(protocol);
   kdDebug(1202) << k_funcinfo << "protocol: " << protocol << endl;

   // Even if this is not given by the protocol, we can determine it.
   // Please don't remove this ;-). It makes it possible to show the file type
   // using the mimetype comment, which for most users is a nicer alternative
   // than the raw mimetype name.
   listingList.append( "MimeType" );
   for (unsigned int i=0; i<NumberOfAtoms; i++)
   {
      if ((confColumns[i].udsId==KIO::UDS_URL) || (confColumns[i].udsId==KIO::UDS_MIME_TYPE))
         continue;
      QStringList::Iterator listIt = listingList.find(confColumns[i].desktopFileName);
      if ( listIt == listingList.end() ) // not found -> hide
      {
         //move all columns behind one to the front
         for (unsigned int l=0; l<NumberOfAtoms; l++)
            if (confColumns[i].displayInColumn>confColumns[i].displayInColumn)
               confColumns[i].displayInColumn--;
         //disable this column
         confColumns[i].displayThisOne=FALSE;
         confColumns[i].toggleThisOne->setEnabled(FALSE);
         confColumns[i].toggleThisOne->setChecked(FALSE);
      }
   }
}

void KonqBaseListViewWidget::createColumns()
{
   //this column is always required, so add it
   if (columns()<1) addColumn(i18n("Name"));
   setSorting(0,TRUE);

   //remove all but the first column
   for (int i=columns()-1; i>0; i--)
      removeColumn(i);
   //now add the checked columns
   int currentColumn(1);
   for (int i=0; i<NumberOfAtoms; i++)
   {
      if ((confColumns[i].displayThisOne) && (confColumns[i].displayInColumn==currentColumn))
      {
         addColumn(i18n(confColumns[i].name.utf8() ));
         if (sortedByColumn == confColumns[i].desktopFileName)
            setSorting(currentColumn,m_bAscending);
         if (confColumns[i].udsId==KIO::UDS_SIZE)
             setColumnAlignment(currentColumn,AlignRight);
         i=-1;
         currentColumn++;
      }
   }
   if (sortedByColumn=="FileName")
      setSorting(0,m_bAscending);
   //for (unsigned int i=0; i<NumberOfAtoms; i++)
   //  kdDebug(1202)<<"i: "<<i<<" name: "<<confColumns[i].name<<" disp: "<<confColumns[i].displayInColumn<<" on: "<<confColumns[i].displayThisOne<<endl;
}

void KonqBaseListViewWidget::stop()
{
   m_dirLister->stop();
}

const KURL & KonqBaseListViewWidget::url()
{
   return m_url;
}

void KonqBaseListViewWidget::updateSelectedFilesInfo()
{
   // Display statusbar info, and emit selectionInfo
   KFileItemList lst = selectedFileItems();
   m_pBrowserView->emitCounts( lst, true );
}

void KonqBaseListViewWidget::initConfig()
{
   m_pSettings = KonqFMSettings::settings();

   QFont stdFont( m_pSettings->standardFont() );
   setFont( stdFont );
   //TODO: create config GUI
   QFont itemFont( m_pSettings->standardFont() );
   itemFont.setUnderline( m_pSettings->underlineLink() );
   setItemFont( itemFont );
   setItemColor( m_pSettings->normalTextColor() );

   updateListContents();
}

void KonqBaseListViewWidget::contentsMousePressEvent( QMouseEvent *e )
{
   if ( m_rubber )
   {
      drawRubber();
      delete m_rubber;
      m_rubber = 0;
   }

   delete m_selected;
   m_selected = new QPtrList<KonqBaseListViewItem>;

   QPoint vp = contentsToViewport( e->pos() );
   KonqBaseListViewItem* item = isExecuteArea( vp ) ?
         static_cast<KonqBaseListViewItem*>( itemAt( vp ) ) : 0L;

   if ( item )
      KListView::contentsMousePressEvent( e );
   else {
      if ( e->button() == LeftButton )
      {
         m_rubber = new QRect( e->x(), e->y(), 0, 0 );
         if ( ! ( e->state() & ControlButton ) )
            setSelected( itemAt( vp ), false );
      }

      QListView::contentsMousePressEvent( e );
   }
   // Store list of selected items at mouse-press time.
   // This is used when autoscrolling (why?)
   // and during dnd (the target item is temporarily selected)
   selectedItems( m_selected );
}

void KonqBaseListViewWidget::contentsMouseReleaseEvent( QMouseEvent *e )
{
   if ( m_rubber )
   {
      drawRubber();
      delete m_rubber;
      m_rubber = 0;
   }

   if ( m_scrollTimer )
   {
      disconnect( m_scrollTimer, SIGNAL( timeout() ),
                  this, SLOT( slotAutoScroll() ) );
      m_scrollTimer->stop();
      delete m_scrollTimer;
      m_scrollTimer = 0;
   }

   delete m_selected; m_selected = 0;
   KListView::contentsMouseReleaseEvent( e );
}

void KonqBaseListViewWidget::contentsMouseMoveEvent( QMouseEvent *e )
{
   if ( m_rubber )
   {
      slotAutoScroll();
      return;
   }

   QPoint vp = contentsToViewport( e->pos() );
   KonqBaseListViewItem* item = isExecuteArea( vp ) ?
         static_cast<KonqBaseListViewItem*>( itemAt( vp ) ) : 0L;

   if ( item != m_activeItem )
   {
      if ( m_activeItem != 0 )
      {
         m_activeItem->setActive( false );
      }

      m_activeItem = item;

      if ( item )
      {
         item->setActive( true );
         emit m_pBrowserView->setStatusBarText( item->item()->getStatusBarInfo() );
         m_pBrowserView->emitMouseOver( item->item() );
      }
      else
      {
         reportSelectedItems();
      }
   }

   KListView::contentsMouseMoveEvent( e );
}

void KonqBaseListViewWidget::drawRubber()
{
   if ( !m_rubber )
      return;

   QPainter p;
   p.begin( viewport() );
   p.setRasterOp( NotROP );
   p.setPen( QPen( color0, 1 ) );
   p.setBrush( NoBrush );

   QPoint pt( m_rubber->x(), m_rubber->y() );
   pt = contentsToViewport( pt );
   style().drawPrimitive( QStyle::PE_FocusRect, &p,
                          QRect( pt.x(), pt.y(), m_rubber->width(), m_rubber->height() ),
                          colorGroup(), QStyle::Style_Default, colorGroup().base() );
   p.end();
}

void KonqBaseListViewWidget::slotAutoScroll()
{
   if ( !m_rubber )
      return;

   // this code assumes that all items have the same height

   const QPoint pos = viewport()->mapFromGlobal( QCursor::pos() );
   const QPoint vc = viewportToContents( pos );

   if ( vc == m_rubber->bottomRight() )
      return;

   const int oldTop = m_rubber->normalize().top();
   const int oldBottom = m_rubber->normalize().bottom();

   drawRubber();
   m_rubber->setRight( vc.x() );
   m_rubber->setBottom( vc.y() );

   QListViewItem *cur = itemAt( QPoint(0,0) );

   bool block = signalsBlocked();
   blockSignals( true );

   if ( cur )
   {
      QRect rect = itemRect( cur );
      rect = QRect( viewportToContents( rect.topLeft() ),
                    viewportToContents( rect.bottomRight() ) );

      int offset = 0;
      if ( !allColumnsShowFocus() )
      {
         int hpos = header()->mapToIndex( 0 );
         for ( int index = 0; index < hpos; index++ )
            offset += columnWidth( header()->mapToSection( index ) );

         rect.setLeft( offset );
         rect.setRight( offset + columnWidth( 0 ) );
      }
      else
      {
         for ( int index = 0; index < columns(); index++ )
            offset += columnWidth( header()->mapToSection( index ) );

         rect.setLeft( 0 );
         rect.setRight( offset );
      }

      QRect r = rect;
      QListViewItem *tmp = cur;

      while ( cur && rect.top() <= oldBottom )
      {
         if ( rect.intersects( m_rubber->normalize() ) )
         {
            if ( !cur->isSelected() && cur->isSelectable() )
               setSelected( cur, true );
         } else if ( !m_selected || !m_selected->contains( (KonqBaseListViewItem*)cur ) )
            setSelected( cur, false );

         cur = cur->itemBelow();
         rect.moveBy( 0, rect.height() );
      }

      rect = r;
      rect.moveBy( 0, -rect.height() );
      cur = tmp->itemAbove();

      while ( cur && rect.bottom() >= oldTop )
      {
         if ( rect.intersects( m_rubber->normalize() ) )
         {
            if ( !cur->isSelected() && cur->isSelectable() )
               setSelected( cur, true );
         } else if ( !m_selected || !m_selected->contains( (KonqBaseListViewItem*)cur ) )
            setSelected( cur, false );

         cur = cur->itemAbove();
         rect.moveBy( 0, -rect.height() );
      }
   }

   blockSignals( block );
   emit selectionChanged();

   drawRubber();

   const int scroll_margin = 40;
   ensureVisible( vc.x(), vc.y(), scroll_margin, scroll_margin );

   if ( !QRect( scroll_margin, scroll_margin,
                viewport()->width() - 2*scroll_margin,
                viewport()->height() - 2*scroll_margin ).contains( pos ) )
   {
      if ( !m_scrollTimer )
      {
         m_scrollTimer = new QTimer( this );

         connect( m_scrollTimer, SIGNAL( timeout() ),
                  this, SLOT( slotAutoScroll() ) );
         m_scrollTimer->start( 100, false );
      }
   }
   else if ( m_scrollTimer )
   {
      disconnect( m_scrollTimer, SIGNAL( timeout() ),
                  this, SLOT( slotAutoScroll() ) );
      m_scrollTimer->stop();
      delete m_scrollTimer;
      m_scrollTimer = 0;
   }
}

void KonqBaseListViewWidget::viewportPaintEvent( QPaintEvent *e )
{
   drawRubber();
   KListView::viewportPaintEvent( e );
   drawRubber();
}

void KonqBaseListViewWidget::viewportResizeEvent(QResizeEvent * e)
{
   KListView::viewportResizeEvent(e);
   emit viewportAdjusted();
}

void KonqBaseListViewWidget::viewportDragMoveEvent( QDragMoveEvent *_ev )
{
   KonqBaseListViewItem *item =
       isExecuteArea( _ev->pos() ) ? (KonqBaseListViewItem*)itemAt( _ev->pos() ) : 0L;

   // Unselect previous drag-over-item
   if ( m_dragOverItem && m_dragOverItem != item )
       if ( !m_selected || !m_selected->contains( m_dragOverItem ) )
           setSelected( m_dragOverItem, false );

   if ( !item )
   {
      _ev->acceptAction();
      return;
   }

   if ( item->item()->acceptsDrops() )
   {
      _ev->acceptAction();
      if ( m_dragOverItem != item )
      {
	setSelected( item, true );
	m_dragOverItem = item;
      }
   }
   else
   {
      _ev->ignore();
      m_dragOverItem = 0L;
   }
}

void KonqBaseListViewWidget::viewportDragEnterEvent( QDragEnterEvent *_ev )
{
   m_dragOverItem = 0L;

   // By default we accept any format
   _ev->acceptAction();
}

void KonqBaseListViewWidget::viewportDragLeaveEvent( QDragLeaveEvent * )
{
   if ( m_dragOverItem != 0L )
      setSelected( m_dragOverItem, false );
   m_dragOverItem = 0L;
}

void KonqBaseListViewWidget::viewportDropEvent( QDropEvent *ev  )
{
   if ( m_dirLister->url().isEmpty() )
      return;
   kdDebug() << "KonqBaseListViewWidget::viewportDropEvent" << endl;
   if ( m_dragOverItem != 0L )
      setSelected( m_dragOverItem, false );
   m_dragOverItem = 0L;

   ev->accept();

   // We dropped on an item only if we dropped on the Name column.
   KonqBaseListViewItem *item =
       isExecuteArea( ev->pos() ) ? (KonqBaseListViewItem*)itemAt( ev->pos() ) : 0;

   KFileItem * destItem = (item) ? item->item() : m_dirLister->rootItem();
   KURL u = destItem ? destItem->url() : url();
   if ( u.isEmpty() )
      return;
   KonqOperations::doDrop( destItem /*may be 0L*/, u, ev, this );
}

void KonqBaseListViewWidget::startDrag()
{
   KURL::List urls = selectedUrls();
   // Multiple URLs ?

   QListViewItem * m_pressedItem = currentItem();

   QPixmap pixmap2;
   bool pixmap0Invalid = !m_pressedItem->pixmap(0) || m_pressedItem->pixmap(0)->isNull();

   if (( urls.count() > 1 ) || (pixmap0Invalid))
   {
      int iconSize = m_pBrowserView->m_pProps->iconSize();
      iconSize = iconSize ? iconSize : KGlobal::iconLoader()->currentSize( KIcon::Small ); // Default = small
      pixmap2 = DesktopIcon( "kmultiple", iconSize );
      if ( pixmap2.isNull() )
          kdWarning(1202) << "Could not find multiple pixmap" << endl;
   }

   KURLDrag *d = new KURLDrag( urls, viewport() );
   if ( !pixmap2.isNull())
   {
      d->setPixmap( pixmap2 );
   }
   else if (!pixmap0Invalid)
   {
      d->setPixmap( *m_pressedItem->pixmap( 0 ) );
   }
   d->drag();
}

void KonqBaseListViewWidget::slotItemRenamed(QListViewItem* item, const QString &name, int col)
{
   Q_ASSERT( col == 0 );
   Q_ASSERT( item != 0 );

   // The correct behavior is to show the old name until the rename has successfully
   // completed. Unfortunately, KListView forces us to allow the text to be changed
   // before we try the rename, so set it back to the pre-rename state.
   KonqBaseListViewItem *renamedItem = static_cast<KonqBaseListViewItem*>(item);
   renamedItem->updateContents();

   // Don't do anything if the user renamed to a blank name.
   if( !name.isEmpty() )
   {
      // Actually attempt the rename. If it succeeds, KDirLister will update the name.
      KonqOperations::rename( this, renamedItem->item()->url(), KIO::encodeFileName( name ) );
   }

   // When the KListViewLineEdit loses focus, focus tends to go to the location bar...
   setFocus();
}

void KonqBaseListViewWidget::reportSelectedItems()
{
   KFileItemList lst = selectedFileItems();
   m_pBrowserView->emitCounts( lst, false );
   m_pBrowserView->emitMouseOver( 0 );
}

void KonqBaseListViewWidget::slotMouseButtonClicked(int _button, QListViewItem* _item, const QPoint& pos, int)
{
   if ( _button == MidButton )
   {
      if(_item && isExecuteArea( viewport()->mapFromGlobal(pos) ) )
         m_pBrowserView->mmbClicked( static_cast<KonqBaseListViewItem*>(_item)->item() );
      else // MMB on background
         m_pBrowserView->mmbClicked( 0L );
   }
}

void KonqBaseListViewWidget::slotExecuted( QListViewItem* item )
{
  if ( !item )
      return;
  // isExecuteArea() checks whether the mouse pointer is
  // over an area where an action should be triggered
  // (i.e. the Name column, including pixmap and "+")
  if ( isExecuteArea( viewport()->mapFromGlobal(QCursor::pos())) )
  {
    slotReturnPressed( item );
  }
}

void KonqBaseListViewWidget::selectedItems( QPtrList<KonqBaseListViewItem>* _list )
{
   iterator it = begin();
   for( ; it != end(); it++ )
      if ( it->isSelected() )
         _list->append( &*it );
}

KFileItemList KonqBaseListViewWidget::selectedFileItems()
{
   KFileItemList list;
   iterator it = begin();
   for( ; it != end(); it++ )
      if ( it->isSelected() )
         list.append( it->item() );
   return list;
}

KURL::List KonqBaseListViewWidget::selectedUrls()
{
   KURL::List list;
   iterator it = begin();
   for( ; it != end(); it++ )
      if ( it->isSelected() )
         list.append( it->item()->url() );
   return list;
}

KonqPropsView * KonqBaseListViewWidget::props() const
{
   return m_pBrowserView->m_pProps;
}

void KonqBaseListViewWidget::slotReturnPressed( QListViewItem *_item )
{
   if ( !_item )
      return;
   KFileItem *fileItem = static_cast<KonqBaseListViewItem*>(_item)->item();
   if ( !fileItem )
      return;

   KURL url = fileItem->url();
   url.cleanPath();
   bool isIntoTrash =  url.isLocalFile() && url.path(1).startsWith(KGlobalSettings::trashPath());
   if ( !isIntoTrash || (isIntoTrash && fileItem->isDir()) )
   {
       m_pBrowserView->lmbClicked( fileItem );
   }
   else
   {
       KMessageBox::information(0L, i18n("You must take the file out of the trash before being able to use it."));
    }
}

void KonqBaseListViewWidget::slotPopupMenu(QListViewItem*, const QPoint &point, int )
{
   kdDebug() << "KonqBaseListViewWidget::slotPopupMenu" << endl;
   popupMenu( point,true );
}

void KonqBaseListViewWidget::popupMenu( const QPoint& _global, bool alwaysForSelectedFiles )
{
   KFileItemList lstItems;
   KParts::BrowserExtension::PopupFlags popupFlags = KParts::BrowserExtension::DefaultPopupItems;

   // Only consider a right-click on the name column as something
   // related to the selection. On all the other columns, we want
   // a popup for the current dir instead.
   if ((alwaysForSelectedFiles) || (isExecuteArea( viewport()->mapFromGlobal( _global ) )))
   {
       QPtrList<KonqBaseListViewItem> items;
       selectedItems( &items );
       for(KonqBaseListViewItem *item = items.first(); item; item = items.next())
          lstItems.append( item->item() );
   }

   KFileItem * rootItem = 0L;
   bool deleteRootItem = false;
   if ( lstItems.count() == 0 ) // emit popup for background
   {
      clearSelection();

      if ( m_dirLister->url().isEmpty() )
         return;
      rootItem = m_dirLister->rootItem();
      if ( !rootItem )
      {
         if ( url().isEmpty() )
            return;
         // Maybe we want to do a stat to get full info about the root item
         // (when we use permissions). For now create a dummy one.
         rootItem = new KFileItem( S_IFDIR, (mode_t)-1, url() );
         deleteRootItem = true;
      }

      lstItems.append( rootItem );
      popupFlags = KParts::BrowserExtension::ShowNavigationItems | KParts::BrowserExtension::ShowUp;
   }
   emit m_pBrowserView->extension()->popupMenu( 0L, _global, lstItems, KParts::URLArgs(), popupFlags );

   if ( deleteRootItem )
      delete rootItem; // we just created it
}

void KonqBaseListViewWidget::updateListContents()
{
   for (KonqBaseListViewWidget::iterator it = begin(); it != end(); it++ )
      it->updateContents();
}

bool KonqBaseListViewWidget::openURL( const KURL &url )
{
   kdDebug(1202) << k_funcinfo << "protocol: " << url.protocol()
                               <<" url: " << url.path() << endl;

   // The first time or new protocol? So create the columns first.
   if ( columns() < 1 || url.protocol() != m_url.protocol() )
   {
      readProtocolConfig( url.protocol() );
      createColumns();
   }

   m_bTopLevelComplete = false;
   m_itemFound = false;

   if ( m_itemToGoTo.isEmpty() && url.equals( m_url.upURL(), true ) )
      m_itemToGoTo = m_url.fileName( true );

   // Check for new properties in the new dir
   // newProps returns true the first time, and any time something might
   // have changed.
   bool newProps = m_pBrowserView->m_pProps->enterDir( url );

   m_dirLister->setNameFilter( m_pBrowserView->nameFilter() );
   m_dirLister->setMimeFilter( m_pBrowserView->mimeFilter() );
   m_dirLister->setShowingDotFiles( m_pBrowserView->m_pProps->isShowingDotFiles() );

   if ( m_pBrowserView->extension()->urlArgs().reload )
   {
      KParts::URLArgs args = m_pBrowserView->extension()->urlArgs();
      args.xOffset = contentsX();
      args.yOffset = contentsY();
      m_pBrowserView->extension()->setURLArgs( args );

      if ( currentItem() && itemRect( currentItem() ).isValid() )
         m_itemToGoTo = currentItem()->text(0);
   }

   if ( columnWidthMode(0) == Maximum )
      setColumnWidth(0,50);

   m_url = url;
   m_bUpdateContentsPosAfterListing = true;

   // Start the directory lister !
   m_dirLister->openURL( url, false /* new url */, m_pBrowserView->extension()->urlArgs().reload );

   // Apply properties and reflect them on the actions
   // do it after starting the dir lister to avoid changing the properties
   // of the old view
   if ( newProps )
   {
      m_pBrowserView->newIconSize( m_pBrowserView->m_pProps->iconSize() );
      m_pBrowserView->m_paShowDot->setChecked( m_pBrowserView->m_pProps->isShowingDotFiles() );
      m_pBrowserView->m_paCaseInsensitive->setChecked( m_pBrowserView->m_pProps->isCaseInsensitiveSort() );

      // It has to be "viewport()" - this is what KonqDirPart's slots act upon,
      // and otherwise we get a color/pixmap in the square between the scrollbars.
      m_pBrowserView->m_pProps->applyColors( viewport() );
   }

   return true;
}

void KonqBaseListViewWidget::setComplete()
{
   kdDebug(1202) << k_funcinfo << "Update Contents Pos: "
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

      emit selectionChanged();
   }

   m_itemToGoTo = "";
   m_restored = false;

   // Show totals
   reportSelectedItems();

   if ( !isUpdatesEnabled() || !viewport()->isUpdatesEnabled() )
   {
      viewport()->setUpdatesEnabled( true );
      setUpdatesEnabled( true );
      triggerUpdate();
   }

   // Show "cut" icons as such
   m_pBrowserView->slotClipboardDataChanged();
}

void KonqBaseListViewWidget::slotStarted()
{
   //kdDebug(1202) << k_funcinfo << endl;

   if (!m_bTopLevelComplete)
      emit m_pBrowserView->started( 0 );
}

void KonqBaseListViewWidget::slotCompleted()
{
   //kdDebug(1202) << k_funcinfo << endl;

   setComplete();
   if ( m_bTopLevelComplete )
       emit m_pBrowserView->completed();
   m_pBrowserView->listingComplete();
}

void KonqBaseListViewWidget::slotCanceled()
{
   //kdDebug(1202) << k_funcinfo << endl;

   setComplete();
   emit m_pBrowserView->canceled( QString::null );
}

void KonqBaseListViewWidget::slotClear()
{
   kdDebug(1202) << k_funcinfo << endl;

   m_activeItem = 0;
   delete m_selected; m_selected = 0;
   m_pBrowserView->resetCount();
   m_pBrowserView->lstPendingMimeIconItems().clear();

   viewport()->setUpdatesEnabled( false );
   setUpdatesEnabled( false );
   clear();
}

void KonqBaseListViewWidget::slotNewItems( const KFileItemList & entries )
{
   //kdDebug(1202) << k_funcinfo << entries.count() << endl;

   for ( QPtrListIterator<KFileItem> kit ( entries ); kit.current(); ++kit )
   {
      KonqListViewItem * tmp = new KonqListViewItem( this, *kit );
      if ( !m_itemFound && tmp->text(0) == m_itemToGoTo )
      {
         setCurrentItem( tmp );
         m_itemFound = true;
      }

      if ( !(*kit)->isMimeTypeKnown() )
          m_pBrowserView->lstPendingMimeIconItems().append( tmp );
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

void KonqBaseListViewWidget::slotDeleteItem( KFileItem * _fileitem )
{
  kdDebug(1202) << k_funcinfo << "removing " << _fileitem->url().url() << " from tree!" << endl;

  m_pBrowserView->deleteItem( _fileitem );
  iterator it = begin();
  for( ; it != end(); ++it )
    if ( (*it).item() == _fileitem )
    {
      m_pBrowserView->lstPendingMimeIconItems().remove( &(*it) );

      delete &(*it);
      // HACK HACK HACK: QListViewItem/KonqBaseListViewItem should
      // take care and the source looks like it does; till the
      // real bug is found, this fixes some crashes (malte)
      emit selectionChanged();
      return;
    }

  if ( !viewport()->isUpdatesEnabled() )
  {
    viewport()->setUpdatesEnabled( true );
    setUpdatesEnabled( true );
    triggerUpdate();
  }
  slotUpdateBackground();
}

void KonqBaseListViewWidget::slotRefreshItems( const KFileItemList & entries )
{
   QPtrListIterator<KFileItem> kit ( entries );
   for( ; kit.current(); ++kit )
   {
      iterator it = begin();
      for( ; it != end(); ++it )
         if ( (*it).item() == kit.current() )
         {
            it->updateContents();
            break;
         }
   }
}

void KonqBaseListViewWidget::slotRedirection( const KURL & url )
{
   kdDebug(1202) << k_funcinfo << endl;

   if (( columns() <1) || ( url.protocol() != m_url.protocol() ))
   {
      readProtocolConfig( url.protocol() );
      createColumns();
   }
   emit m_pBrowserView->extension()->setLocationBarURL( url.prettyURL() );
   m_pBrowserView->m_url=url;
   m_url = url;
}

KonqBaseListViewWidget::iterator& KonqBaseListViewWidget::iterator::operator++()
{
   if ( !m_p ) return *this;
   KonqBaseListViewItem *i = (KonqBaseListViewItem*)m_p->firstChild();
   if ( i )
   {
      m_p = i;
      return *this;
   }
   i = (KonqBaseListViewItem*)m_p->nextSibling();
   if ( i )
   {
      m_p = i;
      return *this;
   }
   m_p = (KonqBaseListViewItem*)m_p->parent();

   while ( m_p )
   {
      if ( m_p->nextSibling() )
         break;
      m_p = (KonqBaseListViewItem*)m_p->parent();
   }

   if ( m_p )
      m_p = (KonqBaseListViewItem*)m_p->nextSibling();

   return *this;
}

KonqBaseListViewWidget::iterator KonqBaseListViewWidget::iterator::operator++(int)
{
   KonqBaseListViewWidget::iterator it = *this;
   if ( !m_p ) return it;
   KonqBaseListViewItem *i = (KonqBaseListViewItem*)m_p->firstChild();
   if (i)
   {
      m_p = i;
      return it;
   }
   i = (KonqBaseListViewItem*)m_p->nextSibling();
   if (i)
   {
      m_p = i;
      return it;
   }
   m_p = (KonqBaseListViewItem*)m_p->parent();

   while ( m_p )
   {
      if ( m_p->nextSibling() )
         break;
      m_p = (KonqBaseListViewItem*)m_p->parent();
   }

   if (m_p)
      m_p = (KonqBaseListViewItem*)m_p->nextSibling();
   return it;
}

void KonqBaseListViewWidget::paintEmptyArea( QPainter *p, const QRect &r )
{
   const QPixmap *pm = viewport()->paletteBackgroundPixmap();

   if (!pm || pm->isNull())
      p->fillRect(r, viewport()->backgroundColor());
   else
   {
       QRect devRect = p->xForm( r );
       int ax = (devRect.x() + contentsX());
       int ay = (devRect.y() + contentsY());
       /* kdDebug() << "KonqBaseListViewWidget::paintEmptyArea "
                  << r.x() << "," << r.y() << " " << r.width() << "x" << r.height()
                  << " drawing pixmap with offset " << ax << "," << ay
                  << endl;*/
       p->drawTiledPixmap(r, *pm, QPoint(ax, ay));
   }
}

void KonqBaseListViewWidget::disableIcons( const KURL::List & lst )
{
   iterator kit = begin();
   for( ; kit != end(); ++kit )
   {
      bool bFound = false;
      // Wow. This is ugly. Matching two lists together....
      // Some sorting to optimise this would be a good idea ?
      for (KURL::List::ConstIterator it = lst.begin(); !bFound && it != lst.end(); ++it)
      {
         if ( (*kit).item()->url() == *it ) // *it is encoded already
         {
            bFound = true;
            // maybe remove "it" from lst here ?
         }
      }
      (*kit).setDisabled( bFound );
   }
}

void KonqBaseListViewWidget::saveState( QDataStream & ds )
{
   QString str;
   if ( currentItem() )
      str = static_cast<KonqBaseListViewItem*>(currentItem())->item()->url().fileName(true);
   ds << str;
}

void KonqBaseListViewWidget::restoreState( QDataStream & ds )
{
   m_restored = true;

   QString str;
   ds >> str;
   if ( !str.isEmpty() )
      m_itemToGoTo = str;
}

void KonqBaseListViewWidget::slotUpdateBackground()
{
   if ( viewport()->paletteBackgroundPixmap() && !viewport()->paletteBackgroundPixmap()->isNull() )
   {
      if ( !m_backgroundTimer )
      {
         m_backgroundTimer = new QTimer( this );
         connect( m_backgroundTimer, SIGNAL( timeout() ), viewport(), SLOT( update() ) );
      }
      else
         m_backgroundTimer->stop();

      m_backgroundTimer->start( 50, true );
   }
}

#include "konq_listviewwidget.moc"
