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
#include "konq_propsview.h"

#include <kio_job.h>
#include <kio_error.h>
#include <kpixmapcache.h>
#include <kmimetypes.h>
#include <krun.h>
#include <kdirwatch.h>
#include <kcursor.h>
#include <kprotocolmanager.h>

#include <assert.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include <kurl.h>
#include <kapp.h>

#include <qmsgbox.h>
#include <qkeycode.h>
#include <qlist.h>
#include <qdragobject.h>
#include <qheader.h>
#include <klocale.h>
#include <drag.h>

#include <opUIUtils.h>

KonqKfmTreeView::KonqKfmTreeView()
{
  kdebug(0, 1202, "+KonqKfmTreeView");
  ADD_INTERFACE( "IDL:Konqueror/KfmTreeView:1.0" );

  setWidget( this );

  QWidget::setFocusPolicy( StrongFocus );
  // viewport()->setFocusPolicy( StrongFocus );
  // setFocusPolicy( ClickFocus );
  setMultiSelection( true );

  m_bIsLocalURL        = false;
  m_pWorkingDir        = 0L;
  m_bTopLevelComplete  = true;
  m_bSubFolderComplete = true;
  m_iColumns           = -1;
  m_jobId              = 0;
  m_dragOverItem       = 0L;
  m_pressed            = false;
  m_pressedItem        = 0L;
  m_handCursor         = KCursor().handCursor();
  m_stdCursor          = KCursor().arrowCursor();
  m_overItem           = "";

  setRootIsDecorated( true );
  setTreeStepSize( 20 );
  setSorting( 1 );

  initConfig();

  QObject::connect( &m_bufferTimer, SIGNAL( timeout() ),
	   this, SLOT( slotBufferTimeout() ) );
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

  QObject::connect( kdirwatch, SIGNAL( dirty( const char* ) ),
	   this, SLOT( slotDirectoryDirty( const char* ) ) );

  //  connect( m_pView->gui(), SIGNAL( configChanged() ), SLOT( initConfig() ) );

  // Qt 1.41 hack to get drop events
  viewport()->installEventFilter( this );
  viewport()->setAcceptDrops( true );
  viewport()->setMouseTracking( true );

  m_dragOverItem = 0L;
}

KonqKfmTreeView::~KonqKfmTreeView()
{
  kdebug(0, 1202, "-KonqKfmTreeView");
  /* viewport()->removeEventFilter( this );
  kdirwatch->disconnect( this ); */

  iterator it = begin();
  for( ; it != end(); ++it )
    it->prepareToDie();

  // Stop running jobs
  if ( m_jobId )
  {
    KIOJob* job = KIOJob::find( m_jobId );
    if ( job )
      job->kill();
    m_jobId = 0;
  }
}

bool KonqKfmTreeView::mappingOpenURL( Konqueror::EventOpenURL eventURL )
{
  KonqBaseView::mappingOpenURL( eventURL );
  openURL( eventURL.url );
  return true;
}

bool KonqKfmTreeView::mappingFillMenuView( Konqueror::View::EventFillMenu viewMenu )
{
#define MVIEW_RELOADTREE_ID 1483 // temporary
  
  if ( !CORBA::is_nil( viewMenu.menu ) )
  {
    if ( viewMenu.create )
    {
      kdebug(0,1202,"Adding reload tree");
      CORBA::WString_var text = Q2C( i18n("Rel&oad Tree") );
      viewMenu.menu->insertItem4( text, this, "slotReloadTree", 0, MVIEW_RELOADTREE_ID, -1 );
    }
    else
    {
      kdebug(0,1202,"Removing reload tree");
      viewMenu.menu->removeItem( MVIEW_RELOADTREE_ID );
    }
  }
  
  return true;
}

bool KonqKfmTreeView::mappingFillMenuEdit( Konqueror::View::EventFillMenu editMenu )
{
  // TODO : add select and selectall
  return true;
}

void KonqKfmTreeView::stop()
{
  //TODO
}

char *KonqKfmTreeView::url()
{
  QString u = m_strWorkingURL.c_str();
  if ( u.isEmpty() )
    u = m_strURL;
  return CORBA::string_dup( u.ascii() );
}

void KonqKfmTreeView::slotReloadTree()
{
  QString u = m_strWorkingURL.c_str();
  if ( u.isEmpty() )
    u = m_strURL;
  openURL( u.ascii() );
}

void KonqKfmTreeView::slotShowDot()
{
  // TODO
}

void KonqKfmTreeView::openURLRequest( const char *_url )
{
  Konqueror::URLRequest urlRequest;
  urlRequest.url = CORBA::string_dup( _url );
  urlRequest.reload = (CORBA::Boolean)false;
  urlRequest.xOffset = 0;
  urlRequest.yOffset = 0;
  SIGNAL_CALL1( "openURL", urlRequest );
}

