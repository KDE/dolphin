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

#include "konq_htmlview.h"

#include "kbrowser.h"
#include "krun.h"
#include "konq_propsview.h"
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

#include <opUIUtils.h>

KonqHTMLView::KonqHTMLView()
{
  ADD_INTERFACE( "IDL:Konqueror/HTMLView:1.0" );
  ADD_INTERFACE( "IDL:Konqueror/PrintingExtension:1.0" );

  setWidget( this );

  QWidget::setFocusPolicy( StrongFocus );

//  initConfig();

  QObject::connect( this, SIGNAL( setTitle( QString ) ),
                    this, SLOT( slotSetTitle( QString ) ) );
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
  openURL( QString( eventURL.url ), (bool)eventURL.reload ); // implemented by kbrowser
  SIGNAL_CALL2( "started", id(), CORBA::Any::from_string( (char *)eventURL.url, 0 ) );
  checkViewMenu();
  return true;
}

bool KonqHTMLView::mappingFillMenuView( Konqueror::View::EventFillMenu viewMenu )
{
//FIXME!!!!!!!!!!!!!!!
#define ID_BASE 14226

  if ( !CORBA::is_nil( viewMenu.menu ) )
  {
    if ( viewMenu.create )
    {
      CORBA::WString_var text = Q2C( i18n("&Save As...") );
      viewMenu.menu->insertItem4( text, this, "saveDocument", 0, ID_BASE+1, -1 );
      text = Q2C( i18n("Save &Frame As...") );
      viewMenu.menu->insertItem4( text, this, "saveFrame", 0, ID_BASE+2, -1 );
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

bool KonqHTMLView::mappingFillMenuEdit( Konqueror::View::EventFillMenu )
{
  // todo : add "Select All"
  return false;
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
  QObject::connect( frame, SIGNAL( onURL( KHTMLView*, QString ) ), 
                    this, SLOT( slotShowURL( KHTMLView*, QString ) ) );

  QObject::connect( frame, SIGNAL( mousePressed( const char*, const QPoint&, int ) ),
                    this, SLOT( slotMousePressed( const char*, const QPoint&, int ) ) );
		    
  QObject::connect( frame, SIGNAL( frameInserted( KBrowser * ) ),
                    this, SLOT( slotFrameInserted( KBrowser * ) ) );		    

  QObject::connect( frame, SIGNAL( urlClicked( QString ) ),
                    this, SLOT( slotURLClicked( QString ) ) );		    

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

void KonqHTMLView::slotURLClicked( QString url )
{
  SIGNAL_CALL2( "started", id(), CORBA::Any::from_string( (char *)url.latin1(), 0 ) );
}

void KonqHTMLView::slotShowURL( KHTMLView *, QString _url )
{
  if ( !_url )
  {
    SIGNAL_CALL1( "setStatusBarText", CORBA::Any::from_wstring( 0L, 0 ) );
    return;
  }

  KURL url( _url );
  QString com;

  KMimeType *typ = KMimeType::findByURL( url );

  if ( typ )
    com = typ->comment( url, false );

  if ( url.isMalformed() )
  {
    CORBA::WString_var wurl = Q2C( url.url() );
    SIGNAL_CALL1( "setStatusBarText", CORBA::Any::from_wstring( wurl.out(), 0 ) );
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
	tmp = i18n("%1 (Link)").arg(com);
      char buff_two[1024];
      text += "->";
      int n = readlink ( decodedPath, buff_two, 1022);
      if (n == -1)
      {
        text2 += "  ";
        text2 += tmp;
	CORBA::WString_var wtext2 = Q2C( text2 );
        SIGNAL_CALL1( "setStatusBarText", CORBA::Any::from_wstring( wtext2, 0 ) );
	return;
      }
      buff_two[n] = 0;
      text += buff_two;
      text += "  ";
      text += tmp;
    }
    else if ( S_ISREG( buff.st_mode ) )
    {
      if (buff.st_size < 1024)
	text = QString("%1 (%2 %3)").arg(text2).arg((long) buff.st_size).arg(i18n("bytes"));
      else
      {
	float d = (float) buff.st_size/1024.0;
	text = QString("%1 (%2 K)").arg(text2).arg(d, 0, 'f', 2); // was %.2f
      }
      text += "  ";
      text += com;
    }
    else if ( S_ISDIR( buff.st_mode ) )
    {
      text += "/  ";
      text += com;
    }
    else
    {
      text += "  ";
      text += com;
    }
    CORBA::WString_var wtext = Q2C( text );
    SIGNAL_CALL1( "setStatusBarText", CORBA::Any::from_wstring( wtext.out(), 0 ) );
  }
  else
  {
    CORBA::WString_var wurl = Q2C( url.url() );
    SIGNAL_CALL1( "setStatusBarText", CORBA::Any::from_wstring( wurl.out(), 0 ) );
  }    
}

void KonqHTMLView::slotSetTitle( QString title )
{
  CORBA::WString_var ctitle = Q2C( title );
  m_vMainWindow->setPartCaption( id(), ctitle );
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

KHTMLEmbededWidget* KonqHTMLView::newEmbededWidget( QWidget* _parent, const char *, const char *, const char *,
						    int /*_marginwidth */, int /*_marginheight*/, 
						    int _frameborder, bool _noresize )
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
    QStringList::Iterator it = p->forms->begin();   
    int i = 0;
    for (; it != p->forms->end(); ++it )
      sp.forms[ i++ ] = *it;
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
    p->forms = new QStringList;
    
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

void KonqHTMLView::print()
{
  KHTMLView::print();
}

void KonqHTMLView::saveDocument()
{
  if ( isFrameSet() )
  {
    //TODO
  }
  else
  {
    KURL srcURL( getKHTMLWidget()->getDocumentURL().url() );

    if ( srcURL.filename().isEmpty() )
      srcURL.setFileName( "index.html" );

    KFileDialog *dlg = new KFileDialog( QDir::currentDirPath(), "*\n*.html\n*.htm",
					this , i18n("Save As..."), true, false );
    dlg->setSelection( QDir::currentDirPath() + "/" + srcURL.filename() );
    if ( dlg->exec() )
      {
	KURL destURL( dlg->selectedFileURL() );
	Konqueror::EventNewTransfer transfer;
	transfer.source = srcURL.url();
	transfer.destination = destURL.url();
	EMIT_EVENT( m_vParent, Konqueror::eventNewTransfer, transfer );
      }

    delete dlg;
  }
}

void KonqHTMLView::saveFrame()
{
  KHTMLView *v = getSelectedView();
  if ( v )
  {
    KURL srcURL( v->getKHTMLWidget()->getDocumentURL().url() );

    if ( srcURL.filename().isEmpty() )
      srcURL.setFileName( "index.htm" );

    KFileDialog *dlg = new KFileDialog( QDir::currentDirPath(), "*\n*.html\n*.htm",
					this , i18n("Save Frameset As..."), true, false );
    dlg->setSelection( QDir::currentDirPath() + "/" + srcURL.filename() );
    if ( dlg->exec() )
    {
      KURL destURL( dlg->selectedFileURL() );
      Konqueror::EventNewTransfer transfer;
      transfer.source = srcURL.url();
      transfer.destination = destURL.url();
      EMIT_EVENT( m_vParent, Konqueror::eventNewTransfer, transfer );
    }

    delete dlg;
  }
}

void KonqHTMLView::openURL( QString _url, bool _reload, int _xoffset, int _yoffset, const char *_post_data )
{
  KBrowser::openURL( _url, _reload, _xoffset, _yoffset, _post_data );
  SIGNAL_CALL2( "setLocationBarURL", id(), CORBA::Any::from_string( (char *)_url.latin1(), 0 ) );
}

void KonqHTMLView::checkViewMenu()
{
  if ( !CORBA::is_nil( m_vViewMenu ) )
  {
    CORBA::WString_var text;
    if ( isFrameSet() )
    {
      text = Q2C( i18n("&Save Frameset As...") );
      m_vViewMenu->changeItemText( text, ID_BASE + 1 );
      m_vViewMenu->setItemEnabled( ID_BASE + 2, true );
    }      
    else
    {
      text = Q2C( i18n("&Save As...") );
      m_vViewMenu->changeItemText( text, ID_BASE + 1 );
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

void KonqEmbededFrame::resizeEvent( QResizeEvent * )
{
  if ( m_pChild == 0L )
    return;

  m_pChild->setGeometry( 0, 0, width(), height() );
}


#include "konq_htmlview.moc"
