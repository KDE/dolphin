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

#include "kbrowser.h"
#include <kio_job.h>
#include <kio_cache.h>
#include <kio_error.h>

#include <assert.h>
#include <string.h>

#include <kurl.h>
#include <kapp.h>
#include <kdebug.h>
#include <khtml.h>

#include <qmsgbox.h>
#include <klocale.h>

KBrowser::KBrowser( QWidget *parent, const char *name, KBrowser *_parent_browser )
    : KHTMLView( parent, name, 0, _parent_browser )
{
  m_pParentBrowser     = _parent_browser;
  
  m_bStartedRubberBand = false;
  m_pRubberBandPainter = 0L;
  m_bRubberBandVisible = false;
  m_jobId              = 0;
  m_bFileManagerMode   = false;
  m_bParsing           = false;
  m_bComplete          = true;
  m_bReload            = false;
  
  // m_lstPendingURLRequests.setAutoDelete( true );
  m_lstURLRequestJobs.setAutoDelete( true );
  
  // Used for drawing the rubber band
  connect( getKHTMLWidget(), SIGNAL( scrollVert( int ) ),
	   SLOT( slotUpdateSelect(int) ) );

  // We need to know which files the HTML widget wants to get from us
  connect( this, SIGNAL( imageRequest( QString ) ), this, SLOT( slotURLRequest( QString ) ) );
  connect( this, SIGNAL( cancelImageRequest( QString ) ),
	   this, SLOT( slotCancelURLRequest( QString ) ) );
  connect( this, SIGNAL( documentDone( KHTMLView* ) ), this, SLOT( slotDocumentFinished( KHTMLView* ) ) );
  
  m_lstChildren.setAutoDelete( true );

  getKHTMLWidget()->setFocusPolicy( QWidget::StrongFocus );
}

KBrowser::~KBrowser()
{
  slotStop();
}

KHTMLView* KBrowser::newView( QWidget *_parent, const char *_name, int )
{
  KBrowser *v = createFrame( _parent, _name );
  
  m_lstChildren.append( new Child( v, false ) );
  
  emit frameInserted( v );
  
  return v;
}

KBrowser* KBrowser::createFrame( QWidget *_parent, const char *_name )
{
  return new KBrowser( _parent, _name, this );
}

void KBrowser::begin( QString _url, int _x_offset, int _y_offset )
{
  // Delete all frames in this view
  m_lstChildren.clear();
  KHTMLView::begin( _url, _x_offset, _y_offset );
}

void KBrowser::slotStop()
{
  if ( !m_strWorkingURL.isEmpty() )
  {
    m_strWorkingURL = "";
    
    if ( m_bParsing )
    {
      end();
      m_bParsing = false;
    }
  }
  
  m_bComplete = true;  
  m_jobId = 0;

  cancelAllRequests();

  m_lstURLRequestJobs.clear();
  m_lstPendingURLRequests.clear();

  emit canceled();
  if ( m_pParentBrowser )
    m_pParentBrowser->childCompleted( this );
}

void KBrowser::slotReload()
{
  // Reloads everything including the framesets
  if ( !m_strURL.isEmpty() )
    openURL( m_strURL, true );
}

void KBrowser::slotReloadFrames()
{
  // Does not reload framesets but all frames
  if ( isFrame() )
  {
    Child *c;
    for ( c = m_lstChildren.first(); c != 0L; c = m_lstChildren.next() )
    {
      c->m_bReady = false;
      c->m_pBrowser->slotReloadFrames();
    }
  }
  else
    slotReload();
}

void KBrowser::openURL( QString _url )
{
  openURL( _url, false, 0, 0, 0L );
}