void KonqKfmTreeView::initConfig()
{
  QPalette p          = viewport()->palette();
//  KfmViewSettings *settings  = m_pView->settings();
//  KonqPropsView *props = m_pView->props();

  KConfig *config = kapp->getConfig();
  config->setGroup("Settings");

  KfmViewSettings *settings = new KfmViewSettings( config );
  KonqPropsView *props = new KonqPropsView( config );


  m_bgColor           = settings->bgColor();
  m_textColor         = settings->textColor();
  m_linkColor         = settings->linkColor();
  m_vLinkColor        = settings->vLinkColor();
  m_stdFontName       = settings->stdFontName();
  m_fixedFontName     = settings->fixedFontName();
  m_fontSize          = settings->fontSize();

//   m_bgPixmap          = props->bgPixmap();

//   if ( m_bgPixmap.isNull() )
//     viewport()->setBackgroundMode( PaletteBackground );
//   else
//     viewport()->setBackgroundMode( NoBackground );

  m_mouseMode         = settings->mouseMode();

  m_underlineLink     = settings->underlineLink();
  m_changeCursor      = settings->changeCursor();
  m_isShowingDotFiles = props->isShowingDotFiles();

  QColorGroup c = p.normal();
  QColorGroup n( m_textColor, m_bgColor, c.light(), c.dark(), c.mid(),
		 m_textColor, m_bgColor );
  p.setNormal( n );
  c = p.active();
  QColorGroup a( m_textColor, m_bgColor, c.light(), c.dark(), c.mid(),
		 m_textColor, m_bgColor );
  p.setActive( a );
  c = p.disabled();
  QColorGroup d( m_textColor, m_bgColor, c.light(), c.dark(), c.mid(),
		 m_textColor, m_bgColor );
  p.setDisabled( d );
  viewport()->setPalette( p );
//   viewport()->setBackgroundColor( m_bgColor );

  QFont font( m_stdFontName, m_fontSize );
  setFont( font );

  delete settings;
  delete props;
}

void KonqKfmTreeView::setBgColor( const QColor& _color )
{
  m_bgColor = _color;

  update();
}

void KonqKfmTreeView::setTextColor( const QColor& _color )
{
  m_textColor = _color;

  update();
}

void KonqKfmTreeView::setLinkColor( const QColor& _color )
{
  m_linkColor = _color;

  update();
}

void KonqKfmTreeView::setVLinkColor( const QColor& _color )
{
  m_vLinkColor = _color;

  update();
}

void KonqKfmTreeView::setStdFontName( const char *_name )
{
  m_stdFontName = _name;

  update();
}

void KonqKfmTreeView::setFixedFontName( const char *_name )
{
  m_fixedFontName = _name;

  update();
}

void KonqKfmTreeView::setFontSize( const int _size )
{
  m_fontSize = _size;

  update();
}

void KonqKfmTreeView::setBgPixmap( const QPixmap& _pixmap )
{
  m_bgPixmap = _pixmap;

  if ( m_bgPixmap.isNull() )
    viewport()->setBackgroundMode( PaletteBackground );
  else
    viewport()->setBackgroundMode( NoBackground );

  update();
}

void KonqKfmTreeView::setUnderlineLink( bool _underlineLink )
{
  m_underlineLink = _underlineLink;

  update();
}

void KonqKfmTreeView::setShowingDotFiles( bool _isShowingDotFiles )
{
  m_isShowingDotFiles = _isShowingDotFiles;

  update();
}

bool KonqKfmTreeView::eventFilter( QObject *o, QEvent *e )
{
  if ( o != viewport() )
     return false;

  if ( e->type() == QEvent::Drop )
  {
    dropEvent( (QDropEvent*)e );
    return true;
  }
  else if ( e->type() == QEvent::DragEnter )
  {
    dragEnterEvent( (QDragEnterEvent*)e );
    return true;
  }
  else if ( e->type() == QEvent::DragLeave )
  {
    dragLeaveEvent( (QDragLeaveEvent*)e );
    return true;
  }
  else if ( e->type() == QEvent::DragMove )
  {
    dragMoveEvent( (QDragMoveEvent*)e );
    return true;
  }

  return QListView::eventFilter( o, e );
}

