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

#include "konq_textview.h"
#include "konq_textviewitems.h"
#include "konq_textviewwidget.h"
#include "konq_htmlsettings.h"

#include <stdlib.h>

#include <qdragobject.h>
#include <qheader.h>

#include <kcursor.h>
#include <kdebug.h>
#include <kdirlister.h>
#include <kglobal.h>
#include <kglobalsettings.h>
#include <kio/job.h>
#include <kio/paste.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kprotocolmanager.h>
#include <konqoperations.h>

#include <konqsettings.h>
#include "konq_propsview.h"

#include <assert.h>

KonqTextViewWidget::KonqTextViewWidget( KonqTextView *parent, QWidget *parentWidget )
:QListView( parentWidget )
,m_dirLister(0L)
,m_pSettings(KonqFMSettings::defaultTreeSettings())
,m_bTopLevelComplete(true)
,m_dragOverItem(0L)
,m_overItem(0L)
,m_pressed(false)
,m_pressedItem(0L)
,m_stdCursor(KCursor().arrowCursor())
,m_handCursor(KCursor().handCursor())
,m_pBrowserView(parent)
,timer()
,locale(KGlobal::locale())
,m_settingsChanged(TRUE)
,m_filesSelected(FALSE)
,m_selectedFilesStatusText()
{
   kDebugInfo( 1202, "+KonqTextViewWidget");

   // Create a properties instance for this view
   // (copying the default values)
   m_pProps = new KonqPropsView( * KonqPropsView::defaultProps() );

   initConfig();

   addColumn(" ",fontMetrics().width("@")+2);
   addColumn(i18n("Name"),fontMetrics().width("_a_quite_long_filename_"));
   setColumnAlignment(0,AlignRight);

   addColumn(i18n("Size"),fontMetrics().width("000000000"));
   int permissionsWidth=fontMetrics().width(i18n("Permissions"))+fontMetrics().width("--");
   //cerr<<" set to: "<<permissionsWidth<<endl;

   addColumn(i18n("Owner"),fontMetrics().width("a_long_username"));
   addColumn(i18n("Group"),fontMetrics().width("a_groupname"));

   setColumnAlignment(2,AlignRight);
   setAllColumnsShowFocus(TRUE);
   setRootIsDecorated(false);
   setSorting( 1 );
   setMultiSelection( true );

   QObject::connect(this,SIGNAL(rightButtonPressed(QListViewItem*,const QPoint&,int)),this,SLOT(slotRightButtonPressed(QListViewItem*,const QPoint&,int)));
   QObject::connect(this,SIGNAL(returnPressed(QListViewItem*)),this,SLOT(slotReturnPressed( QListViewItem*)));
   QObject::connect(this,SIGNAL(doubleClicked(QListViewItem*)),this,SLOT(slotReturnPressed(QListViewItem*)));
   QObject::connect(this,SIGNAL(currentChanged(QListViewItem*)),this,SLOT(slotCurrentChanged(QListViewItem*)));

   viewport()->setAcceptDrops( true );
   viewport()->setMouseTracking( true );
   viewport()->setFocusPolicy( QWidget::WheelFocus );
   setFocusPolicy( QWidget::WheelFocus );
   setAcceptDrops( true );

   //the statusbar looks better if the view has a frame
   setFrameStyle( QFrame::WinPanel | QFrame::Sunken);
   setLineWidth(1);
   
   colors[REGULAR]=Qt::black;
   colors[EXEC]=QColor(0,170,0);
   colors[REGULARLINK]=Qt::black;
   colors[DIR]=Qt::black;
   colors[DIRLINK]=Qt::black;
   colors[BADLINK]=Qt::red;
   colors[SOCKET]=Qt::magenta;
   colors[FIFO]=Qt::magenta;
   colors[UNKNOWN]=Qt::red;
   colors[CHARDEV]=Qt::blue;
   colors[BLOCKDEV]=Qt::blue;

   highlight[REGULAR]=Qt::white;
   highlight[EXEC]=colors[EXEC].light(200);
   highlight[REGULARLINK]=Qt::white;
   highlight[DIR]=Qt::white;
   highlight[DIRLINK]=Qt::white;
   highlight[BADLINK]=colors[BADLINK].light();
   highlight[SOCKET]=colors[SOCKET].light();
   highlight[FIFO]=colors[FIFO].light();
   highlight[UNKNOWN]=colors[UNKNOWN].light();
   highlight[CHARDEV]=colors[CHARDEV].light(180);
   highlight[BLOCKDEV]=colors[BLOCKDEV].light(180);

   timer.start();
}