void KBrowser::openURL( QString _url, bool _reload, int _xoffset, int _yoffset, const char* /*_post_data*/ )
{
  // Check URL
  if ( KURL::split( _url ).isEmpty() )
  {
    emit error( ERR_MALFORMED_URL, _url );
    return;
  }

  if ( m_jobId )
  {
    KIOJob* job = KIOJob::find( m_jobId );
    if ( job )
      job->kill();
    m_jobId = 0;
  }
  
  if ( m_bParsing )
  {
    end();
    m_bParsing = false;
  }
  
  slotStop();

  m_strWorkingURL = _url;
  m_iNextYOffset = _yoffset;
  m_iNextXOffset = _xoffset;

  m_bReload = _reload;
  
  CachedKIOJob* job = new CachedKIOJob;
  
  job->setGUImode( KIOJob::NONE );
  
  connect( job, SIGNAL( sigFinished( int ) ), this, SLOT( slotFinished( int ) ) );
  connect( job, SIGNAL( sigRedirection( int, const char* ) ), this, SLOT( slotRedirection( int, const char* ) ) );
  connect( job, SIGNAL( sigData( int, const char*, int ) ), this, SLOT( slotData( int, const char*, int ) ) );
  connect( job, SIGNAL( sigError( int, int, const char* ) ), this, SLOT( slotError( int, int, const char* ) ) );
  
  m_jobId = job->id();
  // TODO
  /* if ( _post_data )
    job->post( _url, _post_data );
  else */
  job->get( _url, _reload );

  m_bComplete = false;
    
  emit started( m_strWorkingURL );
}

void KBrowser::slotFinished( int /*_id*/ )
{
  kdebug(0,1202,"SLOT_FINISHED 1");

  kdebug(0,1202,"SLOT_FINISHED 2");
  m_strWorkingURL = "";
  
  if ( m_bParsing )
  {
    kdebug(0,1202,"SLOT_FINISHED 3");
    end();
  }

  m_jobId = 0;
  m_bParsing = false;
}

void KBrowser::slotRedirection( int /*_id*/, const char *_url )
{
  // We get this only !before! we receive the first data
  assert( !m_strWorkingURL.isEmpty() );
  m_strWorkingURL = _url;
}

void KBrowser::slotData( int /*_id*/, const char *_p, int _len )
{
  kdebug(0,1202,"SLOT_DATA %d", _len);

  /** DEBUG **/
  assert( (int)strlen(_p ) <= _len );
  /** End DEBUG **/

  // The first data ?
  if ( !m_strWorkingURL.isEmpty() )
  {
    kdebug(0,1202,"BEGIN...");
    m_lstChildren.clear();
    m_strURL = m_strWorkingURL;
    m_strWorkingURL = "";
    KURL::List lst = KURL::split( m_strURL );
    assert ( !lst.isEmpty() );
    QString baseurl = (*lst.begin()).url();

    m_bParsing = true;
    begin( baseurl, m_iNextXOffset, m_iNextYOffset );
    parse();
  }

  write( _p );
}

void KBrowser::slotError( int /*_id*/, int _err, const char *_text )
{
  if ( _err == ERR_WARNING )
    return; //let's ignore warnings for now
    
  kdebug(0,1202,"+++++++++++++ ERROR %d, %s ", _err, _text);
  
  slotStop();

  emit error( _err, _text );

  // !!!!!! HACK !!!!!!!!!!
  kioErrorDialog( _err, _text );
  
  kdebug(0,1202,"+++++++++++ RETURN from error ++++++++++");
  
  // emit canceled();
}

void KBrowser::slotURLRequest( QString _url )
{
  kdebug(0,1202,"URLRequest %s", _url.ascii());
  
  m_lstPendingURLRequests.append( _url );
 
  servePendingURLRequests();
}

void KBrowser::servePendingURLRequests()
{
  if ( m_lstURLRequestJobs.count() == MAX_REQUEST_JOBS )
    return;
  if ( m_lstPendingURLRequests.count() == 0 )
    return;
  
  KBrowserURLRequestJob* j = new KBrowserURLRequestJob( this );
  m_lstURLRequestJobs.append( j );
  QString url = m_lstPendingURLRequests.first();
  QString tmp = completeURL( url );
  m_lstPendingURLRequests.remove(m_lstPendingURLRequests.begin());

  j->run( tmp, url, m_bReload );
}

void KBrowser::urlRequestFinished( KBrowserURLRequestJob* _request )
{
  m_lstURLRequestJobs.removeRef( _request );
  servePendingURLRequests();
}

void KBrowser::slotCancelURLRequest( QString _url )
{
  QString tmp = completeURL( _url );
  m_lstPendingURLRequests.remove( tmp );
}

