/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#include "konq_iconview.h"
#include "kfmviewprops.h"

#include <assert.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include <kurl.h>
#include <kapp.h>
#include <kio_job.h>
#include <kio_error.h>
#include <kpixmapcache.h>
#include <krun.h>
#include <kio_paste.h>
#include <kmimetypes.h>

#include <qmsgbox.h>
#include <qkeycode.h>
#include <qpalette.h>
#include <qdragobject.h>

KonqKfmIconView::KonqKfmIconView( QWidget* _parent ) : KIconContainer( _parent )
{
  ADD_INTERFACE( "IDL:Konqueror/KfmIconView:1.0" );
  
  m_bInit = true;

  setWidget( this );

  QWidget::show();

  QWidget::setFocusPolicy( StrongFocus );
  viewport()->setFocusPolicy( StrongFocus );

  m_bIsLocalURL = false;
  m_bComplete = true;
  m_jobId = 0;

  initConfig();

  QObject::connect( &m_bufferTimer, SIGNAL( timeout() ), this, SLOT( slotBufferTimeout() ) );
  QObject::connect( this, SIGNAL( mousePressed( KIconContainerItem*, const QPoint&, int ) ),
	   this, SLOT( slotMousePressed( KIconContainerItem*, const QPoint&, int ) ) );
  QObject::connect( this, SIGNAL( doubleClicked( KIconContainerItem*, const QPoint&, int ) ),
	   this, SLOT( slotDoubleClicked( KIconContainerItem*, const QPoint&, int ) ) );
  QObject::connect( this, SIGNAL( returnPressed( KIconContainerItem*, const QPoint& ) ),
	   this, SLOT( slotReturnPressed( KIconContainerItem*, const QPoint& ) ) );
  QObject::connect( this, SIGNAL( dragStart( const QPoint&, QList<KIconContainerItem>&, QPixmap ) ),
	   this, SLOT( slotDragStart( const QPoint&, QList<KIconContainerItem>&, QPixmap ) ) );
  QObject::connect( this, SIGNAL( drop( QDropEvent*, KIconContainerItem*, QStrList& ) ),
	   this, SLOT( slotDrop( QDropEvent*, KIconContainerItem*, QStrList& ) ) );
  QObject::connect( this, SIGNAL( onItem( KIconContainerItem* ) ), this, SLOT( slotOnItem( KIconContainerItem* ) ) );
  //  connect( m_pView->gui(), SIGNAL( configChanged() ), SLOT( initConfig() ) );

  m_bInit = false;
}

KonqKfmIconView::~KonqKfmIconView()
{
  // Stop running jobs
  if ( m_jobId )
  {
    KIOJob* job = KIOJob::find( m_jobId );
    if ( job )
      job->kill();
    m_jobId = 0;
  }
}

bool KonqKfmIconView::mappingOpenURL( Konqueror::EventOpenURL eventURL )
{
  KonqBaseView::mappingOpenURL(eventURL);
  openURL( eventURL.url );
  return true;
}

bool KonqKfmIconView::mappingCreateViewMenu( Konqueror::View::EventCreateViewMenu viewMenu )
{
#define MVIEW_IMAGEPREVIEW_ID 1594 // temporary
#define MVIEW_SHOWDOT_ID 1595 // temporary

  if ( !CORBA::is_nil( viewMenu.menu ) )
  {
    if ( viewMenu.create )
    {
      //    menu->insertItem4( i18n("&Large Icons"), this, "slotLargeIcons", 0, -1, -1 );
      //    menu->insertItem4( i18n("&Small Icons"), this, "slotSmallIcons", 0, -1, -1 );
      kdebug(0,1202,"adding image preview and showdotfiles");
      viewMenu.menu->insertItem4( i18n("Image &Preview"), this, "slotShowSchnauzer" , 0, MVIEW_IMAGEPREVIEW_ID, -1 );
      viewMenu.menu->insertItem4( i18n("Show &Dot Files"), this, "slotShowDot" , 0, MVIEW_SHOWDOT_ID, -1 );
    }
    else
    {
      kdebug(0,1202,"removing image preview and showdotfiles");
      viewMenu.menu->removeItem( MVIEW_SHOWDOT_ID );
      viewMenu.menu->removeItem( MVIEW_IMAGEPREVIEW_ID );
    }
  }
  
  return true;
}

void KonqKfmIconView::slotLargeIcons()
{
  setDisplayMode( KIconContainer::Horizontal );
}