KonqTextViewWidget::~KonqTextViewWidget()
{
   kDebugInfo( 1202, "-KonqTextViewWidget");

   if ( m_dirLister ) delete m_dirLister;
   m_dirLister=0;
   if (m_pProps) delete m_pProps;
   m_pProps=0;
/*   for( iterator it=begin(); it != end(); ++it )
      it->prepareToDie();*/
}

void KonqTextViewWidget::stop()
{
   m_dirLister->stop();
}

const KURL & KonqTextViewWidget::url()
{
   return m_sURL;
}

void KonqTextViewWidget::initConfig()
{
   QColor bgColor           = m_pSettings->bgColor();
   // TODO QColor textColor         = m_pSettings->normalTextColor();
   // TODO highlightedTextColor

   QString stdFontName      = m_pSettings->stdFontName();
   int fontSize             = m_pSettings->fontSize();

   m_bgPixmap         = m_pProps->bgPixmap();

   if ( m_bgPixmap.isNull() )
   {
      // viewport()->setBackgroundMode( PaletteBackground );
      /*viewport()->*/setBackgroundColor( bgColor );
   }
   else
      viewport()->setBackgroundPixmap( m_bgPixmap );

   setFont(QFont ( stdFontName, fontSize ));

   m_bSingleClick       = KGlobalSettings::singleClick();
   m_bUnderlineLink     = m_pSettings->underlineLink();
   m_bChangeCursor      = KonqHTMLSettings::defaultHTMLSettings()->changeCursor();
}

void KonqTextViewWidget::viewportDragMoveEvent( QDragMoveEvent *_ev )
{
   kDebugInfo(1202,"KonqTextViewWidget::viewportDragMoveEvent");
   KonqTextViewItem *item = (KonqTextViewItem*)itemAt( _ev->pos() );
   if ( !item )
   {
      if ( m_dragOverItem )
         setSelected( m_dragOverItem, false );
      _ev->accept();
      return;
   }

   if ( m_dragOverItem == item )
      return;
   if ( m_dragOverItem != 0L )
      setSelected( m_dragOverItem, false );

   if ( item->item()->acceptsDrops( ) )
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
   return;
}

void KonqTextViewWidget::viewportDragEnterEvent( QDragEnterEvent *_ev )
{
   kDebugInfo(1202,"KonqTextViewWidget::viewportDragEnterEvent");
   m_dragOverItem = 0L;
   // Save the available formats
   m_lstDropFormats.clear();

   for( int i = 0; _ev->format( i ); i++ )
   {
      if ( *( _ev->format( i ) ) )
         m_lstDropFormats.append( _ev->format( i ) );
   }
   // By default we accept any format
   _ev->accept();
}

void KonqTextViewWidget::viewportDragLeaveEvent( QDragLeaveEvent * )
{
   kDebugInfo(1202,"KonqTextViewWidget::viewportDragLeaveEvent");
   if ( m_dragOverItem != 0L )
      setSelected( m_dragOverItem, false );
   m_dragOverItem = 0L;
}

void KonqTextViewWidget::viewportDropEvent( QDropEvent *ev  )
{
   kDebugInfo(1202,"KonqTextViewWidget::viewportDropEvent");
  if ( m_dragOverItem != 0L )
    setSelected( m_dragOverItem, false );
  m_dragOverItem = 0L;

  KonqTextViewItem *item = (KonqTextViewItem*)itemAt( ev->pos() );

  KonqFileItem * destItem = (item) ? item->item() : m_dirLister->rootItem();
  assert( destItem );
  KonqOperations::doDrop( destItem, ev, this );
}