void KBrowser::slotURLSelected( QString _url, int _button, QString _target )
{
//   if ( !m_bComplete )
//     slotStop();

  // Security
  KURL u1( _url );
  KURL u2( m_strURL );
  if ( strcmp( u1.protocol(), "cgi" ) == 0 &&
       strcmp( u2.protocol(), "file" ) != 0 && strcmp( u2.protocol(), "cgi" ) != 0 )
  {
    QMessageBox::critical( (QWidget*)0L, i18n( "Security Alert" ),
			   i18n( "This page is untrusted\nbut it contains a link to your local file system."),
			   i18n("OK") );
    return;
  }
    
  if ( _url.isNull() )
    return;
    
  QString url = completeURL( _url );

  if ( !_target.isNull() && !_target.isEmpty() && _button == LeftButton )
  {
    if ( strcmp( _target, "_parent" ) == 0 )
    {
      KHTMLView *v = getParentView();
      if ( !v )
	v = this;
      v->openURL( url );
      emit urlClicked( url );
      return;
    }
    else if ( strcmp( _target, "_top" ) == 0 )
    {
      kdebug(0,1202,"OPENING top %s", url.ascii());
      topView()->openURL( url );
      emit urlClicked( url );
      kdebug(0,1202,"OPENED top");
      return;
    }
    else if ( strcmp( _target, "_blank" ) == 0 )
    {
      emit newWindow( url );
      return;
    }
    else if ( strcmp( _target, "_self" ) == 0 )
    {
      openURL( url );
      emit urlClicked( url );
      return;
    }
    
    KHTMLView *v = ((KBrowser*)topView())->findChildView( _target );
    if ( !v )
      v = findView( _target );
    if ( v )
    {
      v->openURL( url );
      emit urlClicked( url );
      return;
    }
    else
    {
      emit newWindow( url );
      return;
    }
  }
  else if ( _button == MidButton )
  {
      emit newWindow( url );
      return;
  }
  else if ( _button == LeftButton )
  { 
    // Test whether both URLs differ in the Reference only.
    KURL u1( url );
    if ( u1.isMalformed() )
    {
      kioErrorDialog( ERR_MALFORMED_URL, url );
      return;
    }

    // if only the reference differs, then we just go to the new anchor
    if ( urlcmp( url, m_strURL, TRUE, TRUE ) )
    {
      QString anchor = u1.htmlRef();
      kdebug(0,1202,"Going to anchor %s", anchor.ascii());
      gotoAnchor( anchor );
      emit urlClicked( url );
      return;
    }
    
    openURL( url );
    emit urlClicked( url );
  }
}

void KBrowser::slotFormSubmitted( QString _method, QString _url, const char *_data )
{ 
  QString url = completeURL( _url );

  KURL u( _url );
  if ( u.isMalformed() )
  {
    emit error( ERR_MALFORMED_URL, url );
    return;
  }
  
  if ( strcasecmp( _method, "GET" ) == 0 )
  {
    // GET
    QString query = u.query();
    if ( !query.isEmpty() )
    {
      u.setQuery( query + "&" + _data );
    }
    else
    {
      u.setQuery( _data );
    }
    
    openURL( u.url() );
  }
  else
  {
    // POST
    openURL( url, false, 0, 0, _data );
  }
}

