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

#include "konq_htmlview.h"

#include "kbrowser.h"
#include "krun.h"
#include "kfmviewprops.h"
#include "kmimetypes.h"

#include <sys/stat.h>
#include <unistd.h>

#include <qstring.h>
#include <string.h>
#include <qstrlist.h>
#include <qdir.h>

#include <kcursor.h>
#include <khtml.h>
#include <khtmlsavedpage.h>
#include <kapp.h>

#include <k2url.h>
#include <kio_error.h>

KonqHTMLView::KonqHTMLView( QWidget *_parent, const char *_name, KBrowser *_parent_browser )
  : KBrowser( _parent, _name, _parent_browser )
{
  ADD_INTERFACE( "IDL:Konqueror/HTMLView:1.0" );

  setWidget( this );

  QWidget::show();

  QWidget::setFocusPolicy( StrongFocus );

  initConfig();

  QObject::connect( this, SIGNAL( onURL( KHTMLView*, const char* ) ), this, SLOT( slotOnURL( KHTMLView*, const char* ) ) );

  QObject::connect( this, SIGNAL( mousePressed( const char*, const QPoint&, int ) ),
                    this, SLOT( slotMousePressed( const char*, const QPoint&, int ) ) );
  QObject::connect( this, SIGNAL( setTitle( const char* ) ),
                    this, SLOT( slotSetTitle( const char * ) ) );
  QObject::connect( this, SIGNAL( started( const char * ) ),
                    this, SLOT( slotStarted( const char * ) ) );
  QObject::connect( this, SIGNAL( completed() ),
                    this, SLOT( slotCompleted() ) );
//  QObject::connect( this, SIGNAL( canceled() ),
//                    this, SLOT( slotCanceled() ) );
}

KonqHTMLView::~KonqHTMLView()
{
}

void KonqHTMLView::initConfig()
{
  KfmViewSettings *settings = KfmViewSettings::defaultHTMLSettings(); // m_pView->settings();
  KHTMLWidget* htmlWidget = getKHTMLWidget();

  htmlWidget->setDefaultBGColor( settings->bgColor() );
  htmlWidget->setDefaultTextColors( settings->textColor(),
				    settings->linkColor(),
				    settings->vLinkColor() );
  htmlWidget->setStandardFont( settings->stdFontName() );
  htmlWidget->setFixedFont( settings->fixedFontName() );

  htmlWidget->setUnderlineLinks( settings->underlineLink() );

  if ( settings->changeCursor() )
    htmlWidget->setURLCursor( KCursor().handCursor() );
  else
    htmlWidget->setURLCursor( KCursor().arrowCursor() );
}

/*
void KonqHTMLView::slotNewWindow( const char *_url )
{
  (void)new KRun( _url, 0, false );
}
*/

KBrowser* KonqHTMLView::createFrame( QWidget *_parent, const char *_name )
{
  KBrowser *m_pBrowser = new KBrowser( _parent, _name, this );

  KConfig *config = kapp->getConfig();
  config->setGroup("Settings");

  KfmViewSettings *settings = KfmViewSettings::defaultHTMLSettings();

  KHTMLWidget* htmlWidget = m_pBrowser->getKHTMLWidget();

  htmlWidget->setDefaultBGColor( settings->bgColor() );
  htmlWidget->setDefaultTextColors( settings->textColor(),
				    settings->linkColor(),
				    settings->vLinkColor() );
  htmlWidget->setStandardFont( settings->stdFontName() );
  htmlWidget->setFixedFont( settings->fixedFontName() );

  htmlWidget->setUnderlineLinks( settings->underlineLink() );

  if ( settings->changeCursor() )
    htmlWidget->setURLCursor( KCursor().handCursor() );
  else
    htmlWidget->setURLCursor( KCursor().arrowCursor() );

  QObject::connect( m_pBrowser, SIGNAL( onURL( KHTMLView*, const char* ) ), this, SLOT( slotOnURL( KHTMLView*, const char* ) ) );
  QObject::connect( m_pBrowser, SIGNAL( mousePressed( const char*, const QPoint&, int ) ),
	   this, SLOT( slotMousePressed( const char*, const QPoint&, int ) ) );
  QObject::connect( m_pBrowser, SIGNAL( setTitle( const char* ) ),
                    this, SLOT( slotSetTitle( const char * ) ) );
	
  return m_pBrowser;
}

bool KonqHTMLView::mappingOpenURL( Konqueror::EventOpenURL eventURL )
{
  KonqBaseView::mappingOpenURL(eventURL);
  openURL( eventURL.url, (bool)eventURL.reload ); // implemented by kbrowser

  return true;
}