void KonqTextViewWidget::keyPressEvent( QKeyEvent *_ev )
{
   //cerr<<"keyPressEvent"<<endl;
   // We are only interested in the escape key here
   KonqTextViewItem* item = (KonqTextViewItem*)currentItem();
   if (((_ev->key()==Key_Return) || (_ev->key()==Key_Enter))&& (_ev->state()==ControlButton))
   {
      if (item==0) return;
      if ( !item->isSelected() )
      {
         iterator it = begin();
         for( ; it != end(); it++ )
            if ( it->isSelected() )
               setSelected( &*it, false );
         setSelected( item, true );
      }

      QPoint p( width() / 2, height() / 2 );
      p = mapToGlobal( p );

      //cerr<<"popup the menu"<<endl;
      popupMenu( p );
      
   }
   //insert without modifiers toggles the selection of the current item and moves to the next
   else if ((_ev->key()==Key_Insert) && (_ev->state()==0))
   {
      if (item==0) return;
      item->setSelected(!item->isSelected());
      QListViewItem *nextItem=item->itemBelow();
      if (nextItem!=0)
      {
         setCurrentItem(nextItem);
         ensureItemVisible(nextItem);
      }
      else item->repaint();
      emit selectionChanged();
      updateSelectedFilesInfo();
   }
   else
   {
      //cerr<<"QListView->keyPressEvent"<<endl;
      QListView::keyPressEvent( _ev );
   };
};

void KonqTextViewWidget::viewportMousePressEvent( QMouseEvent *_ev )
{
   kDebugInfo(1202,"KonqTextViewWidget::viewportMousePressEvent");

   QPoint globalPos = mapToGlobal( _ev->pos() );
   m_pressed = false;

   if ( m_bSingleClick )
   {
      KonqTextViewItem *item = (KonqTextViewItem*)itemAt( _ev->pos() );
      if ( item )
      {
         if ( m_overItem )
         {
            //reset to standard cursor
            setCursor( m_stdCursor );
            m_overItem = 0;
         }

         if ( ( _ev->state() & ControlButton ) && _ev->button() == LeftButton )
         {
            setSelected( item, !item->isSelected() );
            return;
         }

         if ( _ev->button() == RightButton && !item->isSelected() )
         {
            clearSelection();
            setSelected( item, true );
         }
         else if ( _ev->button() == LeftButton || _ev->button() == MidButton )
         {
            if ( !item->isSelected() )
            {
               clearSelection();
               setSelected( item, true );
            }
	
            m_pressed = true;
            m_pressedPos = _ev->pos();
            m_pressedItem = item;
            return;
         }
         popupMenu( globalPos );
         return;
      }
      else if ( _ev->button() == RightButton )
      {
         popupMenu( globalPos );
         return;
      }
      else clearSelection();
   }
}

void KonqTextViewWidget::viewportMouseReleaseEvent( QMouseEvent *_mouse )
{
   kDebugInfo(1202,"KonqTextViewWidget::viewportMouseReleaseEvent");
   if ( !m_pressed )
      return;

   if ( m_bSingleClick
        && _mouse->button() == LeftButton
        && !( ( _mouse->state() & ControlButton ) == ControlButton ))
   {
      slotReturnPressed( m_pressedItem );
   }

   m_pressed = false;
   m_pressedItem = 0L;
}