bool KBrowser::mousePressedHook( QString _url, QString , QMouseEvent *_mouse, bool _isselected )
{
  m_bStartedRubberBand = false;
  m_strSelectedURL = "";
    
  // Select by drawing a rectangle
  if (_url.isEmpty() && _mouse->button() == LeftButton && isFileManagerMode())
  {
    QPoint p = mapToGlobal( _mouse->pos() );

//     select( 0L, false );
//     m_bStartedRubberBand = true;            // Say it is start of drag
//     rectX1 = _mouse->pos().x();   // get start position
//     rectY1 = _mouse->pos().y() + yOffset();
//     rectX2 = rectX1;
//     rectY2 = rectY1;
//     if ( !m_pRubberBandPainter )
//       m_pRubberBandPainter = new QPainter;

    emit mousePressed( _url, p, _mouse->button() );

    return true;
  }
  // Select a URL with Ctrl Button
  else if ( _url != 0L && _mouse->button() == LeftButton &&
	    ( _mouse->state() & ControlButton ) == ControlButton )
  {   
    QPoint p = mapToGlobal( _mouse->pos() );

    selectByURL( 0L, _url, !_isselected );
    m_strSelectedURL = _url;

    emit mousePressed( _url, p, _mouse->button() );
    return true;
  }
  else if ( _url != 0L && _mouse->button() == LeftButton )
  {
    QPoint p = mapToGlobal( _mouse->pos() );

    // We can not do much here, since we dont know wether
    // this may be the start of some DND action.
    m_strSelectedURL = _url;

    emit mousePressed( _url, p, _mouse->button() );
    return true;
  }
  // Context Menu
  else if ( _url != 0L && _mouse->button() == RightButton )
  {   
    QPoint p = mapToGlobal( _mouse->pos() );
	
//     // A popupmenu for a list of URLs or just for one ?
//     QStrList list;
//     getSelected( list );
    
//     if ( list.find( _url ) != -1 )
//       popupMenu( list, p );
//     else
//     {
//       // The selected URL is not marked, so unmark the marked ones.
//       select( 0L, false );
//       selectByURL( 0L, _url, true );
//       list.clear();
//       list.append( _url );
//       popupMenu( list, p );
//     }
    
    emit mousePressed( _url, p, _mouse->button() );

    return true;
  }
  // Context menu for background ?
  else if ( _url == 0L && _mouse->button() == RightButton )
  {      
    //    if ( m_strURL.isEmpty() )
    //      return true;
    
    QPoint p = mapToGlobal( _mouse->pos() );
	
//     select( 0L, false );
//     QStrList list;
//     list.append( m_strURL );
//     popupMenu( list, p, true );

    emit mousePressed( _url, p, _mouse->button() );

    return true;
  }
  
  return false;
}

bool KBrowser::mouseMoveHook( QMouseEvent *_mouse )
{
  if ( m_bStartedRubberBand )
  {
    int x = _mouse->pos().x();
    int y = _mouse->pos().y() + yOffset();
	
    assert( !m_pRubberBandPainter );
    
    if ( !getKHTMLWidget()->isAutoScrollingY() && m_bRubberBandVisible )
    {
      m_pRubberBandPainter->begin( getKHTMLWidget() );
      m_pRubberBandPainter->setRasterOp (NotROP);
      m_pRubberBandPainter->drawRect(rectX1, rectY1-yOffset(), rectX2-rectX1, rectY2-rectY1);
      m_pRubberBandPainter->end();
      m_bRubberBandVisible = false;
    }
	
    int x1 = rectX1;
    int y1 = rectY1;    
    if ( x1 > x )
    {
      int tmp = x1;
      x1 = x;
      x = tmp;
    }
    if ( y1 > y )
    {
      int tmp = y1;
      y1 = y;
      y = tmp;
    }    
    
    QRect rect( x1, y1, x - x1 + 1, y - y1 + 1 );
    select( 0L, rect );
	
    rectX2 = _mouse->pos().x();
    rectY2 = _mouse->pos().y() + yOffset();
	
    if (_mouse->pos().y() > height() )
      getKHTMLWidget()->autoScrollY( AUTOSCROLL_DELAY, AUTOSCROLL_STEP );
    else if ( _mouse->pos().y() < 0 )
      getKHTMLWidget()->autoScrollY( AUTOSCROLL_DELAY, -AUTOSCROLL_STEP );
    else
      getKHTMLWidget()->stopAutoScrollY();

    if ( !getKHTMLWidget()->isAutoScrollingY() )
    {
      m_pRubberBandPainter->begin( getKHTMLWidget() );
      m_pRubberBandPainter->setRasterOp (NotROP);
      m_pRubberBandPainter->drawRect(rectX1, rectY1-yOffset(), rectX2-rectX1, rectY2-rectY1);
      m_pRubberBandPainter->end();
      m_bRubberBandVisible = true;
    }
    
    return true;
  }
  
  return false;
}

