/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>
                 2001, 2002, 2004 Michael Brade <brade@kde.org>

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
#include "konq_listviewsettings.h"
#include "konq_listviewwidget.h"
#include <konq_filetip.h>
#include <konq_settings.h>

#include <kdebug.h>
#include <kdirlister.h>
#include <kiconloader.h>
#include <klocale.h>
#include <kprotocolinfo.h>
#include <kmessagebox.h>
#include <ktoggleaction.h>

#include <q3header.h>
#include <QPainter>
#include <QStyle>
#include <QTimer>
#include <QEvent>
#include <QCursor>
#include <QToolTip>
#include <qdrag.h>

#include <stdlib.h>
#include <assert.h>
#include <konqmimedata.h>

ColumnInfo::ColumnInfo()
   :displayInColumn(-1)
   ,name()
   ,desktopFileName()
   ,udsId(0)
   ,type(QVariant::Invalid)
   ,displayThisOne(false)
   ,toggleThisOne(0)
{}


void ColumnInfo::setData(const QString& n, const QString& desktopName, int kioUds,
                         KToggleAction* someAction, int theWidth)
{
   displayInColumn=-1;
   name=n;
   desktopFileName=desktopName;
   udsId=kioUds;
   type=QVariant::Invalid;
   displayThisOne=false;
   toggleThisOne=someAction;
   width=theWidth;
}

void ColumnInfo::setData(const QString& n, const QString& desktopName, int kioUds,
                         QVariant::Type t, KToggleAction* someAction, int theWidth)
{
   displayInColumn=-1;
   name=n;
   desktopFileName=desktopName;
   udsId=kioUds;
   type=t;
   displayThisOne=false;
   toggleThisOne=someAction;
   width=theWidth;
}


KonqBaseListViewWidget::KonqBaseListViewWidget( KonqListView *parent, QWidget *parentWidget)
   : K3ListView(parentWidget)
   ,sortedByColumn(0)
   ,m_pBrowserView(parent)
   ,m_dirLister(new KDirLister( true /*m_showIcons==false*/))
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
   ,m_fileTip(new KonqFileTip(this))
{
   kDebug(1202) << "+KonqBaseListViewWidget" << endl;

   m_dirLister->setMainWindow(topLevelWidget());

   m_bTopLevelComplete  = true;

   //Adjust K3ListView behaviour
   setMultiSelection(true);
   setSelectionModeExt( FileManager );
   setDragEnabled(true);
   setItemsMovable(false);

   initConfig();
#if 0
   connect( this, SIGNAL(rightButtonPressed(Q3ListViewItem*,const QPoint&,int)),
            this, SLOT(slotRightButtonPressed(Q3ListViewItem*,const QPoint&,int)));
#endif
   connect( this, SIGNAL(returnPressed( Q3ListViewItem * )),
            this, SLOT(slotReturnPressed( Q3ListViewItem * )) );
   connect( this, SIGNAL( mouseButtonClicked( int, Q3ListViewItem *, const QPoint&, int )),
            this, SLOT( slotMouseButtonClicked( int, Q3ListViewItem *, const QPoint&, int )) );
   connect( this, SIGNAL(executed( Q3ListViewItem * )),
            this, SLOT(slotExecuted( Q3ListViewItem * )) );

   connect( this, SIGNAL(currentChanged( Q3ListViewItem * )),
            this, SLOT(slotCurrentChanged( Q3ListViewItem * )) );
   connect( this, SIGNAL(itemRenamed( Q3ListViewItem *, const QString &, int )),
            this, SLOT(slotItemRenamed( Q3ListViewItem *, const QString &, int )) );
   connect( this, SIGNAL(contextMenuRequested( Q3ListViewItem *, const QPoint&, int )),
            this, SLOT(slotPopupMenu( Q3ListViewItem *, const QPoint&, int )) );
   connect( this, SIGNAL(selectionChanged()), this, SLOT(slotSelectionChanged()) );

   connect( horizontalScrollBar(), SIGNAL(valueChanged( int )),
            this, SIGNAL(viewportAdjusted()) );
   connect( verticalScrollBar(), SIGNAL(valueChanged( int )),
            this, SIGNAL(viewportAdjusted()) );

   // Connect the directory lister
   connect( m_dirLister, SIGNAL(started( const KUrl & )),
            this, SLOT(slotStarted()) );
   connect( m_dirLister, SIGNAL(completed()), this, SLOT(slotCompleted()) );
   connect( m_dirLister, SIGNAL(canceled()), this, SLOT(slotCanceled()) );
   connect( m_dirLister, SIGNAL(clear()), this, SLOT(slotClear()) );
   connect( m_dirLister, SIGNAL(newItems( const KFileItemList & ) ),
            this, SLOT(slotNewItems( const KFileItemList & )) );
   connect( m_dirLister, SIGNAL(deleteItem( KFileItem * )),
            this, SLOT(slotDeleteItem( KFileItem * )) );
   connect( m_dirLister, SIGNAL(refreshItems( const KFileItemList & )),
            this, SLOT( slotRefreshItems( const KFileItemList & )) );
   connect( m_dirLister, SIGNAL(redirection( const KUrl & )),
            this, SLOT(slotRedirection( const KUrl & )) );
   connect( m_dirLister, SIGNAL(itemsFilteredByMime( const KFileItemList & )),
            m_pBrowserView, SIGNAL(itemsFilteredByMime( const KFileItemList & )) );

   connect( m_dirLister, SIGNAL(infoMessage( const QString& )),
            m_pBrowserView->extension(), SIGNAL(infoMessage( const QString& )) );
   connect( m_dirLister, SIGNAL(percent( int )),
            m_pBrowserView->extension(), SIGNAL(loadingProgress( int )) );
   connect( m_dirLister, SIGNAL(speed( int )),
            m_pBrowserView->extension(), SIGNAL(speedProgress( int )) );

   connect( header(), SIGNAL(sizeChange( int, int, int )), SLOT(slotUpdateBackground()) );

   viewport()->setMouseTracking( true );
   viewport()->setFocusPolicy( Qt::WheelFocus );
   setFocusPolicy( Qt::WheelFocus );
   setAcceptDrops( true );

   //looks better with the statusbar
   setFrameStyle( QFrame::StyledPanel | QFrame::Sunken );
   setShowSortIndicator( true );
}