void KonqKfmIconView::slotSmallIcons()
{
  setDisplayMode( KIconContainer::Vertical );
}

void KonqKfmIconView::slotShowDot()
{
  // TODO
}

void KonqKfmIconView::stop()
{
  slotCloseURL( 0 ); // FIXME (just trying)
}

char *KonqKfmIconView::url()
{
  QString u= m_sWorkingURL;
  if ( u.isEmpty() )
    u = m_strURL;
  return CORBA::string_dup( u.ascii() );
}

void KonqKfmIconView::initConfig()
{
  QPalette p          = viewport()->palette();
//  KfmViewSettings *settings = m_pView->settings();
//  KfmViewProps *props = m_pView->props();

  KConfig *config = kapp->getConfig();
  config->setGroup("Settings");

  KfmViewSettings *settings = new KfmViewSettings( config );
  KfmViewProps *props = new KfmViewProps( config );

  m_bgColor           = settings->bgColor();
  m_textColor         = settings->textColor();
  m_linkColor         = settings->linkColor();
  m_vLinkColor        = settings->vLinkColor();
  m_stdFontName       = settings->stdFontName();
  m_fixedFontName     = settings->fixedFontName();
  m_fontSize          = settings->fontSize();

  m_bgPixmap          = props->bgPixmap(); // !!

  if ( m_bgPixmap.isNull() )
    viewport()->setBackgroundMode( PaletteBackground );
  else
    viewport()->setBackgroundMode( NoBackground );

  m_mouseMode = settings->mouseMode();

  m_underlineLink = settings->underlineLink();
  m_changeCursor = settings->changeCursor();
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

  QFont font( m_stdFontName, m_fontSize );
  setFont( font );

  if ( !m_bInit )
    refresh();

  delete settings;
  delete props;
}

void KonqKfmIconView::selectedItems( list<KonqKfmIconViewItem*>& _list )
{
  iterator it = KIconContainer::begin();
  for( ; it != KIconContainer::end(); it++ )
    if ( (*it)->isSelected() )
      _list.push_back( (KonqKfmIconViewItem*)&**it );
}

void KonqKfmIconView::slotReturnPressed( KIconContainerItem *_item, const QPoint &_global )
{
  if ( !_item )
    return;

  KonqKfmIconViewItem *item = (KonqKfmIconViewItem*)_item;
  openURLRequest( item->url().ascii() );
}