bool KBrowser::mouseReleaseHook( QMouseEvent *_mouse )
{
  // make sure autoScroll is off
  if ( _mouse->button() == LeftButton && m_bStartedRubberBand )
    getKHTMLWidget()->stopAutoScrollY();

  if ( !m_strSelectedURL.isEmpty() && _mouse->button() == LeftButton &&
	 ( _mouse->state() & ControlButton ) == ControlButton )
  {
    // This is already done, so we just consume this event
    m_strSelectedURL = "";
    return true;
  }
  // The user pressed the mouse over an URL, did no DND and released it
  else if ( !m_strSelectedURL.isEmpty() && _mouse->button() == LeftButton )
  {
    QStringList list;
    getSelected( list );

    // The user selected the first icon
    if ( list.count() == 0 )
    {
      selectByURL( 0L, m_strSelectedURL, true );
      m_strSelectedURL = "";
      // Dont consume so that the event can be handled as usual
      return false;
    }

    // The user selected one icon => deselect the other ones if there are any
    select( 0L, false );
    selectByURL( 0L, m_strSelectedURL, true );
    m_strSelectedURL = "";
    // Dont consume so that the event can be handled as usual
    return false;
  }
   
  if ( m_bStartedRubberBand )
  {
    rectX2 = _mouse->pos().x();
    rectY2 = _mouse->pos().y() + yOffset();
    // No selection ?
    if ( ( rectX2 == rectX1 ) && ( rectY2 == rectY1 ) )
    {
      select( 0L, false );
    }
    else
    {
      /* ...find out which objects are in(tersected with) rectangle and
	 select them, but remove rectangle first. */
      if ( m_bRubberBandVisible )
      {
	m_pRubberBandPainter->begin( view );
	m_pRubberBandPainter->setRasterOp (NotROP);
	m_pRubberBandPainter->drawRect (rectX1, rectY1-yOffset(), rectX2-rectX1, rectY2-rectY1);
	m_pRubberBandPainter->end();
	m_bRubberBandVisible = false;
      }
      if ( rectX2 < rectX1 )
      {
	int tmp = rectX1;
	rectX1 = rectX2;
	rectX2 = tmp;
      }
      if ( rectY2 < rectY1 )
      {
	int tmp = rectY1;
	rectY1 = rectY2;
	rectY2 = tmp;
      }
	    
      QRect rect( rectX1, rectY1, rectX2 - rectX1 + 1, rectY2 - rectY1 + 1 );
      select( 0L, rect );
    }
	
    m_bStartedRubberBand = false;
    assert( m_pRubberBandPainter );
    delete m_pRubberBandPainter;
    m_pRubberBandPainter = 0L;
	
    return true;
  }
    
  return false;
}

void KBrowser::slotUpdateSelect( int )
{
    if ( !m_bStartedRubberBand )
	return;

    int x1 = rectX1;
    int y1 = rectY1;    

    QPoint point = QCursor::pos();
    point = mapFromGlobal( point );
    if ( point.y() > height() )
	point.setY( height() );
    else if ( point.y() < 0 )
	point.setY( 0 );

    int x = rectX2 = point.x();
    int y = rectY2 = point.y() + yOffset();

    if ( x1 > x )
    {
	int tmp = x1;
	x1 = x;
	x = tmp;
    }
    if ( y1 > y )
    {
	int tmp = y1;
	y1 = y;
	y = tmp;
    }    
    
    QRect rect( x1, y1, x - x1 + 1, y - y1 + 1 );
    select( 0L, rect );
}

/*
bool KBrowser::URLVisited( const char *_url )
{
  QStrList *list = Kfm::history();
  if ( list->find( _url ) != -1 )
    return true;
  
  return false;
}
*/

void KBrowser::setDefaultTextColors( const QColor& _textc, const QColor& _linkc, const QColor& _vlinkc )
{
  getKHTMLWidget()->setDefaultTextColors( _textc, _linkc, _vlinkc );

  Child *c;
  for ( c = m_lstChildren.first(); c != 0L; c = m_lstChildren.next() )
    c->m_pBrowser->setDefaultTextColors( _textc, _linkc, _vlinkc );
}

void KBrowser::setDefaultBGColor( const QColor& bgcolor )
{
  getKHTMLWidget()->setDefaultBGColor( bgcolor );

  Child *c;
  for ( c = m_lstChildren.first(); c != 0L; c = m_lstChildren.next() )
    c->m_pBrowser->setDefaultBGColor( bgcolor );
}

QString KBrowser::completeURL( QString _url )
{
  KURL orig( m_strURL );
  
  KURL u( orig, _url );
  return u.url();
}