void KonqTextViewWidget::viewportMouseMoveEvent( QMouseEvent *_mouse )
{
   if ( !m_pressed || !m_pressedItem )
   {
      KonqTextViewItem* item = (KonqTextViewItem*)itemAt( _mouse->pos() );
      if (!item)
      {
         slotOnItem( 0L );
         setCursor( m_stdCursor );
         m_overItem = 0L;
      }
      else if ( m_overItem != item ) // we are on another item than before
      {
         slotOnItem( item );
         m_overItem = item;

         if ( m_bSingleClick && m_bChangeCursor )
            setCursor( m_handCursor );
      }
      return;
   }

   int x = _mouse->pos().x();
   int y = _mouse->pos().y();

   //is it dnd ?
   if ( abs( x - m_pressedPos.x() ) > KGlobalSettings::dndEventDelay() || abs( y - m_pressedPos.y() ) > KGlobalSettings::dndEventDelay() )
   {
      // Collect all selected items
      QStrList urls;
      iterator it = begin();
      for( ; it != end(); it++ )
         if ( it->isSelected() )
            urls.append( it->item()->url().url() );

      // Always the same pixmap
      QPixmap pixmap2 = KGlobal::iconLoader()->loadIcon( "kmultiple", KIconLoader::Medium );
      if ( pixmap2.isNull() )
         warning("KDesktop: Could not find kmultiple pixmap\n");
      
      // Do not handle and more mouse move or mouse release events
      m_pressed = false;

      QUriDrag *d = new QUriDrag( urls, viewport() );
      if ( !pixmap2.isNull() )
      {
         QPoint hotspot;
         hotspot.setX( pixmap2.width() / 2 );
         hotspot.setY( pixmap2.height() / 2 );
         d->setPixmap( pixmap2, hotspot );
      }
      d->drag();
   }
}

void KonqTextViewWidget::slotOnItem( KonqTextViewItem* _item)
{
   QString s;
   if (( _item ) && (!m_filesSelected))
      s = _item->item()->getStatusBarInfo();
   else
      if (m_filesSelected) s=m_selectedFilesStatusText;
   emit m_pBrowserView->setStatusBarText( s );
}

void KonqTextViewWidget::selectedItems( QValueList<KonqTextViewItem*>& _list )
{
  iterator it = begin();
  for( ; it != end(); it++ )
     if ( it->isSelected() )
        _list.append( &*it );
}

void KonqTextViewWidget::slotReturnPressed( QListViewItem *_item )
{
   if ( !_item )
      return;
   KonqFileItem *fileItem = ((KonqTextViewItem*)_item)->item();
   if ( !fileItem )
      return;
   QString serviceType = QString::null;

   KURL u( fileItem->url() );


//   emit m_pBrowserView->extension()->openURLRequest( u, false, 0, 0, fileItem->mimetype() );
    KParts::URLArgs args;
    if ( u.isLocalFile() )
      serviceType = fileItem->mimetype();
    //args.serviceType = item->mimetype();
    emit m_pBrowserView->extension()->openURLRequest( fileItem->url(), args );

}

void KonqTextViewWidget::slotRightButtonPressed( QListViewItem *_item, const QPoint &_global, int )
{
   if ( _item && !_item->isSelected() )
   {
      // Deselect all the others
      iterator it = begin();
      for( ; it != end(); ++it )
         if ( it->isSelected() )
            QListView::setSelected( &*it, false );
      QListView::setSelected( _item, true );
   }

   // Popup menu for m_url
   // Torben: I think this is impossible in the textview, or ?
   //         Perhaps if the list is smaller than the window height.
   // Simon: Yes, I think so. This happens easily if using the textview
   //        with small directories.
   popupMenu( _global );
}

void KonqTextViewWidget::popupMenu( const QPoint& _global )
{
   KonqFileItemList lstItems;

   QValueList<KonqTextViewItem*> items;
   selectedItems( items );
   QValueList<KonqTextViewItem*>::Iterator it = items.begin();
   for( ; it != items.end(); ++it )
      lstItems.append( (*it)->item() );

   if ( lstItems.count() == 0 ) // emit popup for background
   {
      assert( m_dirLister->rootItem() );
      lstItems.append( m_dirLister->rootItem() );
   }

   emit m_pBrowserView->extension()->popupMenu( _global, lstItems );
}

