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
#include <kfiledialog.h>

#include <kurl.h>
#include <kio_error.h>
#include <klocale.h>

KonqHTMLView::KonqHTMLView()
{
  ADD_INTERFACE( "IDL:Konqueror/HTMLView:1.0" );

  setWidget( this );

  QWidget::show();

  QWidget::setFocusPolicy( StrongFocus );

//  initConfig();

  QObject::connect( this, SIGNAL( setTitle( const char* ) ),
                    this, SLOT( slotSetTitle( const char * ) ) );
  QObject::connect( this, SIGNAL( completed() ),
                    this, SLOT( slotCompleted() ) );
//  QObject::connect( this, SIGNAL( started( const char * ) ),
//                    this, SLOT( slotStarted( const char * ) ) );
//  QObject::connect( this, SIGNAL( canceled() ),
//                    this, SLOT( canceled() ) );

  m_vViewMenu = 0L;
		    
  slotFrameInserted( this );
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

bool KonqHTMLView::mappingOpenURL( Konqueror::EventOpenURL eventURL )
{
  KonqBaseView::mappingOpenURL(eventURL);
  openURL( eventURL.url, (bool)eventURL.reload ); // implemented by kbrowser
  SIGNAL_CALL2( "started", id(), CORBA::Any::from_string( (char *)eventURL.url, 0 ) );
  checkViewMenu();
  return true;
}

bool KonqHTMLView::mappingCreateViewMenu( Konqueror::View::EventCreateViewMenu viewMenu )
{
//FIXME!!!!!!!!!!!!!!!
#define ID_BASE 14226

  if ( !CORBA::is_nil( viewMenu.menu ) )
  {
    if ( viewMenu.create )
    {
      viewMenu.menu->insertItem4( i18n("&Save As..."), this, "saveDocument", 0, ID_BASE+1, -1 );
      viewMenu.menu->insertItem4( i18n("Save &Frame As..."), this, "saveFrame", 0, ID_BASE+2, -1 );
      m_vViewMenu = OpenPartsUI::Menu::_duplicate( viewMenu.menu );
      checkViewMenu();
    }
    else
    {
      viewMenu.menu->removeItem ( ID_BASE + 1 );
      viewMenu.menu->removeItem ( ID_BASE + 2 );
      m_vViewMenu = 0L;
    }
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
    KURL u( url );
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

void KonqHTMLView::slotFrameInserted( KBrowser *frame )
{
  QObject::connect( frame, SIGNAL( onURL( KHTMLView*, const char* ) ), 
                    this, SLOT( slotShowURL( KHTMLView*, const char* ) ) );

  QObject::connect( frame, SIGNAL( mousePressed( const char*, const QPoint&, int ) ),
                    this, SLOT( slotMousePressed( const char*, const QPoint&, int ) ) );
		    
  QObject::connect( frame, SIGNAL( frameInserted( KBrowser * ) ),
                    this, SLOT( slotFrameInserted( KBrowser * ) ) );		    

  QObject::connect( frame, SIGNAL( urlClicked( const char * ) ),
                    this, SLOT( slotURLClicked( const char * ) ) );		    

  KfmViewSettings *settings = KfmViewSettings::defaultHTMLSettings();
  KHTMLWidget* htmlWidget = frame->getKHTMLWidget();

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
    checkViewMenu();
}

void KonqHTMLView::slotURLClicked( const char *url )
{
  SIGNAL_CALL2( "started", id(), CORBA::Any::from_string( (char *)url, 0 ) );
}

void KonqHTMLView::slotShowURL( KHTMLView *view, const char *_url )
{
  if ( !_url )
  {
    SIGNAL_CALL1( "setStatusBarText", CORBA::Any::from_string( "", 0 ) );
    return;
  }

  KURL url( _url );
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
  QString decodedName( url.filename( true ) );
	
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
    SIGNAL_CALL1( "setStatusBarText", CORBA::Any::from_string( (char *)text.ascii(), 0 ) );
  }
  else
    SIGNAL_CALL1( "setStatusBarText", CORBA::Any::from_string( (char *)url.url().ascii(), 0 ) );
}

void KonqHTMLView::slotSetTitle( const char *title )
{
  m_vMainWindow->setPartCaption( id(), title );
}

void KonqHTMLView::slotStarted( const char *url )
{
  SIGNAL_CALL2( "started", id(), CORBA::Any::from_string( (char *)url, 0 ) );
}

void KonqHTMLView::slotCompleted()
{
  SIGNAL_CALL1( "completed", id() );
  checkViewMenu();
}

void KonqHTMLView::slotCanceled()
{
  SIGNAL_CALL1( "canceled", id() );
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
  checkViewMenu();
}

char *KonqHTMLView::url()
{
  QString u = m_strWorkingURL;
  if ( u.isEmpty() )
    u = KBrowser::m_strURL;
  return CORBA::string_dup( u.ascii() );
}

Konqueror::View::HistoryEntry *KonqHTMLView::saveState()
{
 kdebug(0,1202,"Konqueror::View::HistoryEntry *KonqHTMLView::saveState()");

  Konqueror::View::HistoryEntry *entry = new Konqueror::View::HistoryEntry;
  
  entry->url = getKHTMLWidget()->getDocumentURL().url().ascii();
  
  SavedPage *p = saveYourself();
  entry->data <<= savePage( p );
  delete p;
  return entry;
}

void KonqHTMLView::restoreState( const Konqueror::View::HistoryEntry &entry )
{
 kdebug(0,1202,"void KonqHTMLView::restoreState( const Konqueror::View::HistoryEntry &entry )");

  Konqueror::HTMLView::SavedState state;
  
  entry.data >>= state;
  
  SavedPage *p = restorePage( state );
    
  //FIXME: It might be good to use an event here, because of possibly installed
  //       event filters. Pppppperhaps: Store p somewhere else, set a bool flag,
  //       emit an "tweaked" event and obey the flag inside the event handler.
  //(Simon)
  
  restore( p );
  delete p;
  SIGNAL_CALL2( "started", id(), CORBA::Any::from_string( (char *)entry.url.in(), 0 ) );
}

Konqueror::HTMLView::SavedState KonqHTMLView::savePage( SavedPage *p )
{
  Konqueror::HTMLView::SavedState sp;
  
  sp.frameName = p->frameName;
  sp.isFrame = p->isFrame;
  sp.scrolling = p->scrolling;
  sp.frameBorder = p->frameborder;
  sp.marginWidth = p->marginwidth;
  sp.marginHeight = p->marginheight;
  sp.allowResize = p->allowresize;
  sp.isFrameSet = p->isFrameSet;
  sp.url = p->url;
  sp.title= p->title;
  sp.xOffset = p->xOffset;
  sp.yOffset = p->yOffset;
  
  if ( p->forms && p->forms->count() > 0 )
  {
    sp.hasFormsList = true;
    
    sp.forms.length( p->forms->count() );
    
    QStrListIterator it( *p->forms );
    int i = 0;
    for (; it.current(); ++it )
      sp.forms[ i++ ] = it.current();
  }
  else
    sp.hasFormsList = false;
  
  if ( p->frameLayout )
  {
    sp.hasFrameLayout = true;
    
    sp.fl_rows = p->frameLayout->rows;
    sp.fl_cols = p->frameLayout->cols;
    sp.fl_frameBorder = p->frameLayout->frameBorder;
    sp.fl_allowResize = p->frameLayout->allowResize;
  }
  else
    sp.hasFrameLayout = false;    

  if ( p->frames && p->frames->count() > 0 )
  {
    sp.hasFrames = true;  
      
    sp.frames.length( p->frames->count() );

    QListIterator<SavedPage> it( *p->frames );
    int i = 0;
    for (; it.current(); ++it)
      sp.frames[ i++ ] = savePage( it.current() );
  }
  else
    sp.hasFrames = false;
        
  return sp;
}

SavedPage *KonqHTMLView::restorePage( Konqueror::HTMLView::SavedState state )
{
  SavedPage *p = new SavedPage;
  
  p->frameName = state.frameName.in();
  p->isFrame = state.isFrame;
  p->scrolling = state.scrolling;
  p->frameborder = state.frameBorder;
  p->marginwidth = state.marginWidth;
  p->marginheight = state.marginHeight;
  p->allowresize = state.allowResize;
  p->isFrameSet = state.isFrameSet;
  p->url = state.url.in();
  p->title = state.title.in();
  p->xOffset = state.xOffset;
  p->yOffset = state.yOffset;
  
  if ( state.hasFormsList )
  {
    p->forms = new QStrList;
    
    for ( CORBA::ULong i = 0; i < state.forms.length(); i++ )
      p->forms->append( state.forms[ i ].in() );
  }
  else
    p->forms = 0L;
  
  if ( state.hasFrameLayout )
  {
    p->frameLayout = new FrameLayout;
    
    p->frameLayout->rows = state.fl_rows.in();
    p->frameLayout->cols = state.fl_cols.in();
    p->frameLayout->frameBorder = state.fl_frameBorder;
    p->frameLayout->allowResize = state.fl_allowResize;
  }
  else
    p->frameLayout = 0L;    
    
  if ( state.hasFrames )
  {
    p->frames = new QList<SavedPage>;
    
    for ( CORBA::ULong i = 0; i < state.frames.length(); i++ )
      p->frames->append( restorePage( state.frames[ i ] ) );
  }
  else
    p->frames = 0L;    
    
  return p;
}

void KonqHTMLView::saveDocument()
{
  if ( isFrameSet() )
  {
    //TODO
  }
  else
  {
    QString destFile = KFileDialog::getOpenFileName( QDir::currentDirPath() );
    if ( !destFile.isEmpty() )
    {
      KURL u( destFile );
      Konqueror::EventNewTransfer transfer;
      transfer.source = getKHTMLWidget()->getDocumentURL().url();
      transfer.destination = u.url();
      EMIT_EVENT( m_vParent, Konqueror::eventNewTransfer, transfer );
    }
  }
}

void KonqHTMLView::saveFrame()
{
  KHTMLView *v = getSelectedView();
  if ( v )
  {
    QString destFile = KFileDialog::getOpenFileName( QDir::currentDirPath() );
    if ( !destFile.isEmpty() )
    {
      KURL u( destFile );
      Konqueror::EventNewTransfer transfer;
      transfer.source = v->getKHTMLWidget()->getDocumentURL().url();
      transfer.destination = u.url();
      EMIT_EVENT( m_vParent, Konqueror::eventNewTransfer, transfer );
    }
  }
}

void KonqHTMLView::openURL( const char *_url, bool _reload, int _xoffset, int _yoffset, const char *_post_data )
{
  KBrowser::openURL( _url, _reload, _xoffset, _yoffset, _post_data );
  SIGNAL_CALL2( "setLocationBarURL", id(), CORBA::Any::from_string( (char *)_url, 0 ) );
}

void KonqHTMLView::checkViewMenu()
{
  if ( !CORBA::is_nil( m_vViewMenu ) )
  {
    if ( isFrameSet() )
    {
      m_vViewMenu->changeItemText( i18n("&Save Frameset As..."), ID_BASE + 1 );
      m_vViewMenu->setItemEnabled( ID_BASE + 2, true );
    }      
    else
    {
      m_vViewMenu->changeItemText( i18n("&Save As..."), ID_BASE + 1 );
      m_vViewMenu->setItemEnabled( ID_BASE + 2, false );
    }      
  }
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