KBrowser* KBrowser::findChildView( const char *_target )
{
  QListIterator<Child> it( m_lstChildren );
  for( ; it.current(); ++it )
  {
    if ( it.current()->m_pBrowser->getFrameName() && 
	 strcmp( it.current()->m_pBrowser->getFrameName(), _target ) == 0 )
      return it.current()->m_pBrowser;
  }
  
  QListIterator<Child> it2( m_lstChildren );
  for( ; it2.current(); ++it2 )
  {
    KBrowser *b = it2.current()->m_pBrowser->findChildView( _target );
    if ( b )
      return b;
  }
  
  return 0L;
}

void KBrowser::childCompleted( KBrowser *_browser )
{
  /** DEBUG **/
  kdebug(0,1202,">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>");
  kdebug(0,1202,"--------------- ChildFinished %p ----------------------",this);
  /** End DEBUG **/

  QListIterator<Child> it( m_lstChildren );
  for( ; it.current(); ++it )
  {
    if ( it.current()->m_pBrowser == _browser )
      it.current()->m_bReady = true;
  }

  if ( m_bComplete )
    slotDocumentFinished( this );

  kdebug(0,1202,"<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<");
}

void KBrowser::slotDocumentFinished( KHTMLView* _view )
{
  if ( _view != this )
    return;
  
  /** DEBUG **/
  if ( !m_pParentBrowser )
  {
    kdebug(0,1202,"--------------------------------------------------");
    kdebug(0,1202,"--------------- DocFinished %p ----------------------",this);
    kdebug(0,1202,"--------------------------------------------------");
  }
  else
    kdebug(0,1202,"########### SUB-DocFinished %p ##############",this);
  /** End DEBUG **/

  m_bComplete = true;
  
  // Are all children complete now ?
  QListIterator<Child> it2( m_lstChildren );
  for( ; it2.current(); ++it2 )
    if ( !it2.current()->m_bReady )
      return;
 
  emit completed();  
  if ( m_pParentBrowser )
    m_pParentBrowser->childCompleted( this );
}

/********************************************************************
 *
 * KBrowserURLRequestJob
 *
 ********************************************************************/

KBrowserURLRequestJob::KBrowserURLRequestJob( KBrowser* _browser )
{
  m_pBrowser = _browser;
  m_jobId = 0;
}

KBrowserURLRequestJob::~KBrowserURLRequestJob()
{
  kdebug(0,1202,"Destructor 1");
  if ( m_jobId )
  {
    KIOJob* job = KIOJob::find( m_jobId );
    if ( job )
      job->kill();
    m_jobId = 0;
  }
  kdebug(0,1202,"Destructor 2");
}

void KBrowserURLRequestJob::run( QString _url, QString _simple_url, bool _reload )
{
  kdebug(0,1202,"$$$$$$$$$ BrowserJob for %s", _url.latin1());
  
  m_strSimpleURL = _simple_url;
  m_strURL = _url;

  CachedKIOJob* job = new CachedKIOJob;
  job->setGUImode( KIOJob::NONE );
  
  connect( job, SIGNAL( sigFinished( int ) ), this, SLOT( slotFinished( int ) ) );
  connect( job, SIGNAL( sigData( int, const char*, int ) ), this, SLOT( slotData( int, const char*, int ) ) );
  connect( job, SIGNAL( sigError( int, int, const char* ) ), this, SLOT( slotError( int, int, const char* ) ) );
  
  m_jobId = job->id();
  job->get( m_strURL, _reload );
}

void KBrowserURLRequestJob::slotFinished( int /*_id*/ )
{
  m_jobId = 0;
  
  kdebug(0,1202,"BROWSER JOB FINISHED %s %s", m_strURL.ascii(), m_strSimpleURL.ascii());
  m_pBrowser->data( m_strSimpleURL, "", 0, true );
  m_pBrowser->urlRequestFinished( this );
  kdebug(0,1202,"Back");
}

void KBrowserURLRequestJob::slotData( int /*_id*/, const char* _data, int _len )
{
  m_pBrowser->data( m_strSimpleURL, _data, _len, false );
}

void KBrowserURLRequestJob::slotError( int /*_id*/, int _err, const char *_text )
{
  m_jobId = 0;
  
  emit error( m_strURL, _err, _text );
  
  m_pBrowser->data( m_strSimpleURL, "", 0, true );
  m_pBrowser->urlRequestFinished( this );
}

#include "kbrowser.moc"
