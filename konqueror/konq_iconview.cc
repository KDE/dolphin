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

#include <k2url.h>
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
  openURL( eventURL.url );
  return true;
}

void KonqKfmIconView::stop()
{
  //TODO
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
  item->returnPressed();
}

void KonqKfmIconView::slotMousePressed( KIconContainerItem *_item, const QPoint &_global, int _button )
{
  //  cerr << "void KonqKfmIconView::slotMousePressed( KIconContainerItem *_item, const QPoint &_global, int _button )" << endl;

  if ( !_item )
  {
    if ( _button == RightButton )
    {
      QStrList urls;
      QString cURL = m_url.url( 1 ).c_str();
      int i = cURL.length();
      
      mode_t mode = 0;
      urls.append( cURL );
      
      // A url ending with '/' is always a directory (said somebody)
      // Wrong, says David : file:/tmp/myfile.gz#gzip:/ isn't !
      if ( !m_url.hasSubURL() && i >= 1 && cURL[ i - 1 ] == '/' )
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

//      m_pView->popupMenu( _global, urls, mode, m_bIsLocalURL );
      //emit signal here... TODO
    }
  }
  else if ( _button == LeftButton )
    ((KonqKfmIconViewItem*)_item)->returnPressed();
  else if ( _button == RightButton )
  {
    QStrList urls;
    
    list<KonqKfmIconViewItem*> icons;
    selectedItems( icons );
    mode_t mode = 0;
    bool first = true;
    list<KonqKfmIconViewItem*>::iterator icit = icons.begin();
    for( ; icit != icons.end(); ++icit )
    {
      urls.append( (*icit)->url() );
      
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
    
//    m_pView->popupMenu( _global, urls, mode, m_bIsLocalURL );
    //emit signal here ..TODO
  }
}

void KonqKfmIconView::slotDoubleClicked( KIconContainerItem *_item, const QPoint &_global, int _button )
{
  if ( _button == LeftButton )
    ((KonqKfmIconViewItem*)_item)->returnPressed();
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
  K2URL url( _url );
  if ( url.isMalformed() )
  {
    string tmp = i18n( "Malformed URL" );
    tmp += "\n";
    tmp += _url;
    QMessageBox::critical( this, i18n( "Error" ), tmp.c_str(), i18n( "OK" ) );
    return;
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
  
  m_bComplete = false;

  KIOJob* job = new KIOJob;
  QObject::connect( job, SIGNAL( sigListEntry( int, UDSEntry& ) ), this, SLOT( slotListEntry( int, UDSEntry& ) ) );
  QObject::connect( job, SIGNAL( sigFinished( int ) ), this, SLOT( slotCloseURL( int ) ) );
  QObject::connect( job, SIGNAL( sigError( int, int, const char* ) ),
	   this, SLOT( slotError( int, int, const char* ) ) );

  m_strWorkingURL = _url;
  m_workingURL = url;

  m_buffer.clear();
  
  m_jobId = job->id();
  job->listDir( _url );

//  emit started( m_strWorkingURL.c_str() );
  SIGNAL_CALL1( "started", CORBA::Any::from_string( (char *)m_strWorkingURL.c_str(), 0 ) );
}

void KonqKfmIconView::slotError( int /*_id*/, int _errid, const char *_errortext )
{
  kioErrorDialog( _errid, _errortext );

//  emit canceled();
}

void KonqKfmIconView::slotCloseURL( int /*_id*/ )
{
  if ( m_bufferTimer.isActive() )
  {    
    m_bufferTimer.stop();
    slotBufferTimeout();
  }
  
  m_jobId = 0;
  m_bComplete = true;

//  emit completed();
  SIGNAL_CALL0( "completed" );
}

void KonqKfmIconView::slotListEntry( int /*_id*/, UDSEntry& _entry )
{
  m_buffer.push_back( _entry );
  if ( !m_bufferTimer.isActive() )
    m_bufferTimer.start( 1000, true );
}

void KonqKfmIconView::slotBufferTimeout()
{
  cerr << "BUFFER TIMEOUT" << endl;
  
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
    if ( m_isShowingDotFiles || name[0]!='.' )
    {

      cerr << "Processing " << name << endl;
    
    // The first entry in the toplevel ?
      if ( !m_strWorkingURL.empty() )
      {
        clear();
      
        m_strURL = m_strWorkingURL.c_str();
        m_strWorkingURL = "";
        m_url = m_strURL.data();
        K2URL u( m_url );
        m_bIsLocalURL = u.isLocalFile();
      }
    
      K2URL u( m_url );
      u.addPath( name.c_str() );
      KonqKfmIconViewItem* item = new KonqKfmIconViewItem( this, *it, u, name.c_str() );
      insert( item, -1, -1, false );

      cerr << "Ended " << name << endl;
    }
  }

  cerr << "Doing setup" << endl;

  setup();
  
  cerr << "111111111111111111" << endl;
  
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
  QObject::connect( job, SIGNAL( sigListEntry( int, UDSEntry& ) ), this, SLOT( slotUpdateListEntry( int, UDSEntry& ) ) );
  QObject::connect( job, SIGNAL( sigFinished( int ) ), this, SLOT( slotUpdateFinished( int ) ) );
  QObject::connect( job, SIGNAL( sigError( int, int, const char* ) ),
	   this, SLOT( slotUpdateError( int, int, const char* ) ) );
  
  m_jobId = job->id();
  job->listDir( m_strURL.data() );

//  emit started( 0 );
  SIGNAL_CALL1( "started", CORBA::Any::from_string( 0L, 0 ) );
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
  {    
    m_bufferTimer.stop();
    slotBufferTimeout();
  }
  
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
      debug("slotUpdateFinished : %s",name.c_str());
      if ( m_isShowingDotFiles || name[0]!='.' )
      {
        K2URL u( m_url );
        u.addPath( name.c_str() );
        KonqKfmIconViewItem* item = new KonqKfmIconViewItem( this, *it, u, name.c_str() );
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

//  emit completed();
  SIGNAL_CALL0( "completed" );
}

void KonqKfmIconView::slotUpdateListEntry( int /*_id*/, UDSEntry& _entry )
{
  m_buffer.push_back( _entry );
}


void KonqKfmIconView::slotOnItem( KIconContainerItem *_item )
{
  if ( !_item )
  {    
//    m_pView->gui()->setStatusBarText( "" );
    SIGNAL_CALL1( "setStatusBarText", CORBA::Any::from_string( "", 0 ) );
    return;
  }

  KMimeType *type = ((KonqKfmIconViewItem* )_item)->mimeType();
  UDSEntry entry = ((KonqKfmIconViewItem* )_item)->udsEntry();
  K2URL url(((KonqKfmIconViewItem* )_item)->url());

  QString comment = type->comment( url, false );  
  QString text;
  QString text2;
  QString linkDest;
  text = _item->text();
  text2 = text;
  text2.detach();

  long size   = 0;
  mode_t mode = 0;

  UDSEntry::iterator it = entry.begin();
  for( ; it != entry.end(); it++ )
  {
    if ( it->m_uds == UDS_SIZE )
      size = it->m_long;
    else if ( it->m_uds == UDS_FILE_TYPE )
      mode = (mode_t)it->m_long;
    else if ( it->m_uds == UDS_LINK_DEST )
      linkDest = it->m_str.c_str();
  }
  
  if ( url.isLocalFile() )
  {
    if (mode & S_ISVTX /*S_ISLNK( mode )*/ )
    {
      QString tmp;
      if ( comment.isEmpty() )
	tmp = i18n ( "Symbolic Link" );
      else
	tmp.sprintf(i18n( "%s (Link)" ), comment.data() );
      text += "->";
      text += linkDest;
      text += "  ";
      text += tmp.data();
    }
    else if ( S_ISREG( mode ) )
    {
      text += " ";
      if (size < 1024)
	text.sprintf( "%s (%ld %s)", 
		      text2.data(), (long) size,
		      i18n( "bytes" ));
      else
      {
	float d = (float) size/1024.0;
	text.sprintf( "%s (%.2f K)", text2.data(), d);
      }
      text += "  ";
      text += comment.data();
    }
    else if ( S_ISDIR( mode ) )
    {
      text += "/  ";
      text += comment.data();
    }
    else
      {
	text += "  ";
	text += comment.data();
      }	  
//    m_pView->gui()->setStatusBarText( text );
    SIGNAL_CALL1( "setStatusBarText", CORBA::Any::from_string( (char *)text.data(), 0 ) );
  }
  else
//    m_pView->gui()->setStatusBarText( url.url().c_str() );
    SIGNAL_CALL1( "setStatusBarText", CORBA::Any::from_string( (char *)url.url().c_str(), 0 ) );
}

void KonqKfmIconView::focusInEvent( QFocusEvent* _event )
{
//  emit gotFocus();

  KIconContainer::focusInEvent( _event );
}

KonqKfmIconViewItem::KonqKfmIconViewItem( KonqKfmIconView *_parent, UDSEntry& _entry, K2URL& _url, const char *_name )
  : KIconContainerItem( _parent )
{
  init( _parent, _entry, _url, _name );
}

void KonqKfmIconViewItem::init( KonqKfmIconView* _IconView, UDSEntry& _entry, K2URL& _url, const char *_name )
{
  m_pParent = _IconView;
  m_entry = _entry;
  m_strURL = _url.url().c_str();
  m_bMarked = false;
  
  mode_t mode = 0;
  UDSEntry::iterator it = _entry.begin();
  for( ; it != _entry.end(); it++ )
    if ( it->m_uds == UDS_FILE_TYPE )
      mode = (mode_t)it->m_long;

  m_bIsLocalURL = _url.isLocalFile();
  m_displayMode = m_pParent->displayMode();
  
  bool mini = false;
  if ( m_pParent->displayMode() == KIconContainer::Vertical )
    mini = true;
  
  cerr << ">>>>>>>>>MIME" << endl;
  m_pMimeType = KMimeType::findByURL( _url, mode, m_bIsLocalURL );
  cerr << "<<<<<<<<<MIME" << endl;
  
  setText( _name );
  
  cerr << ">>>>>>>>>PIX" << endl;
  setPixmap( *( KPixmapCache::pixmapForMimeType( m_pMimeType, _url, m_bIsLocalURL, mini ) ) );
  cerr << "<<<<<<<<<PIX" << endl;
}

void KonqKfmIconViewItem::refresh()
{
  if ( m_displayMode != m_pParent->displayMode() )
  {
    m_displayMode = m_pParent->displayMode();
    
    K2URL url( m_strURL.data() );
  
    mode_t mode = 0;
    UDSEntry::iterator it = m_entry.begin();
    for( ; it != m_entry.end(); it++ )
      if ( it->m_uds == UDS_FILE_TYPE )
	mode = (mode_t)it->m_long;

    bool mini = false;
    if ( m_pParent->displayMode() == KIconContainer::Vertical )
      mini = true;
  
    setPixmap( *( KPixmapCache::pixmapForURL( url, mode, m_bIsLocalURL, mini ) ) );
  }
  
  KIconContainerItem::refresh();
}

void KonqKfmIconViewItem::returnPressed()
{
  mode_t mode = 0;
  UDSEntry::iterator it = m_entry.begin();
  for( ; it != m_entry.end(); it++ )
    if ( it->m_uds == UDS_FILE_TYPE )
      mode = (mode_t)it->m_long;

  // (void)new KRun( m_strURL.c_str(), mode, m_bIsLocalURL );
//  m_pParent->view()->openURL( m_strURL.c_str(), mode, m_bIsLocalURL );
// HACKHACKHACK
  m_pParent->openURLRequest( m_strURL.c_str() );
}

bool KonqKfmIconViewItem::acceptsDrops( QStrList& /* _formats */ )
{
  if ( strcmp( "inode/directory", m_pMimeType->mimeType() ) == 0 )
    return true;

  if ( !m_bIsLocalURL )
    return false;

  if ( strcmp( "application/x-kdelnk", m_pMimeType->mimeType() ) == 0 )
    return true;
  
  // Executable, shell script ... ?
  K2URL u( m_strURL.data() );
  if ( access( u.path(), X_OK ) == 0 )
    return true;
  
  return false;
}

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

  if (mode & S_ISVTX /*S_ISLNK( mode )*/ )
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

  m_pParent->setSelected( this, true );
  
  QStrList lst;
  lst.append( m_strURL.data() );
  
  m_pParent->view()->popupMenu( _global, lst, mode, m_bIsLocalURL );
}
*/

#include "konq_iconview.moc"