bool KonqTextViewWidget::openURL( const KURL &url )
{
   if ( !m_dirLister )
   {
      // Create the directory lister
      m_dirLister = new KDirLister(true);

      QObject::connect( m_dirLister, SIGNAL( started( const QString & ) ),
                        this, SLOT( slotStarted( const QString & ) ) );
      QObject::connect( m_dirLister, SIGNAL( completed() ), this, SLOT( slotCompleted() ) );
      QObject::connect( m_dirLister, SIGNAL( canceled() ), this, SLOT( slotCanceled() ) );
      QObject::connect( m_dirLister, SIGNAL( clear() ), this, SLOT( slotClear() ) );
      QObject::connect( m_dirLister, SIGNAL( newItems( const KonqFileItemList & ) ),
                        this, SLOT( slotNewItems( const KonqFileItemList & ) ) );
      QObject::connect( m_dirLister, SIGNAL( deleteItem( KonqFileItem * ) ),
                        this, SLOT( slotDeleteItem( KonqFileItem * ) ) );
   }

   m_bTopLevelComplete = false;

   m_sURL=url;

   if ( m_pProps->enterDir( url ) )
   {
      // nothing to do yet
   }

   // Start the directory lister !
   m_dirLister->openURL( url, m_pProps->m_bShowDot, false /* new url */ );

   //  setCaptionFromURL( m_sURL );
   return true;
}

void KonqTextViewWidget::slotStarted( const QString & /*url*/ )
{
   if ( !m_bTopLevelComplete )
      emit m_pBrowserView->started( 0 );
   setUpdatesEnabled(FALSE);
   timer.restart();
   if (m_settingsChanged)
   {
      m_settingsChanged=FALSE;
      for (int i=columns()-1; i>1; i--)
         removeColumn(i);

      if (m_showSize)
      {
         addColumn(i18n("Size"),fontMetrics().width("000000000"));
         setColumnAlignment(2,AlignRight);
      };
      if (m_showTime)
      {
         QDateTime dt(QDate(2000,10,10),QTime(20,20,20));
         addColumn(i18n("Modified"),fontMetrics().width(locale->formatDate(dt.date(),true)+" "+locale->formatTime(dt.time())+"---"));
      };
      if (m_showOwner)
         addColumn(i18n("Owner"),fontMetrics().width("a_long_username"));

      //   int permissionsWidth=fontMetrics().width(i18n("Permissions"))+fontMetrics().width("--");
      //   cerr<<" set to: "<<permissionsWidth<<endl;

      if (m_showGroup)
         addColumn(i18n("Group"),fontMetrics().width("a_groupname"));
      if (m_showPermissions)
         addColumn(i18n("Permissions"),fontMetrics().width("--Permissions--"));
      //   int permissionsWidth=fontMetrics().width(i18n("Permissions"))+fontMetrics().width("--");
      //   cerr<<" set to: "<<permissionsWidth<<endl;
   };
}

void KonqTextViewWidget::setComplete()
{
   m_bTopLevelComplete = true;

   setContentsPos( m_pBrowserView->extension()->urlArgs().xOffset, m_pBrowserView->extension()->urlArgs().yOffset );
   setUpdatesEnabled(TRUE);
   repaintContents(0,0,width(),height());
   emit selectionChanged();
   //setContentsPos( m_iXOffset, m_iYOffset );
//   show();
}


void KonqTextViewWidget::slotCompleted()
{
   setComplete();
   emit m_pBrowserView->completed();
   //cerr<<"needed "<<timer.elapsed()<<" msecs"<<endl;
}

void KonqTextViewWidget::slotCanceled()
{
   setComplete();
   emit m_pBrowserView->canceled( QString::null );
}

void KonqTextViewWidget::slotClear()
{
   kDebugInfo( 1202, "KonqTextViewWidget::slotClear()");
   clear();
}

void KonqTextViewWidget::slotNewItems( const KonqFileItemList & entries )
{
   for( QListIterator<KonqFileItem> kit (entries); kit.current(); ++kit )
      new KonqTextViewItem( this, (*kit));
   //cerr<<"KTVI::slotNewItems: received "<<entries.count()<<endl;
}

void KonqTextViewWidget::slotDeleteItem( KonqFileItem * _fileitem )
{
   kDebugInfo(1202,"removing %s from text!", _fileitem->url().url().ascii() );
   iterator it = begin();
   for( ; it != end(); ++it )
      if ( (*it).item() == _fileitem )
      {
         delete &(*it);
         return;
      }
}

KonqTextViewWidget::iterator& KonqTextViewWidget::iterator::operator++()
{
  if ( !m_p ) return *this;
  KonqTextViewItem *i = (KonqTextViewItem*)m_p->firstChild();
  if ( i )
  {
    m_p = i;
    return *this;
  }
  i = (KonqTextViewItem*)m_p->nextSibling();
  if ( i )
  {
    m_p = i;
    return *this;
  }
  m_p = (KonqTextViewItem*)m_p->parent();
  if ( m_p )
    m_p = (KonqTextViewItem*)m_p->nextSibling();

  return *this;
}

