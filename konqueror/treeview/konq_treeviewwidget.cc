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

#include "konq_treeview.h"
#include "konq_treeviewitems.h"
#include "konq_treeviewwidget.h"

#include <qdragobject.h>
#include <qheader.h>

#include <kcursor.h>
#include <kdebug.h>
#include <kdirlister.h>
#include <kdirwatch.h>
#include <kglobal.h>
#include <kio_job.h>
#include <kio_paste.h>
#include <klocale.h>
#include <kmessagebox.h>
#include <kpixmapcache.h>
#include <kprotocolmanager.h>

#include <konqsettings.h>
#include "konq_propsview.h"

#include <assert.h>

KonqTreeViewWidget::KonqTreeViewWidget( KonqTreeView *parent )
: QListView( parent )
{
  kdebug(0, 1202, "+KonqTreeViewWidget");

  setMultiSelection( true );

  m_pBrowserView       = parent;
  m_pWorkingDir        = 0L;
  m_bTopLevelComplete  = true;
  m_bSubFolderComplete = true;
  m_iColumns           = -1;
  m_dragOverItem       = 0L;
  m_pressed            = false;
  m_pressedItem        = 0L;
  m_handCursor         = KCursor().handCursor();
  m_stdCursor          = KCursor().arrowCursor();
  m_overItem           = 0L;
  m_dirLister          = 0L;

  // Create a properties instance for this view
  // (copying the default values)
  m_pProps = new KonqPropsView( * KonqPropsView::defaultProps() );
  m_pSettings = KonqSettings::defaultFMSettings();

  setRootIsDecorated( true );
  setTreeStepSize( 20 );
  setSorting( 1 );

  initConfig();

  QObject::connect( this, SIGNAL( rightButtonPressed( QListViewItem*,
					     const QPoint&, int ) ),
	   this, SLOT( slotRightButtonPressed( QListViewItem*,
					       const QPoint&, int ) ) );
  QObject::connect( this, SIGNAL( returnPressed( QListViewItem* ) ),
	   this, SLOT( slotReturnPressed( QListViewItem* ) ) );
  QObject::connect( this, SIGNAL( doubleClicked( QListViewItem* ) ),
	   this, SLOT( slotReturnPressed( QListViewItem* ) ) );
  QObject::connect( this, SIGNAL( currentChanged( QListViewItem* ) ),
	   this, SLOT( slotCurrentChanged( QListViewItem* ) ) );

  //  connect( m_pView->gui(), SIGNAL( configChanged() ), SLOT( initConfig() ) );

  viewport()->setAcceptDrops( true );
  viewport()->setMouseTracking( true );
  setAcceptDrops( true );

  m_dragOverItem = 0L;

  setFrameStyle( QFrame::NoFrame | QFrame::Plain );

//    connect( &m_timer, SIGNAL( timeout() ), this, SLOT( slotUpdate() ) );
  m_lstNewItems.setAutoDelete( false );
}

KonqTreeViewWidget::~KonqTreeViewWidget()
{
  kdebug(0, 1202, "-KonqTreeViewWidget");

  if ( m_dirLister ) delete m_dirLister;
  delete m_pProps;

  iterator it = begin();
  for( ; it != end(); ++it )
    it->prepareToDie();

}

void KonqTreeViewWidget::stop()
{
  m_dirLister->stop();
}

QString KonqTreeViewWidget::url()
{
  return m_strURL;
}

void KonqTreeViewWidget::initConfig()
{
  QColor bgColor           = m_pSettings->bgColor();
  QColor textColor         = m_pSettings->textColor();
  // unused ???? QColor linkColor         = m_pSettings->linkColor();
  // unused QColor vLinkColor        = m_pSettings->vLinkColor();

  QString stdFontName      = m_pSettings->stdFontName();
  QString fixedFontName    = m_pSettings->fixedFontName();
  int fontSize             = m_pSettings->fontSize();

  m_bgPixmap         = m_pProps->bgPixmap();

//   if ( m_bgPixmap.isNull() )
//     viewport()->setBackgroundMode( PaletteBackground );
//   else
//     viewport()->setBackgroundMode( NoBackground );

  QPalette p          = viewport()->palette();
  QColorGroup c = p.normal();
  QColorGroup n( textColor, bgColor, c.light(), c.dark(), c.mid(),
		 textColor, bgColor );
  p.setNormal( n );
  c = p.active();
  QColorGroup a( textColor, bgColor, c.light(), c.dark(), c.mid(),
		 textColor, bgColor );
  p.setActive( a );
  c = p.disabled();
  QColorGroup d( textColor, bgColor, c.light(), c.dark(), c.mid(),
		 textColor, bgColor );
  p.setDisabled( d );
  setPalette( p );

  QFont font( stdFontName, fontSize );
  setFont( font );

  m_bSingleClick       = m_pSettings->singleClick();
  m_bUnderlineLink     = m_pSettings->underlineLink();
  m_bChangeCursor      = m_pSettings->changeCursor();
}