bool KonqHTMLView::mappingCreateViewMenu( Konqueror::View::EventCreateViewMenu viewMenu )
{
  OpenPartsUI::Menu_var menu = OpenPartsUI::Menu::_duplicate( viewMenu.menu );
  
  if ( !CORBA::is_nil( menu ) )
  {
    menu->insertItem( "testtesttest", this, "testIgnore", 0 );
  }
  
  return true;
}

void KonqHTMLView::slotMousePressed( const char* _url, const QPoint &_global, int _button )
{
  QString url = _url;

  if ( !_url )
    if ( !KBrowser::m_strURL )
      return;
    else
      url = KBrowser::m_strURL;

  if ( _button == RightButton )
  {
    Konqueror::View::MenuPopupRequest popupRequest;
    K2URL u( url );
    popupRequest.urls.length( 1 );
    popupRequest.urls[0] = url;

    mode_t mode = 0;
    if ( u.isLocalFile() )
      {
	struct stat buff;
	if ( stat( u.path(), &buff ) == -1 )
	  {
	    kioErrorDialog( ERR_COULD_NOT_STAT, url );
	    return;
	  }
	mode = buff.st_mode;
      }

    popupRequest.x = _global.x();
    popupRequest.y = _global.y();
    popupRequest.mode = mode;
    popupRequest.isLocalFile = (CORBA::Boolean)u.isLocalFile();
    SIGNAL_CALL1( "popupMenu", popupRequest );
  }
}

void KonqHTMLView::slotOnURL( const char *_url )
{
  if ( !_url )
  {
    SIGNAL_CALL1( "setStatusBarText", CORBA::Any::from_string( "", 0 ) );
    return;
  }

  K2URL url( _url );
  QString com;

  KMimeType *typ = KMimeType::findByURL( url );

  if ( typ )
    com = typ->comment( url, false );

  if ( url.isMalformed() )
  {
    SIGNAL_CALL1( "setStatusBarText", CORBA::Any::from_string( (char *)_url, 0 ) );
    return;
  }

  QString decodedPath( url.path() );
  QString decodedName( url.filename( true ).c_str() );
	
  struct stat buff;
  stat( decodedPath, &buff );

  struct stat lbuff;
  lstat( decodedPath, &lbuff );
  QString text;
  QString text2;
  text = decodedName.copy(); // copy to change it
  text2 = text;
  //text2.detach();
	
  if ( url.isLocalFile() )
  {
    if (S_ISLNK( lbuff.st_mode ) )
    {
      QString tmp;
      if ( com.isNull() )
	tmp = i18n( "Symbolic Link");
      else
	tmp.sprintf(i18n("%s (Link)"), com.data() );
      char buff_two[1024];
      text += "->";
      int n = readlink ( decodedPath, buff_two, 1022);
      if (n == -1)
      {
        text2 += "  ";
        text2 += tmp;
        SIGNAL_CALL1( "setStatusBarText", CORBA::Any::from_string( (char*)text2.data(), 0 ) );

	return;
      }
      buff_two[n] = 0;
      text += buff_two;
      text += "  ";
      text += tmp.data();
    }
    else if ( S_ISREG( buff.st_mode ) )
    {
      text += " ";
      if (buff.st_size < 1024)
	text.sprintf( "%s (%ld %s)",
		      text2.data(), (long) buff.st_size,
		      i18n("bytes").ascii());
      else
      {
	float d = (float) buff.st_size/1024.0;
	text.sprintf( "%s (%.2f K)", text2.data(), d);
      }
      text += "  ";
      text += com.data();
    }
    else if ( S_ISDIR( buff.st_mode ) )
    {
      text += "/  ";
      text += com.data();
    }
    else
    {
      text += "  ";
      text += com.data();
    }
    SIGNAL_CALL1( "setStatusBarText", CORBA::Any::from_string( (char *)text.data(), 0 ) );
  }
  else
    SIGNAL_CALL1( "setStatusBarText", CORBA::Any::from_string( (char *)url.url().c_str(), 0 ) );
}

void KonqHTMLView::slotSetTitle( const char *title )
{
  m_vMainWindow->setPartCaption( id(), title );
}

void KonqHTMLView::slotStarted( const char *url )
{
  SIGNAL_CALL1( "started", CORBA::Any::from_string( (char *)url, 0 ) );
}

void KonqHTMLView::slotCompleted()
{
  SIGNAL_CALL0( "completed" );
}

void KonqHTMLView::slotCanceled()
{
  SIGNAL_CALL0( "canceled" );
}