void KonqKfmIconView::slotMousePressed( KIconContainerItem *_item, const QPoint &_global, int _button )
{
  cerr << "void KonqKfmIconView::slotMousePressed( KIconContainerItem *_item, const QPoint &_global, int _button )" << endl;

  if ( !_item )
  {
    if ( _button == RightButton )
    {
      Konqueror::View::MenuPopupRequest popupRequest;
      QString cURL = m_url.url( 1 );
      int i = cURL.length();

      mode_t mode = 0;
      popupRequest.urls.length( 1 );
      popupRequest.urls[0] = cURL.ascii();

      // A url ending with '/' is always a directory
      if ( i >= 1 && cURL[ i - 1 ] == '/' )
	mode = S_IFDIR;
      // With HTTP we can be shure that everything that does not end with '/'
      // is NOT a directory
      else if ( strcmp( m_url.protocol(), "http" ) == 0 )
	mode = 0;
      // Local filesystem without subprotocol
      if ( m_bIsLocalURL )
      {
	struct stat buff;
	if ( stat( m_url.path(), &buff ) == -1 )
	{
	  kioErrorDialog( ERR_COULD_NOT_STAT, cURL );
	  return;
	}
	mode = buff.st_mode;
      }

      popupRequest.x = _global.x();
      popupRequest.y = _global.y();
      popupRequest.mode = mode;
      popupRequest.isLocalFile = (CORBA::Boolean)m_bIsLocalURL;
      
      SIGNAL_CALL1( "popupMenu", popupRequest );
    }
  }
  else if ( _button == LeftButton )
    slotReturnPressed( _item, _global );
  else if ( _button == RightButton )
  {
    Konqueror::View::MenuPopupRequest popupRequest;
    popupRequest.urls.length( 0 );

    list<KonqKfmIconViewItem*> icons;
    selectedItems( icons );
    mode_t mode = 0;
    bool first = true;
    list<KonqKfmIconViewItem*>::iterator icit = icons.begin();
    int i = 0;
    for( ; icit != icons.end(); ++icit )
    {
      popupRequest.urls.length( i + 1 );
      popupRequest.urls[ i++ ] = (*icit)->url();

      UDSEntry::iterator it = (*icit)->udsEntry().begin();
      for( ; it != (*icit)->udsEntry().end(); it++ )
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
}

void KonqKfmIconView::slotDoubleClicked( KIconContainerItem *_item, const QPoint &_global, int _button )
{
  if ( _button == LeftButton )
    slotReturnPressed( _item, _global );
}

void KonqKfmIconView::slotDrop( QDropEvent *_ev, KIconContainerItem* _item, QStrList &_formats )
{
  QStrList lst;

  // Try to decode to the data you understand...
  if ( QUrlDrag::decode( _ev, lst ) )
  {
    if( lst.count() == 0 )
    {
      cerr << "Oooops, no data ...." << endl;
      return;
    }
  }
  else if ( _formats.count() >= 1 )
  {
    if ( _item == 0L )
      pasteData( m_strURL.data(), _ev->data( _formats.getFirst() ) );
    else
    {
      cerr << "Pasting to " << ((KonqKfmIconViewItem*)_item)->url() << endl;
      pasteData( ((KonqKfmIconViewItem*)_item)->url(), _ev->data( _formats.getFirst() ) );
    }
  }
}

void KonqKfmIconView::slotDragStart( const QPoint& _hotspot, QList<KIconContainerItem>& _selected, QPixmap _pixmap )
{
  list<KonqKfmIconViewItem*> icons;
  selectedItems( icons );

  QStrList urls;

  list<KonqKfmIconViewItem*>::iterator icit = icons.begin();
  for( ; icit != icons.end(); ++icit )
    urls.append( (*icit)->url() );

  QUrlDrag *d = new QUrlDrag( urls, viewport() );
  d->setPixmap( _pixmap, _hotspot );
  d->drag();
}

void KonqKfmIconView::openURL( const char *_url )
{
  KURL url( _url );
  if ( url.isMalformed() )
  {
    string tmp = i18n( "Malformed URL" ).ascii();
    tmp += "\n";
    tmp += _url;
    QMessageBox::critical( this, i18n( "Error" ), tmp.c_str(), i18n( "OK" ) );
    return;
  }

  // TODO: Check whether the URL is really a directory

  // Stop running jobs
  if ( m_jobId )
  {
    KIOJob* job = KIOJob::find( m_jobId );
    if ( job )
      job->kill();
    m_jobId = 0;
  }

  m_bComplete = false;

  KIOJob* job = new KIOJob;
  QObject::connect( job, SIGNAL( sigListEntry( int, const UDSEntry& ) ), this, SLOT( slotListEntry( int, const UDSEntry& ) ) );
  QObject::connect( job, SIGNAL( sigFinished( int ) ), this, SLOT( slotCloseURL( int ) ) );
  QObject::connect( job, SIGNAL( sigError( int, int, const char* ) ),
	   this, SLOT( slotError( int, int, const char* ) ) );

  m_sWorkingURL = _url;
  //m_workingURL = url;

  m_buffer.clear();

  m_jobId = job->id();
  job->listDir( url.url() );

  SIGNAL_CALL2( "started", id(), CORBA::Any::from_string( (char *)m_sWorkingURL, 0 ) );
  m_vMainWindow->setPartCaption( id(), m_sWorkingURL );
}

void KonqKfmIconView::slotError( int /*_id*/, int _errid, const char *_errortext )
{
  kioErrorDialog( _errid, _errortext );

//  emit canceled();
}

void KonqKfmIconView::slotCloseURL( int /*_id*/ )
{
  if ( m_bufferTimer.isActive() )
    m_bufferTimer.stop();

  slotBufferTimeout(); // out of the above test, since the dir might be empty (David)
  m_jobId = 0;
  m_bComplete = true;

  SIGNAL_CALL1( "completed", id() );
}

void KonqKfmIconView::slotListEntry( int /*_id*/, const UDSEntry& _entry )
{
  m_buffer.push_back( _entry );
  if ( !m_bufferTimer.isActive() )
    m_bufferTimer.start( 1000, true );
}

void KonqKfmIconView::slotBufferTimeout()
{
  //cerr << "BUFFER TIMEOUT" << endl;

  list<UDSEntry>::iterator it = m_buffer.begin();

  
  //The first entry in the toplevel ?
  if ( !m_sWorkingURL.isEmpty() ) 
  { 
    clear();
    m_strURL = m_sWorkingURL;
    m_sWorkingURL = "";
    m_url = m_strURL.data();
    KURL u( m_url );
    m_bIsLocalURL = u.isLocalFile();
  }

  for( ; it != m_buffer.end(); it++ )
  {
    string name;

    // Find out about the name
    UDSEntry::iterator it2 = it->begin();
    for( ; it2 != it->end(); it2++ )
      if ( it2->m_uds == UDS_NAME )
	name = it2->m_str;

    assert( !name.empty() );
    if ( m_isShowingDotFiles || name[0]!='.' )
    {
      //cerr << "Processing " << name << endl;

      KURL u( m_url );
      u.addPath( name.c_str() );
      KonqKfmIconViewItem* item = new KonqKfmIconViewItem( this, *it, u );
      insert( item, -1, -1, false );

      //cerr << "Ended " << name << endl;
    }
  }

  //cerr << "Doing setup" << endl;

  setup();

  //cerr << "111111111111111111" << endl;

  // refresh();

  viewport()->update();

  m_buffer.clear();
}

void KonqKfmIconView::updateDirectory()
{
  if ( !m_bComplete )
    return;

  // Stop running jobs
  if ( m_jobId )
  {
    KIOJob* job = KIOJob::find( m_jobId );
    if ( job )
      job->kill();
    m_jobId = 0;
  }

  m_bComplete = false;
  m_buffer.clear();

  KIOJob* job = new KIOJob;
  QObject::connect( job, SIGNAL( sigListEntry( int, const UDSEntry& ) ), this, SLOT( slotUpdateListEntry( int, const UDSEntry& ) ) );
  QObject::connect( job, SIGNAL( sigFinished( int ) ), this, SLOT( slotUpdateFinished( int ) ) );
  QObject::connect( job, SIGNAL( sigError( int, int, const char* ) ),
	   this, SLOT( slotUpdateError( int, int, const char* ) ) );

  m_jobId = job->id();
  job->listDir( m_strURL.data() );

  SIGNAL_CALL2( "started", id(), CORBA::Any::from_string( 0L, 0 ) );
}

void KonqKfmIconView::openURLRequest( const char *_url )
{
  Konqueror::URLRequest url;
  url.url = CORBA::string_dup( _url );
  url.reload = (CORBA::Boolean)false;
  SIGNAL_CALL1( "openURL", url );
}

void KonqKfmIconView::slotUpdateError( int /*_id*/, int _errid, const char *_errortext )
{
  kioErrorDialog( _errid, _errortext );

  m_bComplete = true;

//  emit canceled();
}

void KonqKfmIconView::slotUpdateFinished( int /*_id*/ )
{
  if ( m_bufferTimer.isActive() )
    m_bufferTimer.stop();

  slotBufferTimeout(); // out of the above test, since the dir might be empty (David)
  m_jobId = 0;
  m_bComplete = true;

  // Unmark all items
  iterator kit = KIconContainer::begin();
  for( ; kit != KIconContainer::end(); ++kit )
    ((KonqKfmIconViewItem&)**kit).unmark();

  list<UDSEntry>::iterator it = m_buffer.begin();
  for( ; it != m_buffer.end(); it++ )
  {
    string name;

    // Find out about the name
    UDSEntry::iterator it2 = it->begin();
    for( ; it2 != it->end(); it2++ )
      if ( it2->m_uds == UDS_NAME )
	name = it2->m_str;

    assert( !name.empty() );

    // Find this icon
    bool done = false;
    iterator kit = KIconContainer::begin();
    for( ; kit != KIconContainer::end() && !done; ++kit )
    {
      if ( name == (*kit)->text() )
      {
	((KonqKfmIconViewItem&)**kit).mark();
	done = true;
      }
    }

    if ( !done )
    {
      kdebug(0,1202,"slotUpdateFinished : %s",name.c_str());
      if ( m_isShowingDotFiles || name[0]!='.' )
      {
        KURL u( m_url );
        u.addPath( name.c_str() );
        KonqKfmIconViewItem* item = new KonqKfmIconViewItem( this, *it, u );
        item->mark();
        insert( item );
        cerr << "Inserting " << name << endl;
      }
    }
  }

  // Find all unmarked items and delete them
  QList<KIconContainerItem> lst;
  kit = KIconContainer::begin();
  for( ; kit != KIconContainer::end(); ++kit )
  {
    if ( !((KonqKfmIconViewItem&)**kit).isMarked() )
    {
      cerr << "Removing " << (*kit)->text() << endl;
      lst.append( &**kit );
    }
  }

  KIconContainerItem* kci;
  for( kci = lst.first(); kci != 0L; kci = lst.next() )
    remove( kci, false );

  m_buffer.clear();

  SIGNAL_CALL1( "completed", id() );
}

void KonqKfmIconView::slotUpdateListEntry( int /*_id*/, const UDSEntry& _entry )
{
  m_buffer.push_back( _entry );
}


void KonqKfmIconView::slotOnItem( KIconContainerItem *_item )
{
  QString s;
  if ( _item )
    s = ( (KonqKfmIconViewItem *) _item )->getStatusBarInfo();
  SIGNAL_CALL1( "setStatusBarText", CORBA::Any::from_string( (char *)s.data(), 0 ) );
}

void KonqKfmIconView::focusInEvent( QFocusEvent* _event )
{
//  emit gotFocus();

  KIconContainer::focusInEvent( _event );
}

KonqKfmIconViewItem::KonqKfmIconViewItem( KonqKfmIconView *_parent, UDSEntry& _entry, KURL& _url )
  : KIconContainerItem( _parent ), KonqKfmViewItem( _entry, _url )
{
  m_pIconView = _parent;
  init( _entry );
}

void KonqKfmIconViewItem::init( UDSEntry& _entry )
{
  m_displayMode = m_pIconView->displayMode();

  // Find out about the name
  const char * name = 0L;
  UDSEntry::iterator it2 = _entry.begin();
  for( ; it2 != _entry.end(); it2++ )
    if ( it2->m_uds == UDS_NAME )
      name = it2->m_str.c_str();
  assert(name);
  setText(name);

  bool mini = m_pIconView->displayMode() == KIconContainer::Vertical;
  QPixmap * p = KPixmapCache::pixmapForMimeType( m_pMimeType, m_url, m_bIsLocalURL, mini );
  if (!p) warning("Pixmap not found for mimetype %s",m_pMimeType->mimeType());
  else setPixmap( *p );

}

void KonqKfmIconViewItem::refresh()
{
  if ( m_displayMode != m_pIconView->displayMode() )
  {
    m_displayMode = m_pIconView->displayMode();

    mode_t mode = 0;
    UDSEntry::iterator it = m_entry.begin();
    for( ; it != m_entry.end(); it++ )
      if ( it->m_uds == UDS_FILE_TYPE )
	mode = (mode_t)it->m_long;

    bool mini = m_pIconView->displayMode() == KIconContainer::Vertical;
    setPixmap( *( KPixmapCache::pixmapForURL( m_url, mode, m_bIsLocalURL, mini ) ) );
  }

  KIconContainerItem::refresh();
}

/*
void KonqKfmIconViewItem::returnPressed()
{
  mode_t mode = 0;
  UDSEntry::iterator it = m_entry.begin();
  for( ; it != m_entry.end(); it++ )
    if ( it->m_uds == UDS_FILE_TYPE )
      mode = (mode_t)it->m_long;

  // (void)new KRun( m_strURL.c_str(), mode, m_bIsLocalURL );
//  m_pIconView->view()->openURL( m_strURL.c_str(), mode, m_bIsLocalURL );
// HACKHACKHACK
  m_pIconView->openURLRequest( m_strURL.c_str() );
}
*/

void KonqKfmIconViewItem::paint( QPainter* _painter, const QColorGroup _grp )
{
  mode_t mode = 0;

  UDSEntry::iterator it = m_entry.begin();
  for( ; it != m_entry.end(); it++ )
    if ( it->m_uds == UDS_FILE_TYPE )
    {
      mode = (mode_t)it->m_long;
      break;
    }

  if ( S_ISLNK( mode ) )
  {
    QFont f = _painter->font();
    f.setItalic( true );
    _painter->setFont( f );
  }

  KIconContainerItem::paint( _painter, _grp);
}

/*
void KonqKfmIconViewItem::popupMenu( const QPoint &_global )
{
  mode_t mode = 0;
  UDSEntry::iterator it = m_entry.begin();
  for( ; it != m_entry.end(); it++ )
    if ( it->m_uds == UDS_FILE_TYPE )
      mode = (mode_t)it->m_long;

  m_pIconView->setSelected( this, true );

  QStrList lst;
  lst.append( m_strURL.data() );

  m_pIconView->view()->popupMenu( _global, lst, mode, m_bIsLocalURL );
}
*/

#include "konq_iconview.moc"