KonqTextViewWidget::iterator KonqTextViewWidget::iterator::operator++(int)
{
  KonqTextViewWidget::iterator it = *this;
  if ( !m_p ) return it;
  KonqTextViewItem *i = (KonqTextViewItem*)m_p->firstChild();
  if ( i )
  {
    m_p = i;
    return it;
  }
  i = (KonqTextViewItem*)m_p->nextSibling();
  if ( i )
  {
    m_p = i;
    return it;
  }
  m_p = (KonqTextViewItem*)m_p->parent();
  if ( m_p )
    m_p = (KonqTextViewItem*)m_p->nextSibling();

  return it;
}

void KonqTextViewWidget::drawContentsOffset( QPainter* _painter, int _offsetx, int _offsety,
				    int _clipx, int _clipy, int _clipw, int _cliph )
{
   if ( !_painter )
      return;

   if ( !m_bgPixmap.isNull() )
   {
      int pw = m_bgPixmap.width();
      int ph = m_bgPixmap.height();

      int xOrigin = (_clipx/pw)*pw - _offsetx;
      int yOrigin = (_clipy/ph)*ph - _offsety;

      int rx = _clipx%pw;
      int ry = _clipy%ph;

      for ( int yp = yOrigin; yp - yOrigin < _cliph + ry; yp += ph )
      {
         for ( int xp = xOrigin; xp - xOrigin < _clipw + rx; xp += pw )
            _painter->drawPixmap( xp, yp, m_bgPixmap );
      }
   }

   QListView::drawContentsOffset( _painter, _offsetx, _offsety,
                                  _clipx, _clipy, _clipw, _cliph );
}

void KonqTextViewWidget::focusInEvent( QFocusEvent* _event )
{
   //  emit gotFocus();

   QListView::focusInEvent( _event );
}

void KonqTextViewWidget::updateSelectedFilesInfo()
{
   long fileSizeSum = 0;
   long fileCount = 0;
   long dirCount = 0;
   m_filesSelected=FALSE;
   m_selectedFilesStatusText="";
   for (iterator it = begin(); it != end(); it++ )
   {
      if (it->isSelected())
      {
         m_filesSelected=TRUE;
         if ( S_ISDIR( it->item()->mode() ) )
            dirCount++;
         else
         {
            fileSizeSum += it->item()->size();
            fileCount++;
         }
      }
   };
   if (m_filesSelected)
   {
      int items(fileCount+dirCount);
      if (items == 1)
         m_selectedFilesStatusText= i18n("One Item");
      else
         m_selectedFilesStatusText= i18n("%1 Items").arg(items);
      m_selectedFilesStatusText+= " - ";
      if (fileCount == 1)
         m_selectedFilesStatusText+= i18n("One File");
      else
         m_selectedFilesStatusText+= i18n("%1 Files").arg(fileCount);
      m_selectedFilesStatusText+= " ";
      m_selectedFilesStatusText+= i18n("(%1 Total)").arg(KIO::convertSize(fileSizeSum));
      m_selectedFilesStatusText+= " - ";
      if (dirCount == 1)
         m_selectedFilesStatusText+= i18n("One Directory");
      else
         m_selectedFilesStatusText+= i18n("%1 Directories").arg(dirCount);
   };
   emit m_pBrowserView->setStatusBarText(m_selectedFilesStatusText);
   //cerr<<"KonqTextViewWidget::updateSelectedFilesInfo"<<endl;
};

KURL::List KonqTextViewWidget::selectedUrls()
{
  KURL::List list;
  iterator it = begin();
  for( ; it != end(); it++ )
    if ( it->isSelected() )
      list.append( it->item()->url() );
  return list;
}

void KonqTextViewWidget::slotResult( KIO::Job * job )
{
    if (job->error())
        job->showErrorDialog();
}
#include "konq_textviewwidget.moc"