bool KonqHTMLView::mousePressedHook( const char *_url, const char *_target, QMouseEvent *_mouse, bool _isselected )
{
//  emit gotFocus();

  return KBrowser::mousePressedHook( _url, _target, _mouse, _isselected );
}

// #include "kfmicons.h"

KHTMLEmbededWidget* KonqHTMLView::newEmbededWidget( QWidget* _parent, const char *_name, const char *_src, const char *_type,
						  int _marginwidth, int _marginheight, int _frameborder, bool _noresize )
{
  KonqEmbededFrame *e = new KonqEmbededFrame( _parent, _frameborder,
                                            _noresize );
  // Not sure I understand this. David.
/*  KfmIconView* icons = new KfmIconView( e, m_pView );
  e->setChild( icons );
  if ( _src == 0L || *_src == 0 )
  {
    QString url = m_pView->workingURL();
    if ( url.isEmpty() )
      url = m_pView->currentURL();
    if ( !url.isEmpty() )
      icons->openURL( url );
  }
  icons->openURL( _src ); */

  return e;
}

void KonqHTMLView::stop()
{
  KBrowser::slotStop();
}

char *KonqHTMLView::url()
{
  return CORBA::string_dup( KBrowser::m_strURL.data() );
}

Konqueror::View::HistoryEntry *KonqHTMLView::saveState()
{
  return KonqBaseView::saveState();
/*
  Konqueror::View::HistoryEntry *entry = KonqBaseView::saveState();
  
  SavedPage *p = saveYourself();
  savePage( &entry->data, p );
  delete p;
  return entry;
*/  
}

void KonqHTMLView::restoreState( const Konqueror::View::HistoryEntry &entry )
{
  KonqBaseView::restoreState( entry );
//  SavedPage *p = restorePage( &entry.data );
  //FIXME: It might be good to use an event here, because of possibly installed
  //       event filters. Pppppperhaps: Store p somewhere else, set a bool flag,
  //       emit an "tweaked" event and obey the flag inside the event handler.
  //(Simon)
//  restore( p );
//  delete p;
}

void KonqHTMLView::savePage( CORBA::Any *data, SavedPage *p )
{
  (*data) <<= CORBA::Any::from_string( (char *)p->frameName.ascii(), 0 );
  (*data) <<= CORBA::Any::from_boolean( (CORBA::Boolean)p->isFrame );
  (*data) <<= (CORBA::Long)p->scrolling;
  (*data) <<= (CORBA::Long)p->frameborder;
  (*data) <<= (CORBA::Long)p->marginwidth;
  (*data) <<= (CORBA::Long)p->marginheight;
  (*data) <<= CORBA::Any::from_boolean( (CORBA::Boolean)p->allowresize );
  (*data) <<= CORBA::Any::from_boolean( (CORBA::Boolean)p->isFrameSet );
  (*data) <<= CORBA::Any::from_string( (char *)p->url.ascii(), 0 );
  (*data) <<= CORBA::Any::from_string( (char *)p->title.ascii(), 0 );
  (*data) <<= (CORBA::Long)p->xOffset;
  (*data) <<= (CORBA::Long)p->yOffset;

  if ( p->forms )
  {
    (*data) <<= (CORBA::ULong)p->forms->count();
    QStrListIterator it( *p->forms );
    for (; it.current(); ++it )
      (*data) <<= CORBA::Any::from_string( it.current(), 0 );
  }
  else
    (*data) <<= (CORBA::ULong)0;

  if ( p->frameLayout )
  {
    (*data) <<= CORBA::Any::from_boolean( (CORBA::Boolean)true );
    (*data) <<= CORBA::Any::from_string( (char *)p->frameLayout->rows.ascii(), 0 );
    (*data) <<= CORBA::Any::from_string( (char *)p->frameLayout->cols.ascii(), 0 );
    (*data) <<= (CORBA::Long)p->frameLayout->frameBorder;
    (*data) <<= CORBA::Any::from_boolean( (CORBA::Boolean)p->frameLayout->allowResize );
  }
  else
    (*data) <<= CORBA::Any::from_boolean( (CORBA::Boolean)false );
        
  if ( p->frames )
  {
    (*data) <<= (CORBA::ULong)p->frames->count();
    QListIterator<SavedPage> it( *p->frames );
    for (; it.current(); ++it )
      savePage( data, it.current() );
  }
  else
    (*data) <<= (CORBA::ULong)0;

}