KonqBaseListViewWidget::~KonqBaseListViewWidget()
{
   kDebug(1202) << "-KonqBaseListViewWidget" << endl;

   delete m_selected; m_selected = 0;

   // TODO: this is a hack, better fix the connections of m_dirLister if possible!
   m_dirLister->disconnect( this );
   delete m_dirLister;

   delete m_fileTip;
}

void KonqBaseListViewWidget::readProtocolConfig( const KUrl & url )
{
   const QString protocol = url.protocol();
   KonqListViewSettings config( protocol );
   config.readConfig();
   sortedByColumn = config.sortBy();
   m_bAscending = config.sortOrder();

   m_filenameColumnWidth = config.fileNameColumnWidth();

   QStringList lstColumns = config.columns();
   QList<int> lstColumnWidths = config.columnWidths();
   if (lstColumns.isEmpty())
   {
      // Default column selection
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
   KProtocolInfo::ExtraFieldList extraFields = KProtocolInfo::extraFields(url);
   NumberOfAtoms += extraFields.count();
   confColumns.resize( NumberOfAtoms );

   KProtocolInfo::ExtraFieldList::Iterator extraFieldsIt = extraFields.begin();
   for ( int num = 1; extraFieldsIt != extraFields.end(); ++extraFieldsIt, ++num )
   {
      const QString column = (*extraFieldsIt).name;
      if ( !lstColumns.contains(column) )
         lstColumns << column;
      const QVariant::Type type = static_cast<QVariant::Type>( (*extraFieldsIt).type ); // ## TODO use when sorting
      confColumns[extraIndex++].setData( column, QString("Extra%1").arg(num), KIO::UDS_EXTRA, type, 0);
   }

   //disable everything
   for ( unsigned int i = 0; i < NumberOfAtoms; i++ )
   {
      confColumns[i].displayThisOne = false;
      confColumns[i].displayInColumn = -1;
      if ( confColumns[i].toggleThisOne )
      {
          confColumns[i].toggleThisOne->setChecked( false );
          confColumns[i].toggleThisOne->setEnabled( true );
      }
   }
   int currentColumn = m_filenameColumn + 1;
   //check all columns in lstColumns
   for ( int i = 0; i < lstColumns.count(); i++ )
   {
      //search the column in confColumns
      for ( unsigned int j = 0; j < NumberOfAtoms; j++ )
      {
         if ( confColumns[j].name == lstColumns.at(i) )
         {
            confColumns[j].displayThisOne = true;
            confColumns[j].displayInColumn = currentColumn;
            if ( confColumns[j].toggleThisOne )
               confColumns[j].toggleThisOne->setChecked( true );
            currentColumn++;

            if ( i < lstColumnWidths.count() )
               confColumns[j].width = lstColumnWidths.at(i);
            else
            {
               // Default Column widths
               ColumnInfo *tmpColumn = &confColumns[j];
               QString str;

               if ( tmpColumn->udsId == KIO::UDS_SIZE )
                  str = KGlobal::locale()->formatNumber( 888888888, 0 ) + "  ";
               else if ( tmpColumn->udsId == KIO::UDS_ACCESS )
                  str = "--Permissions--";
               else if ( tmpColumn->udsId == KIO::UDS_USER )
                  str = "a_long_username";
               else if ( tmpColumn->udsId == KIO::UDS_GROUP )
                  str = "a_groupname";
               else if ( tmpColumn->udsId == KIO::UDS_LINK_DEST )
                  str = "a_quite_long_filename_for_link_dest";
               else if ( tmpColumn->udsId == KIO::UDS_FILE_TYPE )
                  str = "a_long_comment_for_mimetype";
               else if ( tmpColumn->udsId == KIO::UDS_MIME_TYPE )
                  str = "_a_long_/_mimetype_";
               else if ( tmpColumn->udsId == KIO::UDS_URL )
                  str = "a_long_lonq_long_very_long_url";
               else if ( (tmpColumn->udsId & KIO::UDS_TIME)
                         || (tmpColumn->udsId == KIO::UDS_EXTRA &&
                             (tmpColumn->type & QVariant::DateTime)) )
               {
                  QDateTime dt( QDate( 2000, 10, 10 ), QTime( 20, 20, 20 ) );
                  str = KGlobal::locale()->formatDateTime( dt ) + "--";
               }
               else
                  str = "it_is_the_default_width";

               confColumns[j].width = fontMetrics().width(str);
            }
            break;
         }
      }
   }
   //check what the protocol provides
   QStringList listingList = KProtocolInfo::listing( url );
   kDebug(1202) << k_funcinfo << "protocol: " << protocol << endl;

   // Even if this is not given by the protocol, we can determine it.
   // Please don't remove this ;-). It makes it possible to show the file type
   // using the mimetype comment, which for most users is a nicer alternative
   // than the raw mimetype name.
   listingList.append( "MimeType" );
   for ( unsigned int i = 0; i < NumberOfAtoms; i++ )
   {
      if ( confColumns[i].udsId == KIO::UDS_URL ||
           confColumns[i].udsId == KIO::UDS_MIME_TYPE ||
           !confColumns[i].displayThisOne )
      {
         continue;
      }

      if ( !listingList.contains( confColumns[i].desktopFileName ) ) // not found -> hide
      {
         //move all columns behind one to the front
         for ( unsigned int l = 0; l < NumberOfAtoms; l++ )
            if ( confColumns[l].displayInColumn > confColumns[i].displayInColumn )
               confColumns[l].displayInColumn--;

         //disable this column
         confColumns[i].displayThisOne = false;
         if ( confColumns[i].toggleThisOne )
         {
           confColumns[i].toggleThisOne->setEnabled( false );
           confColumns[i].toggleThisOne->setChecked( false );
         }
      }
   }
}

void KonqBaseListViewWidget::createColumns()
{
   //this column is always required, so add it
   if ( columns() < 1 )
       addColumn( i18n("Name"), m_filenameColumnWidth );
   setSorting( 0, true );

   //remove all columns that will be re-added
   for ( int i=columns()-1; i>m_filenameColumn; i--)
        removeColumn(i);

   //now add the checked columns
   int currentColumn = m_filenameColumn + 1;
   for ( int i = 0; i < (int)NumberOfAtoms; i++ )
   {
      if ( confColumns[i].displayThisOne && (confColumns[i].displayInColumn == currentColumn) )
      {
         addColumn( i18n(confColumns[i].name.toUtf8()), confColumns[i].width );
         if ( sortedByColumn == confColumns[i].desktopFileName )
            setSorting( currentColumn, m_bAscending );
         if ( confColumns[i].udsId == KIO::UDS_SIZE )
             setColumnAlignment( currentColumn, Qt::AlignRight );
         i = -1;
         currentColumn++;
      }
   }
   if ( sortedByColumn == "FileName" )
      setSorting( 0, m_bAscending );
}

void KonqBaseListViewWidget::stop()
{
   m_dirLister->stop();
}

const KUrl & KonqBaseListViewWidget::url()
{
   return m_url;
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

   bool on = m_pSettings->showFileTips();
   m_fileTip->setOptions( on, m_pSettings->showPreviewsInFileTips(), m_pSettings->numFileTips() );

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
   m_selected = new Q3PtrList<KonqBaseListViewItem>;

   QPoint vp = contentsToViewport( e->pos() );
   KonqBaseListViewItem* item = isExecuteArea( vp ) ?
         static_cast<KonqBaseListViewItem*>( itemAt( vp ) ) : 0L;

   if ( item )
      K3ListView::contentsMousePressEvent( e );
   else {
      if ( e->button() == Qt::LeftButton )
      {
         if ( !( e->modifiers() & Qt::ControlModifier ) )
            setSelected( itemAt( vp ), false );
         m_rubber = new QRect( e->x(), e->y(), 0, 0 );
         m_fileTip->setItem( 0 );
      }
      if ( e->button() != Qt::RightButton )
         Q3ListView::contentsMousePressEvent( e );
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
   K3ListView::contentsMouseReleaseEvent( e );
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
         static_cast<KonqBaseListViewItem *>( itemAt( vp ) ) : 0;

   if ( item != m_activeItem )
   {
      if ( m_activeItem != 0 )
         m_activeItem->setActive( false );

      m_activeItem = item;

      if ( item )
      {
         item->setActive( true );
         emit m_pBrowserView->setStatusBarText( item->item()->getStatusBarInfo() );
         m_pBrowserView->emitMouseOver( item->item() );

         vp.setY( itemRect( item ).y() );
         QRect rect( viewportToContents( vp ), QSize(20, item->height()) );
         m_fileTip->setItem( item->item(), rect, item->pixmap( 0 ) );
         m_fileTip->setPreview( KGlobalSettings::showFilePreview( item->item()->url() ) );
         setShowToolTips( !m_pSettings->showFileTips() );
      }
      else
      {
         reportItemCounts();
         m_pBrowserView->emitMouseOver( 0 );

         m_fileTip->setItem( 0 );
         setShowToolTips( true );
      }
   }

   K3ListView::contentsMouseMoveEvent( e );
}

void KonqBaseListViewWidget::contentsWheelEvent( QWheelEvent *e )
{
   // when scrolling with mousewheel, stop possible pending filetip
   m_fileTip->setItem( 0 );

   if ( m_activeItem != 0 )
   {
      m_activeItem->setActive( false );
      m_activeItem = 0;
   }

   reportItemCounts();
   m_pBrowserView->emitMouseOver( 0 );
   K3ListView::contentsWheelEvent( e );
}

void KonqBaseListViewWidget::leaveEvent( QEvent *e )
{
   if ( m_activeItem != 0 )
   {
      m_activeItem->setActive( false );
      m_activeItem = 0;
   }

   reportItemCounts();
   m_pBrowserView->emitMouseOver( 0 );

   m_fileTip->setItem( 0 );

   K3ListView::leaveEvent( e );
}

void KonqBaseListViewWidget::drawRubber()
{
   if ( !m_rubber )
      return;

   QPainter p;
   p.begin( viewport() );
   #warning FIXME NotROP is not available in qt4
	//Using black with .5 alpha for now
	//p.setRasterOp( NotROP );
   QColor c( 0, 0, 0, 127 );
   p.setPen( QPen( c, 1 ) );
   p.setBrush( QBrush() );

   QPoint pt( m_rubber->x(), m_rubber->y() );
   pt = contentsToViewport( pt );
#warning PE_FocusRect is gone, using PE_FrameFocusRect
   QStyleOptionFocusRect fr;
   fr.backgroundColor = c;
   fr.rect = QRect( pt.x(), pt.y(), m_rubber->width(), m_rubber->height() );
   fr.palette = palette();
   style()->drawPrimitive( QStyle::PE_FrameFocusRect, &fr, &p, viewport() );
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

   const int oldTop = m_rubber->normalized().top();
   const int oldBottom = m_rubber->normalized().bottom();

   drawRubber();
   m_rubber->setBottomRight( vc );

   Q3ListViewItem *cur = itemAt( QPoint(0,0) );

   bool block = signalsBlocked();
   blockSignals( true );

   QRect nr = m_rubber->normalized();
   if ( cur )
   {
      QRect rect = itemRect( cur );
      if ( !allColumnsShowFocus() )
          rect.setWidth( executeArea( cur ) );

      rect = QRect( viewportToContents( rect.topLeft() ),
                    viewportToContents( rect.bottomRight() ) );

      if ( !allColumnsShowFocus() )
      {
         rect.setLeft( header()->sectionPos( 0 ) );
         rect.setWidth( rect.width() );
      }
      else
      {
         rect.setLeft( 0 );
         rect.setWidth( header()->headerWidth() );
      }

      QRect r = rect;
      Q3ListViewItem *tmp = cur;

      while ( cur && rect.top() <= oldBottom )
      {
         if ( rect.intersects( nr ) )
         {
            if ( !cur->isSelected() && cur->isSelectable() )
               setSelected( cur, true );
         } else if ( !m_selected || !m_selected->contains( (KonqBaseListViewItem*)cur ) )
            setSelected( cur, false );

         cur = cur->itemBelow();
         if (cur && !allColumnsShowFocus())
            rect.setWidth( executeArea( cur ) );
         rect.translate( 0, rect.height() );
      }

      rect = r;
      rect.translate( 0, -rect.height() );
      cur = tmp->itemAbove();

      while ( cur && rect.bottom() >= oldTop )
      {
         if ( rect.intersects( nr ) )
         {
            if ( !cur->isSelected() && cur->isSelectable() )
               setSelected( cur, true );
         } else if ( !m_selected || !m_selected->contains( (KonqBaseListViewItem*)cur ) )
            setSelected( cur, false );

         cur = cur->itemAbove();
         if (cur && !allColumnsShowFocus())
            rect.setWidth( executeArea( cur ) );
         rect.translate( 0, -rect.height() );
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
         m_scrollTimer->setSingleShot( false );

         connect( m_scrollTimer, SIGNAL( timeout() ),
                  this, SLOT( slotAutoScroll() ) );
         m_scrollTimer->start( 100 );
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
   K3ListView::viewportPaintEvent( e );
   drawRubber();
}

void KonqBaseListViewWidget::viewportResizeEvent(QResizeEvent * e)
{
   K3ListView::viewportResizeEvent(e);
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
      m_dragOverItem = 0L;
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
   kDebug() << "KonqBaseListViewWidget::viewportDropEvent" << endl;
   if ( m_dragOverItem != 0L )
      setSelected( m_dragOverItem, false );
   m_dragOverItem = 0L;

   ev->accept();

   // We dropped on an item only if we dropped on the Name column.
   KonqBaseListViewItem *item =
       isExecuteArea( ev->pos() ) ? (KonqBaseListViewItem*)itemAt( ev->pos() ) : 0;

   KFileItem * destItem = (item) ? item->item() : m_dirLister->rootItem();
   KUrl u = destItem ? destItem->url() : url();
   if ( u.isEmpty() )
      return;
   KonqOperations::doDrop( destItem /*may be 0L*/, u, ev, this );
}

void KonqBaseListViewWidget::startDrag()
{
   m_fileTip->setItem( 0 );
   KUrl::List urls = selectedUrls( false );

   Q3ListViewItem * m_pressedItem = currentItem();

   QPixmap pixmap2;
   bool pixmap0Invalid = !m_pressedItem->pixmap(0) || m_pressedItem->pixmap(0)->isNull();

   // Multiple URLs ?
   if (( urls.count() > 1 ) || (pixmap0Invalid))
   {
      int iconSize = m_pBrowserView->m_pProps->iconSize();
      iconSize = iconSize ? iconSize : KGlobal::iconLoader()->currentSize( K3Icon::Small ); // Default = small
      pixmap2 = DesktopIcon( "kmultiple", iconSize );
      if ( pixmap2.isNull() )
          kWarning(1202) << "Could not find multiple pixmap" << endl;
   }

   QDrag* drag = new QDrag( viewport() );
   KonqMimeData::populateMimeData( drag->mimeData(), urls, selectedUrls(true) );

   if ( !pixmap2.isNull() )
      drag->setPixmap( pixmap2 );
   else if ( !pixmap0Invalid )
      drag->setPixmap( *m_pressedItem->pixmap( 0 ) );

   drag->start();
}

void KonqBaseListViewWidget::slotItemRenamed( Q3ListViewItem *item, const QString &name, int col )
{
   Q_ASSERT( col == 0 );
   Q_ASSERT( item != 0 );

   // The correct behavior is to show the old name until the rename has successfully
   // completed. Unfortunately, K3ListView forces us to allow the text to be changed
   // before we try the rename, so set it back to the pre-rename state.
   KonqBaseListViewItem *renamedItem = static_cast<KonqBaseListViewItem*>(item);
   renamedItem->updateContents();

   // Don't do anything if the user renamed to a blank name.
   if( !name.isEmpty() )
   {
      // Actually attempt the rename. If it succeeds, KDirLister will update the name.
      KonqOperations::rename( this, renamedItem->item()->url(), KIO::encodeFileName( name ) );
   }

   // When the K3ListViewLineEdit loses focus, focus tends to go to the location bar...
   setFocus();
}

void KonqBaseListViewWidget::reportItemCounts()
{
   KFileItemList lst = selectedFileItems();
   if ( !lst.isEmpty() )
      m_pBrowserView->emitCounts( lst );
   else
   {
      lst = visibleFileItems();
      m_pBrowserView->emitCounts( lst );
   }
}

void KonqBaseListViewWidget::slotSelectionChanged()
{
   reportItemCounts();

   KFileItemList lst = selectedFileItems();
   emit m_pBrowserView->m_extension->selectionInfo( lst );
}

void KonqBaseListViewWidget::slotMouseButtonClicked( int _button,
      Q3ListViewItem *_item, const QPoint& pos, int )
{
   if ( _button == Qt::MidButton )
   {
      if ( _item && isExecuteArea( viewport()->mapFromGlobal(pos) ) )
         m_pBrowserView->mmbClicked( static_cast<KonqBaseListViewItem *>(_item)->item() );
      else // MMB on background
         m_pBrowserView->mmbClicked( 0 );
   }
}

void KonqBaseListViewWidget::slotExecuted( Q3ListViewItem *item )
{
   if ( !item )
      return;
   m_fileTip->setItem( 0 );
   // isExecuteArea() checks whether the mouse pointer is
   // over an area where an action should be triggered
   // (i.e. the Name column, including pixmap and "+")
   if ( isExecuteArea( viewport()->mapFromGlobal( QCursor::pos() ) ) )
      slotReturnPressed( item );
}

void KonqBaseListViewWidget::selectedItems( Q3PtrList<KonqBaseListViewItem> *_list )
{
   iterator it = begin();
   for ( ; it != end(); it++ )
      if ( it->isSelected() )
         _list->append( &*it );
}

KFileItemList KonqBaseListViewWidget::visibleFileItems()
{
   KFileItemList list;
   KonqBaseListViewItem *item = static_cast<KonqBaseListViewItem *>(firstChild());
   while ( item )
   {
      list.append( item->item() );
      item = static_cast<KonqBaseListViewItem *>(item->itemBelow());
   }
   return list;
}

KFileItemList KonqBaseListViewWidget::selectedFileItems()
{
   KFileItemList list;
   iterator it = begin();
   for ( ; it != end(); it++ )
      if ( it->isSelected() )
         list.append( it->item() );
   return list;
}

KUrl::List KonqBaseListViewWidget::selectedUrls( bool mostLocal )
{
   bool dummy;
   KUrl::List list;
   iterator it = begin();
   for ( ; it != end(); it++ )
      if ( it->isSelected() )
         list.append( mostLocal ? it->item()->mostLocalURL( dummy ) : it->item()->url() );
   return list;
}

KonqPropsView * KonqBaseListViewWidget::props() const
{
   return m_pBrowserView->m_pProps;
}

void KonqBaseListViewWidget::slotReturnPressed( Q3ListViewItem *_item )
{
   if ( !_item )
      return;
   KFileItem *fileItem = static_cast<KonqBaseListViewItem *>(_item)->item();
   if ( !fileItem )
      return;

   KUrl url = fileItem->url();
   url.cleanPath();
#warning hardcoded protocol: find a better way to determine if a url is a trash url.
   bool isIntoTrash = url.protocol() == "trash";
   if ( !isIntoTrash || (isIntoTrash && fileItem->isDir()) )
      m_pBrowserView->lmbClicked( fileItem );
   else
      KMessageBox::information( 0, i18n("You must take the file out of the trash before being able to use it.") );
}

void KonqBaseListViewWidget::slotPopupMenu( Q3ListViewItem *i, const QPoint &point, int c )
{
   kDebug(1202) << "KonqBaseListViewWidget::slotPopupMenu" << endl;
   popupMenu( point, ( i != 0 && c == -1 ) ); // i != 0 && c == -1 when activated by keyboard
}

void KonqBaseListViewWidget::popupMenu( const QPoint& _global, bool alwaysForSelectedFiles )
{
   m_fileTip->setItem( 0 );

   KFileItemList lstItems;
   KParts::BrowserExtension::PopupFlags popupFlags = KParts::BrowserExtension::DefaultPopupItems;

   // Only consider a right-click on the name column as something
   // related to the selection. On all the other columns, we want
   // a popup for the current dir instead.
   if ( alwaysForSelectedFiles || isExecuteArea( viewport()->mapFromGlobal( _global ) ) )
   {
       Q3PtrList<KonqBaseListViewItem> items;
       selectedItems( &items );
       for ( KonqBaseListViewItem *item = items.first(); item; item = items.next() )
          lstItems.append( item->item() );
   }

   KFileItem *rootItem = 0L;
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
   emit m_pBrowserView->extension()->popupMenu( 0, _global, lstItems, KParts::URLArgs(), popupFlags );

   if ( deleteRootItem )
      delete rootItem; // we just created it
}

void KonqBaseListViewWidget::updateListContents()
{
   for ( KonqBaseListViewWidget::iterator it = begin(); it != end(); it++ )
      it->updateContents();
}

bool KonqBaseListViewWidget::openURL( const KUrl &url )
{
   kDebug(1202) << k_funcinfo << "protocol: " << url.protocol()
                               << " url: " << url.path() << endl;

   // The first time or new protocol? So create the columns first.
   if ( columns() < 1 || url.protocol() != m_url.protocol() )
   {
      readProtocolConfig( url );
      createColumns();
   }

   m_bTopLevelComplete = false;
   m_itemFound = false;

   if ( m_itemToGoTo.isEmpty() && url.equals( m_url.upURL(), KUrl::CompareWithoutTrailingSlash ) )
      m_itemToGoTo = m_url.fileName( KUrl::IgnoreTrailingSlash );

   // Check for new properties in the new dir
   // newProps returns true the first time, and any time something might
   // have changed.
   bool newProps = m_pBrowserView->m_pProps->enterDir( url );

   m_dirLister->setNameFilter( m_pBrowserView->nameFilter() );
   m_dirLister->setMimeFilter( m_pBrowserView->mimeFilter() );
   m_dirLister->setShowingDotFiles( m_pBrowserView->m_pProps->isShowingDotFiles() );

   KParts::URLArgs args = m_pBrowserView->extension()->urlArgs();
   if ( args.reload )
   {
      args.xOffset = contentsX();
      args.yOffset = contentsY();
      m_pBrowserView->extension()->setURLArgs( args );

      if ( currentItem() && itemRect( currentItem() ).isValid() )
         m_itemToGoTo = currentItem()->text(0);

      m_pBrowserView->m_filesToSelect.clear();
      iterator it = begin();
      for( ; it != end(); it++ )
         if ( it->isSelected() )
            m_pBrowserView->m_filesToSelect += it->text(0);
   }

   m_itemsToSelect = m_pBrowserView->m_filesToSelect;
   if ( !m_itemsToSelect.isEmpty() && m_itemToGoTo.isEmpty() )
      m_itemToGoTo = m_itemsToSelect[0];

   if ( columnWidthMode(0) == Maximum )
      setColumnWidth(0,50);

   m_url = url;
   m_bUpdateContentsPosAfterListing = true;

   // Start the directory lister !
   m_dirLister->openURL( url, false /* new url */, args.reload );

   // Apply properties and reflect them on the actions
   // do it after starting the dir lister to avoid changing the properties
   // of the old view
   if ( newProps )
   {
      m_pBrowserView->newIconSize( m_pBrowserView->m_pProps->iconSize() );
      m_pBrowserView->m_paShowDot->setChecked( m_pBrowserView->m_pProps->isShowingDotFiles() );
      if ( m_pBrowserView->m_paCaseInsensitive->isChecked() != m_pBrowserView->m_pProps->isCaseInsensitiveSort() ) {
          m_pBrowserView->m_paCaseInsensitive->setChecked( m_pBrowserView->m_pProps->isCaseInsensitiveSort() );
          // This is in case openURL returned all items synchronously.
          sort();
      }

      // It has to be "viewport()" - this is what KonqDirPart's slots act upon,
      // and otherwise we get a color/pixmap in the square between the scrollbars.
      m_pBrowserView->m_pProps->applyColors( viewport() );
   }

   return true;
}

void KonqBaseListViewWidget::setComplete()
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

      emit selectionChanged();
   }

   m_itemToGoTo = "";
   m_restored = false;

   // Show totals
   reportItemCounts();

   m_pBrowserView->emitMouseOver( 0 );

   if ( !updatesEnabled() || !viewport()->updatesEnabled() )
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
   //kDebug(1202) << k_funcinfo << endl;

   if (!m_bTopLevelComplete)
      emit m_pBrowserView->started( 0 );
}

void KonqBaseListViewWidget::slotCompleted()
{
   //kDebug(1202) << k_funcinfo << endl;

   setComplete();
   if ( m_bTopLevelComplete )
       emit m_pBrowserView->completed();
   m_pBrowserView->listingComplete();
}

void KonqBaseListViewWidget::slotCanceled()
{
   //kDebug(1202) << k_funcinfo << endl;

   setComplete();
   emit m_pBrowserView->canceled( QString() );
}

void KonqBaseListViewWidget::slotClear()
{
   //kDebug(1202) << k_funcinfo << endl;

   m_activeItem = 0;
   m_fileTip->setItem( 0 );
   delete m_selected; m_selected = 0;
   m_pBrowserView->resetCount();
   m_pBrowserView->lstPendingMimeIconItems().clear();

   viewport()->setUpdatesEnabled( false );
   setUpdatesEnabled( false );
   clear();
}

void KonqBaseListViewWidget::slotNewItems( const KFileItemList & entries )
{
   //kDebug(1202) << k_funcinfo << entries.count() << endl;

  KFileItemList::const_iterator kit = entries.begin();
  const KFileItemList::const_iterator kend = entries.end();
  for (; kit != kend; ++kit )
  {
      KonqListViewItem * tmp = new KonqListViewItem( this, *kit );
      if ( !m_itemFound && tmp->text(0) == m_itemToGoTo )
      {
         setCurrentItem( tmp );
         m_itemFound = true;
      }
      if ( !m_itemsToSelect.isEmpty() ) {
         int tsit = m_itemsToSelect.indexOf( (*kit)->name() );
         if ( tsit >= 0 ) {
            m_itemsToSelect.removeAt( tsit );
            setSelected( tmp, true );
         }
      }
      if ( !(*kit)->isMimeTypeKnown() )
          m_pBrowserView->lstPendingMimeIconItems().append( tmp );
   }
   m_pBrowserView->newItems( entries );

   if ( !viewport()->updatesEnabled() )
   {
      viewport()->setUpdatesEnabled( true );
      setUpdatesEnabled( true );
      triggerUpdate();
   }
   slotUpdateBackground();
}

void KonqBaseListViewWidget::slotDeleteItem( KFileItem * _fileitem )
{
  iterator it = begin();
  for( ; it != end(); ++it )
    if ( (*it).item() == _fileitem )
    {
      kDebug(1202) << k_funcinfo << "removing " << _fileitem->url().url() << " from tree!" << endl;

      m_pBrowserView->deleteItem( _fileitem );
      m_pBrowserView->lstPendingMimeIconItems().removeAll( &(*it) );

      if ( m_activeItem == &(*it) ) {
          m_fileTip->setItem( 0 );
          m_activeItem = 0;
      }

      delete &(*it);
      // HACK HACK HACK: Q3ListViewItem/KonqBaseListViewItem should
      // take care and the source looks like it does; till the
      // real bug is found, this fixes some crashes (malte)
      emit selectionChanged();
      return;
    }

  // This is needed for the case the root of the current view is deleted.
  // I supposed slotUpdateBackground has to be called as well after an item
  // was removed from a listview and was just forgotten previously (Brade).
  // OK, but this code also gets activated when deleting a hidden file... (dfaure)
  if ( !viewport()->updatesEnabled() )
  {
    viewport()->setUpdatesEnabled( true );
    setUpdatesEnabled( true );
    triggerUpdate();
  }
  slotUpdateBackground();
}

void KonqBaseListViewWidget::slotRefreshItems( const KFileItemList & entries )
{
   //kDebug(1202) << k_funcinfo << endl;

  KFileItemList::const_iterator kit = entries.begin();
  const KFileItemList::const_iterator kend = entries.end();
  for (; kit != kend; ++kit )
  {
      iterator it = begin();
      for ( ; it != end(); ++it )
         if ( (*it).item() == (*kit) )
         {
            it->updateContents();
            break;
         }
   }

   reportItemCounts();
}

void KonqBaseListViewWidget::slotRedirection( const KUrl & url )
{
   kDebug(1202) << k_funcinfo << url << endl;

   if ( (columns() < 1) || (url.protocol() != m_url.protocol()) )
   {
      readProtocolConfig( url );
      createColumns();
   }
   const QString prettyURL = url.pathOrURL();
   emit m_pBrowserView->extension()->setLocationBarURL( prettyURL );
   emit m_pBrowserView->setWindowCaption( prettyURL );
   m_pBrowserView->m_url = url;
   m_url = url;
}

KonqBaseListViewWidget::iterator& KonqBaseListViewWidget::iterator::operator++()
{
   if ( !m_p ) return *this;
   KonqBaseListViewItem *i = (KonqBaseListViewItem *)m_p->firstChild();
   if ( i )
   {
      m_p = i;
      return *this;
   }
   i = (KonqBaseListViewItem *)m_p->nextSibling();
   if ( i )
   {
      m_p = i;
      return *this;
   }
   m_p = (KonqBaseListViewItem *)m_p->parent();

   while ( m_p )
   {
      if ( m_p->nextSibling() )
         break;
      m_p = (KonqBaseListViewItem *)m_p->parent();
   }

   if ( m_p )
      m_p = (KonqBaseListViewItem *)m_p->nextSibling();

   return *this;
}

KonqBaseListViewWidget::iterator KonqBaseListViewWidget::iterator::operator++(int)
{
   KonqBaseListViewWidget::iterator it = *this;
   if ( !m_p ) return it;
   KonqBaseListViewItem *i = (KonqBaseListViewItem *)m_p->firstChild();
   if ( i )
   {
      m_p = i;
      return it;
   }
   i = (KonqBaseListViewItem *)m_p->nextSibling();
   if ( i )
   {
      m_p = i;
      return it;
   }
   m_p = (KonqBaseListViewItem *)m_p->parent();

   while ( m_p )
   {
      if ( m_p->nextSibling() )
         break;
      m_p = (KonqBaseListViewItem *)m_p->parent();
   }

   if ( m_p )
      m_p = (KonqBaseListViewItem *)m_p->nextSibling();
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
       /* kDebug() << "KonqBaseListViewWidget::paintEmptyArea "
                  << r.x() << "," << r.y() << " " << r.width() << "x" << r.height()
                  << " drawing pixmap with offset " << ax << "," << ay
                  << endl;*/
       p->drawTiledPixmap(r, *pm, QPoint(ax, ay));
   }
}

void KonqBaseListViewWidget::disableIcons( const KUrl::List & lst )
{
   iterator kit = begin();
   for( ; kit != end(); ++kit )
   {
      bool bFound = false;
      // Wow. This is ugly. Matching two lists together....
      // Some sorting to optimise this would be a good idea ?
      for (KUrl::List::ConstIterator it = lst.begin(); !bFound && it != lst.end(); ++it)
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
      str = static_cast<KonqBaseListViewItem*>(currentItem())->item()->url().fileName( KUrl::IgnoreTrailingSlash );
   ds << str << m_url;
}

void KonqBaseListViewWidget::restoreState( QDataStream & ds )
{
   m_restored = true;

   QString str;
   KUrl url;
   ds >> str >> url;
   if ( !str.isEmpty() )
      m_itemToGoTo = str;

   if ( columns() < 1 || url.protocol() != m_url.protocol() )
   {
      readProtocolConfig( url );
      createColumns();
   }
   m_url = url;

   m_bTopLevelComplete = false;
   m_itemFound = false;
}

void KonqBaseListViewWidget::slotUpdateBackground()
{
   if ( viewport()->paletteBackgroundPixmap() && !viewport()->paletteBackgroundPixmap()->isNull() )
   {
      if ( !m_backgroundTimer )
      {
         m_backgroundTimer = new QTimer( this );
         m_backgroundTimer->setSingleShot( true );
         connect( m_backgroundTimer, SIGNAL( timeout() ), viewport(), SLOT( update() ) );
      }
      else
         m_backgroundTimer->stop();

      m_backgroundTimer->start( 50 );
   }
}

bool KonqBaseListViewWidget::caseInsensitiveSort() const
{
    return m_pBrowserView->m_pProps->isCaseInsensitiveSort();
}

// based on isExecuteArea from klistview.cpp
int KonqBaseListViewWidget::executeArea( Q3ListViewItem *_item )
{
   if ( !_item )
      return 0;

   int width = treeStepSize() * ( _item->depth() + ( rootIsDecorated() ? 1 : 0 ) );
   width += itemMargin();
   int ca = Qt::AlignHorizontal_Mask & columnAlignment( 0 );
   if ( ca == Qt::AlignLeft )
   {
      width += _item->width( fontMetrics(), this, 0 );
      if ( width > columnWidth( 0 ) )
         width = columnWidth( 0 );
   }
   return width;
}

#include "konq_listviewwidget.moc"
