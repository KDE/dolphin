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

#include "konq_iconview.h"
#include "konq_propsview.h"

#include <assert.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

#include <kurl.h>
#include <kapp.h>
#include <kio_job.h>
#include <kio_error.h>
#include <klineeditdlg.h>
#include <kpixmapcache.h>
#include <krun.h>
#include <kio_paste.h>
#include <kmimetypes.h>

#include <qmsgbox.h>
#include <qkeycode.h>
#include <qpalette.h>
#include <qdragobject.h>
#include <klocale.h>

#include <opUIUtils.h>

KonqKfmIconView::KonqKfmIconView( QWidget* _parent ) : KIconContainer( _parent )
{
  kdebug(0, 1202, "+KonqKfmIconView");
  ADD_INTERFACE( "IDL:Konqueror/KfmIconView:1.0" );
  
  m_bInit = true;

  setWidget( this );

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
  QObject::connect( this, SIGNAL( drop( QDropEvent*, KIconContainerItem*, QStrList& ) ),
	   this, SLOT( slotDrop( QDropEvent*, KIconContainerItem*, QStrList& ) ) );
  QObject::connect( this, SIGNAL( onItem( KIconContainerItem* ) ), this, SLOT( slotOnItem( KIconContainerItem* ) ) );
  //  connect( m_pView->gui(), SIGNAL( configChanged() ), SLOT( initConfig() ) );

  m_bInit = false;
}