SavedPage *KonqHTMLView::restorePage( const CORBA::Any *data )
{
  SavedPage *p = new SavedPage;
  
  CORBA::String_var s;
  CORBA::Boolean b;
  CORBA::ULong u, c;
  
  (*data) >>= CORBA::Any::to_string( s, 0 );
  p->frameName = s;
  
  (*data) >>= CORBA::Any::to_boolean( b );
  p->isFrame = b;

  (*data) >>= u;
  p->scrolling = u;
  
  (*data) >>= u;
  p->frameborder = u;
  
  (*data) >>= u;
  p->marginwidth = u;
  
  (*data) >>= u;
  p->marginheight = u;
  
  (*data) >>= CORBA::Any::to_boolean( b );
  p->allowresize = b;
  
  (*data) >>= CORBA::Any::to_boolean( b );
  p->isFrameSet = b;
  
  (*data) >>= CORBA::Any::to_string( s, 0 );
  p->url = s;
  
  (*data) >>= CORBA::Any::to_string( s, 0 );
  p->title = s;
  
  (*data) >>= u;
  p->xOffset = u;
  
  (*data) >>= u;
  p->yOffset = u;
  
  (*data) >>= c;
  if ( c > 0 )
  {
    p->forms = new QStrList;
    for ( u = 0; u < c; u++ )
    {
      (*data) >>= CORBA::Any::to_string( s, 0 );
      p->forms->append( s );
    }
    
  }
  else
    p->forms = 0L;
  
  (*data) >>= CORBA::Any::to_boolean( b );
  if ( (bool)b )
  {
    p->frameLayout = new FrameLayout;
    
    (*data) >>= s;
    p->frameLayout->rows = s;
    
    (*data) >>= s;
    p->frameLayout->cols = s;
    
    (*data) >>= u;
    p->frameLayout->frameBorder = u;
    
    (*data) >>= CORBA::Any::to_boolean( b );
    p->frameLayout->allowResize = b;
  }
  else
    p->frameLayout = 0L;    
    
  (*data) >>= c;
  if ( c > 0 )
  {
    p->frames = new QList<SavedPage>;
    for ( u = 0; u < c; u++ )
    {
      SavedPage *sp = restorePage( data );
      p->frames->append( sp );
    }
  }
  else p->frames = 0L;
  
  return p;
}

#include "konq_partview.h"
#include "konq_mainview.h"

void KonqHTMLView::testIgnore()
{
  if ( !CORBA::is_nil( m_vParent ) )
  {
    if ( m_vParent->supportsInterface( "IDL:Konqueror/MainView:1.0" ) )
    {
      Konqueror::MainView_var mainView = Konqueror::MainView::_narrow( m_vParent->getInterface( "IDL:Konqueror/MainView:1.0" ) );

      Konqueror::PartView_var partView = Konqueror::PartView::_duplicate( new KonqPartView );
    
      mainView->insertView( partView, Konqueror::right );  

      Konqueror::MainView_var mainView2 = Konqueror::MainView::_duplicate( new KonqMainView );

      mainView2->setMainWindow( m_vMainWindow );      

      partView->setPart( mainView2 );
      
      Konqueror::EventOpenURL eventURL;
      
      QString home = "file:";
      home += QDir::homeDirPath();
      eventURL.url = CORBA::string_dup( home.ascii() );
      eventURL.reload = (CORBA::Boolean)true;
      EMIT_EVENT( mainView2, Konqueror::eventOpenURL, eventURL );
    }
  }
}

void KonqHTMLView::openURL( const char *_url, bool _reload, int _xoffset, int _yoffset, const char *_post_data )
{
  KBrowser::openURL( _url, _reload, _xoffset, _yoffset, _post_data );
  SIGNAL_CALL1( "setLocationBarURL", CORBA::Any::from_string( (char *)_url, 0 ) );
}

/**********************************************
 *
 * KonqEmbededFrame
 *
 **********************************************/

KonqEmbededFrame::KonqEmbededFrame( QWidget *_parent, int _frameborder, bool _allowresize )
  : KHTMLEmbededWidget( _parent, _frameborder, _allowresize )
{
  m_pChild = 0L;
}

void KonqEmbededFrame::setChild( QWidget *_widget )
{
  if ( m_pChild )
    delete m_pChild;

  m_pChild = _widget;
  resizeEvent( 0L );
}

QWidget* KonqEmbededFrame::child()
{
  return m_pChild;
}

void KonqEmbededFrame::resizeEvent( QResizeEvent *_ev )
{
  if ( m_pChild == 0L )
    return;

  m_pChild->setGeometry( 0, 0, width(), height() );
}


#include "konq_htmlview.moc"