void KonqKfmTreeView::dragMoveEvent( QDragMoveEvent *_ev )
{
  KfmTreeViewItem *item = (KfmTreeViewItem*)itemAt( _ev->pos() );
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

  if ( item->acceptsDrops( m_lstDropFormats ) )
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

void KonqKfmTreeView::dragEnterEvent( QDragEnterEvent *_ev )
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

void KonqKfmTreeView::dragLeaveEvent( QDragLeaveEvent * )
{
  if ( m_dragOverItem != 0L )
    setSelected( m_dragOverItem, false );
  m_dragOverItem = 0L;

  /** DEBUG CODE */
  // Give the user some feedback...
  /** End DEBUG CODE */
}

void KonqKfmTreeView::dropEvent( QDropEvent * e )
{
  if ( m_dragOverItem != 0L )
    setSelected( m_dragOverItem, false );
  m_dragOverItem = 0L;
}

void KonqKfmTreeView::slotDirectoryDirty( const char *_dir )
{
  QString f = _dir;
  KURL::encode( f );

  QDictIterator<KfmTreeViewDir> it( m_mapSubDirs );
  for( ; it.current(); ++it )
  {
    if ( urlcmp( it.current()->url(0).c_str(), f, true, true ) )
    {
      updateDirectory( it.current(), it.current()->url(0).c_str() );
      return;
    }
  }
}

void KonqKfmTreeView::addSubDir( const char* _url, KfmTreeViewDir* _dir )
{
  m_mapSubDirs.insert( _url, _dir );

  if ( isLocalURL() )
  {
    KURL u( _url );
    kdirwatch->addDir( u.path(0) );
  }
}

void KonqKfmTreeView::removeSubDir( const char *_url )
{
  m_mapSubDirs.remove( _url );

  if ( isLocalURL() )
  {
    KURL u( _url );
    kdirwatch->removeDir( u.path(0) );
  }
}

void KonqKfmTreeView::keyPressEvent( QKeyEvent *_ev )
{
  // We are only interested in the escape key here
  if ( _ev->key() != Key_Escape )
  {
    QListView::keyPressEvent( _ev );
    return;
  }

  KfmTreeViewItem* item = (KfmTreeViewItem*)currentItem();

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

void KonqKfmTreeView::mousePressEvent( QMouseEvent *_ev )
{
  m_pressed = true;
  m_pressedPos = _ev->pos();
  m_pressedItem = (KfmTreeViewItem*)itemAt( _ev->pos() );

//  if ( !hasFocus() )
//    setFocus();

  if ( m_pressedItem )
  {
    if ( m_mouseMode == SingleClick )
    {
      if ( _ev->button() == LeftButton && !( _ev->state() & ControlButton ) )
      {
	if ( !m_overItem.isEmpty() )
	{
	  //reset to standard cursor
	  setCursor(m_stdCursor);
	  m_overItem = "";
	}
	
	iterator it = begin();
	for( ; it != end(); it++ )
	  if ( it->isSelected() )
	    QListView::setSelected( &*it, false );
	
	QListView::setSelected( m_pressedItem, true );
	//      m_pressedItem->returnPressed();
	//	QListView::mousePressEvent( _ev );
	return;
      }
    }
    else if ( m_mouseMode == DoubleClick )
    {
      if ( !m_pressedItem->isSelected() && _ev->button() == LeftButton && !( _ev->state() & ControlButton ) )
      {
	iterator it = begin();
	for( ; it != end(); it++ )
	  if ( it->isSelected() )
	    QListView::setSelected( &*it, false );
	
	QListView::mousePressEvent( _ev );
	return;
      }
      else if ( m_pressedItem->isSelected() && _ev->button() == LeftButton && !( _ev->state() & ControlButton ) )
      {
	QListView::mousePressEvent( _ev );
	// Fix what Qt destroyed :-)
	setSelected( m_pressedItem, true );
	return;
      }
    }

    if ( _ev->button() == RightButton )
    {
      m_pressed = false;
      m_pressedItem = 0L;
    }
  }
  /*
  else
    assert( 0 );
    */

   QListView::mousePressEvent( _ev );
}

void KonqKfmTreeView::mouseReleaseEvent( QMouseEvent *_mouse )
{
  if ( !m_pressed )
    return;

  if ( m_mouseMode == SingleClick &&
       _mouse->button() == LeftButton &&
       !( _mouse->state() & ControlButton ) &&
       isSingleClickArea( _mouse->pos() ) )
  {
    if ( m_pressedItem->isExpandable() )
      m_pressedItem->setOpen( !m_pressedItem->isOpen() );

    slotReturnPressed( m_pressedItem );
  }

  m_pressed = false;
  m_pressedItem = 0L;
}

void KonqKfmTreeView::mouseMoveEvent( QMouseEvent *_mouse )
{
  if ( !m_pressed || !m_pressedItem )
  {
    if ( isSingleClickArea( _mouse->pos() ) )
    {
      KfmTreeViewItem* item = (KfmTreeViewItem*)itemAt( _mouse->pos() );

      if ( m_overItem != item->name() )
      {
	slotOnItem( item );
	m_overItem = item->name();

	if ( m_mouseMode == SingleClick && m_changeCursor )
	  setCursor( m_handCursor );
      }
    }
    else if ( !m_overItem.isEmpty() )
    {
      slotOnItem( 0L );

      setCursor( m_stdCursor );
      m_overItem = "";
    }

    return;
  }

  int x = _mouse->pos().x();
  int y = _mouse->pos().y();

  if ( abs( x - m_pressedPos.x() ) > Dnd_X_Precision || abs( y - m_pressedPos.y() ) > Dnd_Y_Precision )
  {
    // Collect all selected items
    QStrList urls;
    iterator it = begin();
    for( ; it != end(); it++ )
      if ( it->isSelected() )
	urls.append( it->url() );

    // Multiple URLs ?
    QPixmap pixmap2;
    if ( urls.count() > 1 )
    {
      pixmap2 = *( KPixmapCache::pixmap( "kmultiple.xpm", true ) );
      if ( pixmap2.isNull() )
	warning("KDesktop: Could not find kmultiple pixmap\n");
    }

    // Calculate hotspot
    QPoint hotspot;

    // Do not handle and more mouse move or mouse release events
    m_pressed = false;

    QUrlDrag *d = new QUrlDrag( urls, viewport() );
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

bool KonqKfmTreeView::isSingleClickArea( const QPoint& _point )
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

void KonqKfmTreeView::slotOnItem( KfmTreeViewItem* _item)
{
  QString s;
  if ( _item )
    s = _item->getStatusBarInfo();
  CORBA::WString_var ws = Q2C( s );
  SIGNAL_CALL1( "setStatusBarText", CORBA::Any::from_wstring( ws.out(), 0 ) );
}

void KonqKfmTreeView::selectedItems( list<KfmTreeViewItem*>& _list )
{
  iterator it = begin();
  for( ; it != end(); it++ )
    if ( it->isSelected() )
      _list.push_back( &*it );
}

void KonqKfmTreeView::slotReturnPressed( QListViewItem *_item )
{
  if ( !_item )
    return;

  KfmTreeViewItem *item = (KfmTreeViewItem*)_item;
  UDSEntry entry = item->udsEntry();
  mode_t mode = 0;

  UDSEntry::iterator it = entry.begin();
  for( ; it != entry.end(); it++ )
    if ( it->m_uds == UDS_FILE_TYPE )
    {
      mode = (mode_t)it->m_long;
      break;
    }

  //execute only if item is not a directory or a link
  // why ? a link to a file would be ok, I think (David)
  if ( S_ISREG( mode ) )
    openURLRequest( item->url().ascii() ); //FIXME: obey mode/m_bIsLocalURL 
}

void KonqKfmTreeView::slotRightButtonPressed( QListViewItem *_item, const QPoint &_global, int _column )
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

  if ( !_item )
  {
    // Popup menu for m_url
    // Torben: I think this is impossible in the treeview, or ?
    //         Perhaps if the list is smaller than the window height.
  }
  else
  {
    popupMenu( _global );
  }
}

void KonqKfmTreeView::popupMenu( const QPoint& _global )
{
  Konqueror::View::MenuPopupRequest popupRequest;

  popupRequest.urls.length( 0 );

  list<KfmTreeViewItem*> items;
  selectedItems( items );
  mode_t mode = 0;
  bool first = true;
  list<KfmTreeViewItem*>::iterator itit = items.begin();
  int i = 0;
  for( ; itit != items.end(); ++itit )
  {
    popupRequest.urls.length( i + 1 );
    popupRequest.urls[ i++ ] = (*itit)->url();

    UDSEntry::iterator it = (*itit)->udsEntry().begin();
    for( ; it != (*itit)->udsEntry().end(); it++ )
      if ( it->m_uds == UDS_FILE_TYPE )
      {
	if ( first )
	{
	  mode = (mode_t)it->m_long;
	  first = false;
	}
	else if ( mode != (mode_t)it->m_long )
	    mode = 0;
      }
  }

  popupRequest.x = _global.x();
  popupRequest.y = _global.y();
  popupRequest.mode = mode;
  popupRequest.isLocalFile = (CORBA::Boolean)m_bIsLocalURL;
  SIGNAL_CALL1( "popupMenu", popupRequest );
}

void KonqKfmTreeView::openURL( const char *_url )
{
  bool isNewProtocol = false;

  KURL url( _url );
  if ( url.isMalformed() )
  {
    QString tmp = i18n( "Malformed URL\n%1" ).arg( _url );
    QMessageBox::critical( this, i18n( "Error" ), tmp, i18n( "OK" ) );
    return;
  }

  //test if we are switching to a new protocol
  if ( !m_url.url().isEmpty() )
  {
    if ( strcmp( m_url.protocol(), url.protocol() ) != 0 )
      isNewProtocol = true;
  }

  // The first time or new protocol ? So create the columns first
  if ( m_iColumns == -1 || isNewProtocol )
  {
    QStringList listing = KProtocolManager::self().listing( url.protocol() );
    if ( listing.isEmpty() )
    {
      QString tmp = i18n( "Unknown Protocol %1" ).arg( url.protocol());
      QMessageBox::critical( this, i18n( "Error" ), tmp, i18n( "OK" ) );
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

  // TODO: Check wether the URL is really a directory

  // Stop running jobs
  if ( m_jobId )
  {
    KIOJob* job = KIOJob::find( m_jobId );
    if ( job )
      job->kill();
    m_jobId = 0;
  }

  m_strWorkingURL = _url;
  m_workingURL = url;
  m_bTopLevelComplete = false;

  KIOJob* job = new KIOJob;
  QObject::connect( job, SIGNAL( sigListEntry( int, const UDSEntry& ) ), this, SLOT( slotListEntry( int, const UDSEntry& ) ) );
  QObject::connect( job, SIGNAL( sigFinished( int ) ), this, SLOT( slotCloseURL( int ) ) );
  QObject::connect( job, SIGNAL( sigError( int, int, const char* ) ),
	   this, SLOT( slotError( int, int, const char* ) ) );

  m_jobId = job->id();
  job->listDir( _url );

  SIGNAL_CALL2( "started", id(), CORBA::Any::from_string( (char *)m_strWorkingURL.c_str(), 0 ) );
}

void KonqKfmTreeView::slotError( int /*_id*/, int _errid, const char* _errortext )
{
  kioErrorDialog( _errid, _errortext );
}

void KonqKfmTreeView::slotCloseURL( int /*_id*/ )
{
  if ( m_bufferTimer.isActive() )
  {
    m_bufferTimer.stop();
    slotBufferTimeout();
  }

  m_jobId = 0;
  m_bTopLevelComplete = true;

  SIGNAL_CALL1( "completed", id() );
}

void KonqKfmTreeView::openSubFolder( const char *_url, KfmTreeViewDir* _dir )
{
  KURL url( _url );
  if ( url.isMalformed() )
  {
    string tmp = i18n( "Malformed URL" ).ascii();;
    tmp += "\n";
    tmp += _url;
    QMessageBox::critical( this, i18n( "Error" ), tmp.c_str(), i18n( "OK" ) );
    return;
  }

  if ( !m_bTopLevelComplete )
  {
    // TODO: Give a warning
    kdebug(0,1202,"Still waiting for toplevel directory");
    return;
  }

  // If we are opening another sub folder right now, stop it.
  if ( !m_bSubFolderComplete )
  {
    KIOJob* job = KIOJob::find( m_jobId );
    if ( job )
      job->kill();
    m_bSubFolderComplete = true;
    m_pWorkingDir = 0L;
    m_jobId = 0;
  }

  /** Debug code **/
  assert( m_iColumns != -1 && !m_url.url().isEmpty() );
  if ( strcmp( m_url.protocol(), url.protocol() ) != 0 )
    assert( 0 );
  /** End Debug code **/

  assert( m_bSubFolderComplete && !m_pWorkingDir && !m_jobId );

  m_bSubFolderComplete = false;
  m_pWorkingDir = _dir;
  m_strWorkingURL = _url;
  m_workingURL = url;

  KIOJob* job = new KIOJob;
  QObject::connect( job, SIGNAL( sigListEntry( int, const UDSEntry& ) ), this, SLOT( slotListEntry( int, const UDSEntry& ) ) );
  QObject::connect( job, SIGNAL( sigFinished( int ) ), this, SLOT( slotCloseSubFolder( int ) ) );
  QObject::connect( job, SIGNAL( sigError( int, int, const char* ) ),
	   this, SLOT( slotError( int, int, const char* ) ) );

  m_jobId = job->id();
  job->listDir( _url );

  SIGNAL_CALL2( "started", id(), CORBA::Any::from_string( (char *)0L, 0 ) );
}

void KonqKfmTreeView::slotCloseSubFolder( int /*_id*/ )
{
  assert( !m_bSubFolderComplete && m_pWorkingDir );

  if ( m_bufferTimer.isActive() )
  {
    m_bufferTimer.stop();
    slotBufferTimeout();
  }

  m_bSubFolderComplete = true;
  m_pWorkingDir->setComplete( true );
  m_pWorkingDir = 0L;
  m_jobId = 0;

  SIGNAL_CALL1( "completed", id() );
}

void KonqKfmTreeView::slotListEntry( int /*_id*/, const UDSEntry& _entry )
{
  m_buffer.push_back( _entry );
  if ( !m_bufferTimer.isActive() )
    m_bufferTimer.start( 1000, true );
}

void KonqKfmTreeView::slotBufferTimeout()
{
  list<UDSEntry>::iterator it = m_buffer.begin();
  for( ; it != m_buffer.end(); it++ )
  {
    int isdir = -1;
    string name;

    // Find out whether it is a directory
    UDSEntry::iterator it2 = it->begin();
    for( ; it2 != it->end(); it2++ )
    {
      if ( it2->m_uds == UDS_FILE_TYPE )
      {
	mode_t mode = (mode_t)it2->m_long;
	if ( S_ISDIR( mode ) )
	  isdir = 1;
	else
	  isdir = 0;
      }
      else if ( it2->m_uds == UDS_NAME )
      {
	name = it2->m_str;
      }
    }

    assert( isdir != -1 && !name.empty() );

    if ( m_isShowingDotFiles || name[0]!='.' )
    {
      // The first entry in the toplevel ?
      if ( !m_pWorkingDir && !m_strWorkingURL.empty() )
      {
        clear();

        m_strURL = m_strWorkingURL.c_str();
        m_strWorkingURL = "";
        m_url = m_strURL.data();
        KURL u( m_url );
        m_bIsLocalURL = u.isLocalFile();
      }

      if ( m_pWorkingDir )
      {
        KURL u( m_workingURL );
        u.addPath( name.c_str() );
        if ( isdir )
          new KfmTreeViewDir( this, m_pWorkingDir, *it, u );
        else
          new KfmTreeViewItem( this, m_pWorkingDir, *it, u );
      }
      else
      {
        KURL u( m_url );
        u.addPath( name.c_str() );
        if ( isdir )
          new KfmTreeViewDir( this, *it, u );
        else
          new KfmTreeViewItem( this, *it, u );
      }
    }
  }

  m_buffer.clear();
}

KonqKfmTreeView::iterator& KonqKfmTreeView::iterator::operator++()
{
  if ( !m_p ) return *this;
  KfmTreeViewItem *i = (KfmTreeViewItem*)m_p->firstChild();
  if ( i )
  {
    m_p = i;
    return *this;
  }
  i = (KfmTreeViewItem*)m_p->nextSibling();
  if ( i )
  {
    m_p = i;
    return *this;
  }
  m_p = (KfmTreeViewItem*)m_p->parent();
  if ( m_p )
    m_p = (KfmTreeViewItem*)m_p->nextSibling();

  return *this;
}

KonqKfmTreeView::iterator KonqKfmTreeView::iterator::operator++(int)
{
  KonqKfmTreeView::iterator it = *this;
  if ( !m_p ) return it;
  KfmTreeViewItem *i = (KfmTreeViewItem*)m_p->firstChild();
  if ( i )
  {
    m_p = i;
    return it;
  }
  i = (KfmTreeViewItem*)m_p->nextSibling();
  if ( i )
  {
    m_p = i;
    return it;
  }
  m_p = (KfmTreeViewItem*)m_p->parent();
  if ( m_p )
    m_p = (KfmTreeViewItem*)m_p->nextSibling();

  return it;
}

void KonqKfmTreeView::updateDirectory()
{
  if ( !m_bTopLevelComplete || !m_bSubFolderComplete )
    return;

  updateDirectory( 0L, m_strURL.data() );
}

void KonqKfmTreeView::updateDirectory( KfmTreeViewDir *_dir, const char *_url )
{
  // Stop running jobs
  if ( m_jobId )
  {
    KIOJob* job = KIOJob::find( m_jobId );
    if ( job )
      job->kill();
    m_jobId = 0;
  }

  m_buffer.clear();

  if ( _dir )
    m_bSubFolderComplete = false;
  else
    m_bTopLevelComplete = false;

  m_pWorkingDir = _dir;
  m_workingURL = _url;
  m_strWorkingURL = _url;

  KIOJob* job = new KIOJob;
  QObject::connect( job, SIGNAL( sigListEntry( int, const UDSEntry& ) ), this,
	   SLOT( slotUpdateListEntry( int, const UDSEntry& ) ) );
  QObject::connect( job, SIGNAL( sigFinished( int ) ), this, SLOT( slotUpdateFinished( int ) ) );
  QObject::connect( job, SIGNAL( sigError( int, int, const char* ) ),
	   this, SLOT( slotUpdateError( int, int, const char* ) ) );

  m_jobId = job->id();
  job->listDir( _url );

  SIGNAL_CALL2( "started", id(), CORBA::Any::from_string( (char *)0L, 0 ) );
}

void KonqKfmTreeView::slotUpdateError( int /*_id*/, int _errid, const char *_errortext )
{
  kioErrorDialog( _errid, _errortext );

  if ( m_pWorkingDir )
    m_bSubFolderComplete = true;
  else
    m_bTopLevelComplete = true;
  m_pWorkingDir = 0L;

//  emit canceled();
  SIGNAL_CALL0( "canceled" );
}

void KonqKfmTreeView::slotUpdateFinished( int /*_id*/ )
{
  if ( m_bufferTimer.isActive() )
  {
    m_bufferTimer.stop();
    slotBufferTimeout();
  }

  m_jobId = 0;
  if ( m_pWorkingDir )
    m_bSubFolderComplete = true;
  else
    m_bTopLevelComplete = true;

  // Unmark all items
  QListViewItem *item;
  if ( m_pWorkingDir )
    item = m_pWorkingDir->firstChild();
  else
    item = firstChild();
  while( item )
  {
    ((KfmTreeViewItem*)item)->unmark();
    item = item->nextSibling();
  }

  list<UDSEntry>::iterator it = m_buffer.begin();
  for( ; it != m_buffer.end(); it++ )
  {
    int isdir = -1;
    string name;

    // Find out wether it is a directory
    UDSEntry::iterator it2 = it->begin();
    for( ; it2 != it->end(); it2++ )
    {
      if ( it2->m_uds == UDS_FILE_TYPE )
      {
	mode_t mode = (mode_t)it2->m_long;
	if ( S_ISDIR( mode ) )
	  isdir = 1;
	else
	  isdir = 0;
      }
      else if ( it2->m_uds == UDS_NAME )
      {
	name = it2->m_str;
      }
    }

    assert( isdir != -1 && !name.empty() );

    if ( m_isShowingDotFiles || name[0]!='.' )
    {
      // Find this icon
      bool done = false;
      QListViewItem *item;
      if ( m_pWorkingDir )
        item = m_pWorkingDir->firstChild();
      else
        item = firstChild();
      while( item )
      {
        if ( name == ((KfmTreeViewItem*)item)->name().ascii() )
        {
          ((KfmTreeViewItem*)item)->mark();
          done = true;
        }
        item = item->nextSibling();
      }

      if ( !done )
      {
        kdebug(0,1202,"Inserting %s", name.c_str());
        KfmTreeViewItem *item;
        if ( m_pWorkingDir )
        {
          KURL u( m_workingURL );
          u.addPath( name.c_str() );
          kdebug(0,1202,"Final path 1 '%s'", u.path().data());
          if ( isdir )
            item = new KfmTreeViewDir( this, m_pWorkingDir, *it, u );
          else
            item = new KfmTreeViewItem( this, m_pWorkingDir, *it, u );
        }
        else
        {
          KURL u( m_url );
          u.addPath( name.c_str() );
          kdebug(0,1202,"Final path 2 '%s'", u.path().data());
          if ( isdir )
            item = new KfmTreeViewDir( this, *it, u );
          else
            item = new KfmTreeViewItem( this, *it, u );
        }
        item->mark();
      }
    }
  }

  // Find all unmarked items and delete them
  QList<QListViewItem> lst;

  if ( m_pWorkingDir )
    item = m_pWorkingDir->firstChild();
  else
    item = firstChild();
  while( item )
  {
    if ( !((KfmTreeViewItem*)item)->isMarked() )
    {
      kdebug(0,1202,"Removing %s", ((KfmTreeViewItem*)item)->name().data());
      lst.append( item );
    }
    item = item->nextSibling();
  }

  QListViewItem* qlvi;
  for( qlvi = lst.first(); qlvi != 0L; qlvi = lst.next() )
    delete qlvi;

  m_buffer.clear();
  m_pWorkingDir = 0L;

  SIGNAL_CALL1( "completed", id() );
}

void KonqKfmTreeView::slotUpdateListEntry( int /*_id*/, const UDSEntry& _entry )
{
  m_buffer.push_back( _entry );
}

void KonqKfmTreeView::drawContentsOffset( QPainter* _painter, int _offsetx, int _offsety,
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

void KonqKfmTreeView::focusInEvent( QFocusEvent* _event )
{
//  emit gotFocus();

  QListView::focusInEvent( _event );
}

/**************************************************************
 *
 * KfmTreeViewItem
 *
 **************************************************************/

KfmTreeViewItem::KfmTreeViewItem( KonqKfmTreeView *_treeview, KfmTreeViewDir *_parent, UDSEntry& _entry, KURL& _url )
  : QListViewItem( _parent ), KonqKfmViewItem( _entry, _url )
{
  m_pTreeView = _treeview;
  init();
}

KfmTreeViewItem::KfmTreeViewItem( KonqKfmTreeView *_parent, UDSEntry& _entry, KURL& _url )
  : QListViewItem( _parent ), KonqKfmViewItem( _entry, _url )
{
  m_pTreeView = _parent;
  init();
}

void KfmTreeViewItem::init()
{
  QPixmap * p = KPixmapCache::pixmapForMimeType( m_pMimeType, m_url, m_bIsLocalURL, true );
  if (!p) warning("Pixmap not found for mimetype %s",m_pMimeType->name().ascii());
  else setPixmap( 0, *p );
}


/*
void KfmTreeViewItem::popupMenu( const QPoint &_global, int _column )
{
  mode_t mode = 0;
  UDSEntry::iterator it = m_entry.begin();
  for( ; it != m_entry.end(); it++ )
    if ( it->m_uds == UDS_FILE_TYPE )
      mode = (mode_t)it->m_long;

//  m_pFinder->setSelected( this, true );
  m_pTreeView->setSelected( this, true );

  QStrList urls;
  urls.append( m_strURL.c_str() );
  m_pFinder->view()->popupMenu( _global, urls, mode, m_bIsLocalURL );
}
*/

QString KfmTreeViewItem::key( int _column, bool ) const
{
  static char buffer[ 12 ];

  assert( _column < (int)m_entry.size() );

  unsigned long uds = m_entry[ _column ].m_uds;

  if ( uds & UDS_STRING )
    return m_entry[ _column ].m_str.c_str();
  else if ( uds & UDS_LONG )
  {
    sprintf( buffer, "%08d", (int)m_entry[ _column ].m_long );
    return buffer;
  }
  else
    assert( 0 );
}

QString KfmTreeViewItem::text( int _column ) const
{
  //  assert( _column < (int)m_entry.size() );

  if ( _column >= (int)m_entry.size() )
    return "";

  unsigned long uds = m_entry[ _column ].m_uds;

  if ( uds == UDS_ACCESS )
    return makeAccessString( m_entry[ _column ] );
  else if ( uds == UDS_FILE_TYPE )
    return makeTypeString( m_entry[ _column ] );
  else if ( ( uds & UDS_TIME ) == UDS_TIME )
    return makeTimeString( m_entry[ _column ] );
  else if ( uds & UDS_STRING )
    return m_entry[ _column ].m_str.c_str();
  else if ( uds & UDS_LONG )
    return makeNumericString( m_entry[ _column ] );
  else
    assert( 0 );
}

const char* KfmTreeViewItem::makeNumericString( const UDSAtom &_atom ) const
{
  static char buffer[ 100 ];
  sprintf( buffer, "%i", (int)_atom.m_long );
  return buffer;
}

const char* KfmTreeViewItem::makeTimeString( const UDSAtom &_atom ) const
{
  static char buffer[ 100 ];

  time_t t = (time_t)_atom.m_long;
  struct tm* t2 = localtime( &t );

  strftime( buffer, 100, "%c", t2 );

  return buffer;
}

QString KfmTreeViewItem::makeTypeString( const UDSAtom &_atom ) const
{
  mode_t mode = (mode_t) _atom.m_long;

  if ( S_ISLNK( mode ) )
    return i18n( "Link" );
  else if ( S_ISDIR( mode ) )
    return i18n( "Directory" );
  // TODO check for devices here, too
  else
    return i18n( "File" );
}

const char* KfmTreeViewItem::makeAccessString( const UDSAtom &_atom ) const
{
  static char buffer[ 12 ];

  mode_t mode = (mode_t) _atom.m_long;
	
  char uxbit,gxbit,oxbit;

  if ( (mode & (S_IXUSR|S_ISUID)) == (S_IXUSR|S_ISUID) )
    uxbit = 's';
  else if ( (mode & (S_IXUSR|S_ISUID)) == S_ISUID )
    uxbit = 'S';
  else if ( (mode & (S_IXUSR|S_ISUID)) == S_IXUSR )
    uxbit = 'x';
  else
    uxbit = '-';
	
  if ( (mode & (S_IXGRP|S_ISGID)) == (S_IXGRP|S_ISGID) )
    gxbit = 's';
  else if ( (mode & (S_IXGRP|S_ISGID)) == S_ISGID )
    gxbit = 'S';
  else if ( (mode & (S_IXGRP|S_ISGID)) == S_IXGRP )
    gxbit = 'x';
  else
    gxbit = '-';
	
  if ( (mode & (S_IXOTH|S_ISVTX)) == (S_IXOTH|S_ISVTX) )
    oxbit = 't';
  else if ( (mode & (S_IXOTH|S_ISVTX)) == S_ISVTX )
    oxbit = 'T';
  else if ( (mode & (S_IXOTH|S_ISVTX)) == S_IXOTH )
    oxbit = 'x';
  else
    oxbit = '-';

  buffer[0] = ((( mode & S_IRUSR ) == S_IRUSR ) ? 'r' : '-' );
  buffer[1] = ((( mode & S_IWUSR ) == S_IWUSR ) ? 'w' : '-' );
  buffer[2] = uxbit;
  buffer[3] = ((( mode & S_IRGRP ) == S_IRGRP ) ? 'r' : '-' );
  buffer[4] = ((( mode & S_IWGRP ) == S_IWGRP ) ? 'w' : '-' );
  buffer[5] = gxbit;
  buffer[6] = ((( mode & S_IROTH ) == S_IROTH ) ? 'r' : '-' );
  buffer[7] = ((( mode & S_IWOTH ) == S_IWOTH ) ? 'w' : '-' );
  buffer[8] = oxbit;
  buffer[9] = 0;

  return buffer;
}

void KfmTreeViewItem::paintCell( QPainter *_painter, const QColorGroup & cg, int column, int width, int alignment )
{
  if ( m_pTreeView->m_mouseMode == SingleClick &&
       m_pTreeView->m_underlineLink && column == 0)
  {
    QFont f = _painter->font();
    f.setUnderline( true );
    _painter->setFont( f );
  }
  QListViewItem::paintCell( _painter, cg, column, width, alignment );
}

/**************************************************************
 *
 * KfmTreeViewDir
 *
 **************************************************************/

KfmTreeViewDir::KfmTreeViewDir( KonqKfmTreeView *_parent, UDSEntry& _entry, KURL& _url )
  : KfmTreeViewItem( _parent, _entry, _url )
{
  QString url = _url.url();
  m_pTreeView->addSubDir( url, this );

  m_bComplete = false;
}

KfmTreeViewDir::KfmTreeViewDir( KonqKfmTreeView *_treeview, KfmTreeViewDir * _parent, UDSEntry& _entry, KURL& _url )
  : KfmTreeViewItem( _treeview, _parent, _entry, _url )
{
  QString url = _url.url();
  m_pTreeView->addSubDir( url, this );

  m_bComplete = false;
}

KfmTreeViewDir::~KfmTreeViewDir()
{
  if ( m_pTreeView )
    m_pTreeView->removeSubDir( m_url.url() );
}

void KfmTreeViewDir::setup()
{
  setExpandable( true );
  QListViewItem::setup();
}

void KfmTreeViewDir::setOpen( bool _open )
{
  if ( !_open || m_bComplete )
  {
    QListViewItem::setOpen( _open );
    return;
  }

  m_pTreeView->openSubFolder( m_url.url(), this );

  QListViewItem::setOpen( _open );
}

string KfmTreeViewDir::url( int _trailing )
{
  string tmp;
  tmp = m_url.url( _trailing );
  return tmp;
}

#include "konq_treeview.moc"