void KonqTreeViewWidget::viewportDragMoveEvent( QDragMoveEvent *_ev )
{
  KonqTreeViewItem *item = (KonqTreeViewItem*)itemAt( _ev->pos() );
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

void KonqTreeViewWidget::viewportDragEnterEvent( QDragEnterEvent *_ev )
{
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

void KonqTreeViewWidget::viewportDragLeaveEvent( QDragLeaveEvent * )
{
  if ( m_dragOverItem != 0L )
    setSelected( m_dragOverItem, false );
  m_dragOverItem = 0L;

  /** DEBUG CODE */
  // Give the user some feedback...
  /** End DEBUG CODE */
}

void KonqTreeViewWidget::viewportDropEvent( QDropEvent *_ev  )
{
  if ( m_dragOverItem != 0L )
    setSelected( m_dragOverItem, false );
  m_dragOverItem = 0L;

  KonqTreeViewItem *item = (KonqTreeViewItem*)itemAt( _ev->pos() );

  QStringList lst;

  QString dest = ( item ) ? item->item()->url().url() : m_dirLister->url();

  if ( QUrlDrag::decodeToUnicodeUris( _ev, lst ) )
  {
    if ( lst.count() == 0 )
      return;

    KIOJob *job = new KIOJob;
    job->copy( lst, dest );
  }
  else if ( m_lstDropFormats.count() >= 1 )
    pasteData( dest, _ev->data( m_lstDropFormats.first() ) );
}

void KonqTreeViewWidget::addSubDir( const KURL & _url, KonqTreeViewDir* _dir )
{
  m_mapSubDirs.insert( _url.url(), _dir );

  if ( _url.isLocalFile() )
    kdirwatch->addDir( _url.path(0) );
}

void KonqTreeViewWidget::removeSubDir( const KURL & _url )
{
  m_mapSubDirs.remove( _url.url() );

  if ( _url.isLocalFile() )
    kdirwatch->removeDir( _url.path(0) );
}

void KonqTreeViewWidget::keyPressEvent( QKeyEvent *_ev )
{
  // We are only interested in the escape key here
  if ( _ev->key() != Key_Escape )
  {
    QListView::keyPressEvent( _ev );
    return;
  }

  KonqTreeViewItem* item = (KonqTreeViewItem*)currentItem();

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

  popupMenu( p );
}

void KonqTreeViewWidget::viewportMousePressEvent( QMouseEvent *_ev )
{

  QPoint globalPos = mapToGlobal( _ev->pos() );
  m_pressed = false;

  if ( m_bSingleClick )
  {
    KonqTreeViewItem *item = (KonqTreeViewItem*)itemAt( _ev->pos() );
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

void KonqTreeViewWidget::viewportMouseReleaseEvent( QMouseEvent *_mouse )
{
  if ( !m_pressed )
    return;

  if ( m_bSingleClick &&
       _mouse->button() == LeftButton &&
       !( ( _mouse->state() & ControlButton ) == ControlButton ) &&
       isSingleClickArea( _mouse->pos() ) )
  {
    if ( m_pressedItem->isExpandable() )
      m_pressedItem->setOpen( !m_pressedItem->isOpen() );

    slotReturnPressed( m_pressedItem );
  }

  m_pressed = false;
  m_pressedItem = 0L;
}

void KonqTreeViewWidget::viewportMouseMoveEvent( QMouseEvent *_mouse )
{
  if ( !m_pressed || !m_pressedItem )
  {
    KonqTreeViewItem* item = (KonqTreeViewItem*)itemAt( _mouse->pos() );
    if ( isSingleClickArea( _mouse->pos() ) )
    {
      if ( m_overItem != item ) // we are on another item than before
      {
	slotOnItem( item );
	m_overItem = item;

	if ( m_bSingleClick && m_bChangeCursor )
	  setCursor( m_handCursor );
      }
    }
    else if ( !item )
    {
      slotOnItem( 0L );

      setCursor( m_stdCursor );
      m_overItem = 0L;
    }

    return;
  }

  int x = _mouse->pos().x();
  int y = _mouse->pos().y();

  if ( abs( x - m_pressedPos.x() ) > KGlobal::dndEventDelay() || abs( y - m_pressedPos.y() ) > KGlobal::dndEventDelay() )
  {
    // Collect all selected items
    QStrList urls;
    iterator it = begin();
    for( ; it != end(); it++ )
      if ( it->isSelected() )
	urls.append( it->item()->url().url() );

    // Multiple URLs ?
    QPixmap pixmap2;
    if ( urls.count() > 1 )
    {
      pixmap2 = KPixmapCache::pixmap( "kmultiple", true );
      if ( pixmap2.isNull() )
	warning("KDesktop: Could not find kmultiple pixmap\n");
    }

    // Calculate hotspot
    QPoint hotspot;

    // Do not handle and more mouse move or mouse release events
    m_pressed = false;

    QUriDrag *d = new QUriDrag( urls, viewport() );
    if ( pixmap2.isNull() )
    {
      hotspot.setX( m_pressedItem->pixmap( 0 )->width() / 2 );
      hotspot.setY( m_pressedItem->pixmap( 0 )->height() / 2 );
      d->setPixmap( *(m_pressedItem->pixmap( 0 )), hotspot );
    }
    else
    {
      hotspot.setX( pixmap2.width() / 2 );
      hotspot.setY( pixmap2.height() / 2 );
      d->setPixmap( pixmap2, hotspot );
    }

    d->drag();
  }
}

bool KonqTreeViewWidget::isSingleClickArea( const QPoint& _point )
{
  if ( itemAt( _point ) )
  {
    int x = _point.x();
    int pos = header()->mapToActual( 0 );
    int offset = 0;
    int width = columnWidth( pos );

    for ( int index = 0; index < pos; index++ )
      offset += columnWidth( index );

    return ( x > offset && x < ( offset + width ) );
  }

  return false;
}

void KonqTreeViewWidget::slotOnItem( KonqTreeViewItem* _item)
{
  QString s;
  if ( _item )
    s = _item->item()->getStatusBarInfo();
  emit m_pBrowserView->setStatusBarText( s );
}

void KonqTreeViewWidget::selectedItems( QValueList<KonqTreeViewItem*>& _list )
{
  iterator it = begin();
  for( ; it != end(); it++ )
    if ( it->isSelected() )
      _list.append( &*it );
}

void KonqTreeViewWidget::slotReturnPressed( QListViewItem *_item )
{
  if ( !_item )
    return;

  KFileItem *item = ((KonqTreeViewItem*)_item)->item();
  mode_t mode = item->mode();

  //execute only if item is a file (or a symlink to a file)
  if ( S_ISREG( mode ) )
    emit m_pBrowserView->openURLRequest( item->url().url(), false, 0, 0 );
}

void KonqTreeViewWidget::slotRightButtonPressed( QListViewItem *_item, const QPoint &_global, int )
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
  // Torben: I think this is impossible in the treeview, or ?
  //         Perhaps if the list is smaller than the window height.
  // Simon: Yes, I think so. This happens easily if using the treeview
  //        with small directories.
  popupMenu( _global );
}

void KonqTreeViewWidget::popupMenu( const QPoint& _global )
{
  KFileItemList lstItems;

  QValueList<KonqTreeViewItem*> items;
  selectedItems( items );
  QValueList<KonqTreeViewItem*>::Iterator it = items.begin();
  for( ; it != items.end(); ++it )
    lstItems.append( (*it)->item() );

  if ( lstItems.count() == 0 )
  {
    mode_t mode = S_IFDIR;
    KURL url( m_dirLister->url() );

    KFileItem item( "foobarbazzboom", mode, url );
    lstItems.append( &item );
  }

  emit m_pBrowserView->popupMenu( _global, lstItems );
}

void KonqTreeViewWidget::openURL( const char *_url, int xOffset, int yOffset )
{
  kdebug(0, 1202, "KonqTreeViewWidget::openURL( %s, %d, %d )", _url, xOffset, yOffset );
  bool isNewProtocol = false;

  KURL url( _url );
  if ( url.isMalformed() )
  {
    QString tmp = i18n( "Malformed URL\n%1" ).arg( _url );
    KMessageBox::sorry( this, tmp);
    return;
  }

  //test if we are switching to a new protocol
  if ( m_dirLister )
  {
    if ( strcmp( m_dirLister->kurl().protocol(), url.protocol() ) != 0 )
      isNewProtocol = true;
  }

  // The first time or new protocol ? So create the columns first
  if ( m_iColumns == -1 || isNewProtocol )
  {
    QStringList listing = KProtocolManager::self().listing( url.protocol() );
    if ( listing.isEmpty() )
    {
      QString tmp = i18n( "Unknown Protocol %1" ).arg( url.protocol());
      KMessageBox::sorry( this, tmp );
      return;
    }

    if ( m_iColumns == -1 )
      m_iColumns = 0;

    int currentColumn = 0;

    QStringList::Iterator it = listing.begin();
    for( ; it != listing.end(); it++ )
    {	
      if ( currentColumn > m_iColumns - 1 )
      {
	addColumn( *it );
	m_iColumns++;
	currentColumn++;
      }
      else
      {
	setColumnText( currentColumn, *it );
	currentColumn++;
      }
    }

    while ( currentColumn < m_iColumns )
      setColumnText( currentColumn++, "" );
  }

  if ( !m_dirLister )
  {
    // Create the directory lister
    m_dirLister = new KDirLister();

    QObject::connect( m_dirLister, SIGNAL( started( const QString & ) ),
                      this, SLOT( slotStarted( const QString & ) ) );
    QObject::connect( m_dirLister, SIGNAL( completed() ), this, SLOT( slotCompleted() ) );
    QObject::connect( m_dirLister, SIGNAL( canceled() ), this, SLOT( slotCanceled() ) );
    QObject::connect( m_dirLister, SIGNAL( clear() ), this, SLOT( slotClear() ) );
    QObject::connect( m_dirLister, SIGNAL( newItem( KFileItem * ) ),
                      this, SLOT( slotNewItem( KFileItem * ) ) );
    QObject::connect( m_dirLister, SIGNAL( deleteItem( KFileItem * ) ),
                      this, SLOT( slotDeleteItem( KFileItem * ) ) );
  }

  m_bTopLevelComplete = false;
  m_iXOffset = xOffset;
  m_iYOffset = yOffset;

  // Start the directory lister !
  m_dirLister->openURL( url, m_pProps->m_bShowDot, false /* new url */ );
  m_strURL = url.url();

//  setCaptionFromURL( m_strURL );
}

void KonqTreeViewWidget::setComplete()
{
  if ( m_pWorkingDir )
  {
    m_bSubFolderComplete = true;
    m_pWorkingDir->setComplete( true );
    m_pWorkingDir = 0L;
  }
  else
  {
    m_bTopLevelComplete = true;
    setContentsPos( m_iXOffset, m_iYOffset );
  }
}

void KonqTreeViewWidget::slotStarted( const QString & /*url*/ )
{
  if ( !m_bTopLevelComplete )
    emit m_pBrowserView->started();
  m_timer.start( 500, true );
}

void KonqTreeViewWidget::slotCompleted()
{
  if ( !m_bTopLevelComplete )
    emit m_pBrowserView->completed();
  setComplete();
  m_timer.stop();
  slotUpdate();
}

void KonqTreeViewWidget::slotCanceled()
{
  setComplete();
  emit m_pBrowserView->canceled();
  m_timer.stop();
  slotUpdate();
}

void KonqTreeViewWidget::slotClear()
{
  kdebug( KDEBUG_INFO, 1202, "KonqTreeViewWidget::slotClear()");
  if ( !m_pWorkingDir )
    clear();
}

void KonqTreeViewWidget::slotNewItem( KFileItem * _fileitem )
{
  m_lstNewItems.append( _fileitem );
  if ( !m_timer.isActive() )
    {
      m_timer.start( 500, true );
      slotUpdate();
    }
}

void KonqTreeViewWidget::slotUpdate()
{
  kdebug( KDEBUG_INFO, 1202, "KonqTreeViewWidget::slotUpdate(...)");
  QListIterator<KFileItem> kit ( m_lstNewItems );
  for( ; kit.current(); ++kit )
  {
    bool isdir = S_ISDIR( (*kit)->mode() );

    KURL dir ( (*kit)->url() );
    dir.setFileName( "" );
    kdebug( KDEBUG_INFO, 1202, "dir = %s", dir.url().ascii());
    KonqTreeViewDir * parentDir = 0L;
    if( !urlcmp( dir.url(0), m_strURL, true, true ) ) 
      {
        parentDir = findDir ( dir.url( 0 ) );
        kdebug( KDEBUG_INFO, 1202, "findDir returned %p", parentDir );
      }

    if ( parentDir ) { // adding under a directory item
      if ( isdir )
        new KonqTreeViewDir( this, parentDir, (*kit) );
      else
        new KonqTreeViewItem( this, parentDir, (*kit) );
    } else { // adding on the toplevel
      if ( isdir )
        new KonqTreeViewDir( this, (*kit) );
      else
        new KonqTreeViewItem( this, (*kit) );
    }
  }
  m_lstNewItems.clear();
}


void KonqTreeViewWidget::slotDeleteItem( KFileItem * _fileitem )
{
  kdebug(0,1202,"removing %s from tree!", _fileitem->url().url().ascii() );
  iterator it = begin();
  for( ; it != end(); ++it )
    if ( (*it).item() == _fileitem )
    {
      delete &(*it);
      return;
    }
}

void KonqTreeViewWidget::openSubFolder( const KURL &_url, KonqTreeViewDir* _dir )
{
  if ( !m_bTopLevelComplete )
  {
    // TODO: Give a warning
    kdebug(0,1202,"Still waiting for toplevel directory");
    return;
  }

  // If we are opening another sub folder right now, stop it.
  if ( !m_bSubFolderComplete )
  {
    m_dirLister->stop();
  }

  /** Debug code **/
  assert( m_iColumns != -1 && m_dirLister );
  if ( strcmp( m_dirLister->kurl().protocol(), _url.protocol() ) != 0 )
    assert( 0 ); // not same protocol as parent dir -> abort
  /** End Debug code **/

  m_bSubFolderComplete = false;
  m_pWorkingDir = _dir;
  m_dirLister->openURL( _url, m_pProps->m_bShowDot, true /* keep existing data */ );
}

KonqTreeViewWidget::iterator& KonqTreeViewWidget::iterator::operator++()
{
  if ( !m_p ) return *this;
  KonqTreeViewItem *i = (KonqTreeViewItem*)m_p->firstChild();
  if ( i )
  {
    m_p = i;
    return *this;
  }
  i = (KonqTreeViewItem*)m_p->nextSibling();
  if ( i )
  {
    m_p = i;
    return *this;
  }
  m_p = (KonqTreeViewItem*)m_p->parent();
  if ( m_p )
    m_p = (KonqTreeViewItem*)m_p->nextSibling();

  return *this;
}

KonqTreeViewWidget::iterator KonqTreeViewWidget::iterator::operator++(int)
{
  KonqTreeViewWidget::iterator it = *this;
  if ( !m_p ) return it;
  KonqTreeViewItem *i = (KonqTreeViewItem*)m_p->firstChild();
  if ( i )
  {
    m_p = i;
    return it;
  }
  i = (KonqTreeViewItem*)m_p->nextSibling();
  if ( i )
  {
    m_p = i;
    return it;
  }
  m_p = (KonqTreeViewItem*)m_p->parent();
  if ( m_p )
    m_p = (KonqTreeViewItem*)m_p->nextSibling();

  return it;
}

void KonqTreeViewWidget::drawContentsOffset( QPainter* _painter, int _offsetx, int _offsety,
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

void KonqTreeViewWidget::focusInEvent( QFocusEvent* _event )
{
//  emit gotFocus();

  QListView::focusInEvent( _event );
}

static KonqTreeViewDir * lasttvd = 0L;

KonqTreeViewDir * KonqTreeViewWidget::findDir( const QString &_url )
{
  if ( lasttvd && urlcmp( lasttvd->url(0), _url, true, true ) )
    return lasttvd;

  QDictIterator<KonqTreeViewDir> it( m_mapSubDirs );
  for( ; it.current(); ++it )
  {
    debug( it.current()->url(0) );
    if ( urlcmp( it.current()->url(0), _url, true, true ) ) 
      {
        lasttvd = it.current();
        return it.current();
      }           
  }
  return 0L;
}

#include "konq_treeviewwidget.moc"