KonqKfmIconView::~KonqKfmIconView()
{
  kdebug(0, 1202, "-KonqKfmIconView");
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

bool KonqKfmIconView::mappingFillMenuView( Konqueror::View::EventFillMenu viewMenu )
{
#define MVIEW_IMAGEPREVIEW_ID 1594 // temporary
#define MVIEW_SHOWDOT_ID 1595 // temporary

  if ( !CORBA::is_nil( viewMenu.menu ) )
  {
    if ( viewMenu.create )
    {
      CORBA::WString_var text;
      //    menu->insertItem4( i18n("&Large Icons"), this, "slotLargeIcons", 0, -1, -1 );
      //    menu->insertItem4( i18n("&Small Icons"), this, "slotSmallIcons", 0, -1, -1 );
      kdebug(0,1202,"adding image preview and showdotfiles");
      text = Q2C( i18n("Image &Preview") );
      viewMenu.menu->insertItem4( text, this, "slotShowSchnauzer" , 0, MVIEW_IMAGEPREVIEW_ID, -1 );
      text = Q2C( i18n("Show &Dot Files") );
      viewMenu.menu->insertItem4( text, this, "slotShowDot" , 0, MVIEW_SHOWDOT_ID, -1 );
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

bool KonqKfmIconView::mappingFillMenuEdit( Konqueror::View::EventFillMenu editMenu )
{
#define MEDIT_SELECT_ID 1394 // hmm...
#define MEDIT_SELECTALL_ID 1395

  if ( !CORBA::is_nil( editMenu.menu ) )
  {
    if ( editMenu.create )
    {
      CORBA::WString_var text;
      //    menu->insertItem4( i18n("&Large Icons"), this, "slotLargeIcons", 0, -1, -1 );
      //    menu->insertItem4( i18n("&Small Icons"), this, "slotSmallIcons", 0, -1, -1 );
      kdebug(0,1202,"adding select and selectall");
      text = Q2C( i18n("Select") );
      editMenu.menu->insertItem4( text, this, "slotSelect" , 0, MEDIT_SELECT_ID, -1 );
      text = Q2C( i18n("Select &All") );
      editMenu.menu->insertItem4( text, this, "slotSelectAll" , 0, MEDIT_SELECTALL_ID, -1 );
    }
    else
    {
      kdebug(0,1202,"removing select and selectall");
      editMenu.menu->removeItem( MEDIT_SELECT_ID );
      editMenu.menu->removeItem( MEDIT_SELECTALL_ID );
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

void KonqKfmIconView::slotSelect()
{
  KLineEditDlg l( i18n("Select files:"), "", this );
  if ( l.exec() )
  {
    QString pattern = l.text();
    if ( pattern.isEmpty() )
      return;

    // QRegExp re( pattern, true, true );
    // view->getActiveView()->select( re, true );

    // TODO
    // Do we need unicode support ? (kregexp?)
  }
}

void KonqKfmIconView::slotSelectAll()
{
  kdebug(0, 1202, "KonqKfmIconView::slotSelectAll");
  selectAll();
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
  for( ; it != KIconContainer::end(); ++it )
    if ( (*it)->isSelected() )
      _list.push_back( (KonqKfmIconViewItem*)&**it );
}

void KonqKfmIconView::slotReturnPressed( KIconContainerItem *_item, const QPoint & )
{
  if ( !_item )
    return;

  KonqKfmIconViewItem *item = (KonqKfmIconViewItem*)_item;
  openURLRequest( item->url().ascii() );
}

void KonqKfmIconView::slotMousePressed( KIconContainerItem *_item, const QPoint &_global, int _button )
{
  kdebug(0,1202,"void KonqKfmIconView::slotMousePressed( KIconContainerItem *_item, const QPoint &_global, int _button )");

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

      if ( first )
      {
        mode = (*icit)->mode();
        first = false;
      }
      else if ( mode != (*icit)->mode() ) // modes are different
        mode = 0; // reset to 0
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
      kdebug(0,1202,"Oooops, no data ....");
      return;
    }
  }
  else if ( _formats.count() >= 1 )
  {
    if ( _item == 0L )
      pasteData( m_strURL.data(), _ev->data( _formats.getFirst() ) );
    else
    {
      kdebug(0,1202,"Pasting to %s", ((KonqKfmIconViewItem*)_item)->url().data());
      pasteData( ((KonqKfmIconViewItem*)_item)->url(), _ev->data( _formats.getFirst() ) );
    }
  }
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

  SIGNAL_CALL2( "started", id(), CORBA::Any::from_string( (char *)m_sWorkingURL.data(), 0 ) );
  CORBA::WString_var caption = Q2C( m_sWorkingURL );
  m_vMainWindow->setPartCaption( id(), caption );
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
  //kdebug(0,1202,"BUFFER TIMEOUT");

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
      //kdebug(0,1202,"Processing %s", name);

      KURL u( m_url );
      u.addPath( name.c_str() );
      KonqKfmIconViewItem* item = new KonqKfmIconViewItem( this, *it, u );
      insert( item, -1, -1, false );

      //kdebug(0,1202,"Ended %s", name);
    }
  }

  //kdebug(0,1202,"Doing setup");

  setup();

  //kdebug(0,1202,"111111111111111111");

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
    QString name;

    // Find out about the name
    UDSEntry::iterator it2 = it->begin();
    for( ; it2 != it->end(); it2++ )
      if ( it2->m_uds == UDS_NAME )
	name = it2->m_str.c_str();

    assert( !name.isEmpty() );

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
      kdebug(0,1202,"slotUpdateFinished : %s",name.ascii());
      if ( m_isShowingDotFiles || name[0]!='.' )
      {
        KURL u( m_url );
        u.addPath( name );
        KonqKfmIconViewItem* item = new KonqKfmIconViewItem( this, *it, u );
        item->mark();
        insert( item );
        kdebug(0,1202,"Inserting %s", name.ascii());
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
      kdebug(0,1202,"Removing %s", (*kit)->text().ascii());
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
  CORBA::WString_var ws = Q2C( s );
  SIGNAL_CALL1( "setStatusBarText", CORBA::Any::from_wstring( ws.out(), 0 ) );
}

void KonqKfmIconView::focusInEvent( QFocusEvent* _event )
{
//  emit gotFocus();

  KIconContainer::focusInEvent( _event );
}

///////// KonqKfmIconViewItem ////////

KonqKfmIconViewItem::KonqKfmIconViewItem( KonqKfmIconView *_parent, UDSEntry& _entry, KURL& _url )
  : KIconContainerItem( _parent ), 
    KFileItem( _entry, _url )
{
  m_pIconView = _parent;
  init();
}

void KonqKfmIconViewItem::init()
{
  // Done out of the constructor since it uses fields filled by KFileItem constructor

  // Set the item text (the one displayed) from the text computed by KFileItem
  setText( getText() );
  // Set the item name from the url hold by KFileItem
  setName( url() );
  // Determine the item pixmap from one determined by KFileItem
  QPixmap *p = getPixmap( m_pIconView->displayMode() == KIconContainer::Vertical );
  if (p) setPixmap( *p );
}

void KonqKfmIconViewItem::refresh()
{
  QPixmap *p = getPixmap( m_pIconView->displayMode() == KIconContainer::Vertical ); // determine the pixmap (KFileItem)
  if (p) setPixmap( *p ); // store it in the item (KIconContainerItem)

  KIconContainerItem::refresh();
}

void KonqKfmIconViewItem::paint( QPainter* _painter, bool _drag )
{
  if ( isLink() ) // implemented in KFileItem
  {
    QFont f = _painter->font();
    f.setItalic( true );
    _painter->setFont( f );
  }

  KIconContainerItem::paint( _painter, _drag );
}

#include "konq_iconview.moc"
