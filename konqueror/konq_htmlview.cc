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
#include "konq_propsview.h"
#include "konq_mainview.h"
#include "konq_childview.h"
#include "konq_txtview.h"
#include "konq_defaults.h"

#include <sys/stat.h>
#include <unistd.h>

#include <qstring.h>
#include <string.h>
#include <qstringlist.h>
#include <qdir.h>

#include <kcursor.h>
#include <khtml.h>
#include <khtmlsavedpage.h>
#include <kbrowser.h>
#include <klocale.h>
#include <kfiledialog.h>
#include <kurl.h>
#include <kio_error.h>
#include <kmimetypes.h>
#include <kapp.h>

#include <opUIUtils.h>

KonqHTMLView::KonqHTMLView( KonqMainView *mainView )
{
  ADD_INTERFACE( "IDL:Konqueror/HTMLView:1.0" );
  ADD_INTERFACE( "IDL:Browser/PrintingExtension:1.0" );

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
  
  m_pMainView = mainView;
		    
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

bool KonqHTMLView::event( const char *event, const CORBA::Any &value )
{
  EVENT_MAPPER( event, value );
  
  MAPPING( Browser::View::eventFillMenuEdit, Browser::View::EventFillMenu, mappingFillMenuEdit );
  MAPPING( Browser::View::eventFillMenuView, Browser::View::EventFillMenu, mappingFillMenuView );
  MAPPING( Browser::View::eventCreateViewToolBar, Browser::View::EventCreateViewToolBar, mappingCreateViewToolBar );
  MAPPING( Browser::eventOpenURL, Browser::EventOpenURL, mappingOpenURL );
  MAPPING( Konqueror::HTMLView::eventRequestDocument, Konqueror::HTMLView::EventRequestDocument, mappingRequestDocument );
    
  END_EVENT_MAPPER;
  
  return false;
}


bool KonqHTMLView::mappingOpenURL( Browser::EventOpenURL eventURL )
{
  KonqBaseView::mappingOpenURL(eventURL);
  KonqHTMLView::openURL( QString( eventURL.url ), (bool)eventURL.reload, (int)eventURL.xOffset, (int)eventURL.yOffset );
  SIGNAL_CALL2( "started", id(), CORBA::Any::from_string( (char *)eventURL.url, 0 ) );
  checkViewMenu();
  return true;
}

bool KonqHTMLView::mappingFillMenuView( Browser::View::EventFillMenu viewMenu )
{
//FIXME!!!!!!!!!!!!!!!
#define ID_BASE 14226

  if ( !CORBA::is_nil( viewMenu.menu ) )
  {
    if ( viewMenu.create )
    {
      CORBA::WString_var text;
      viewMenu.menu->insertItem4( ( text = Q2C( i18n("&Save As...") ) ), 
                                  this, "saveDocument", 0, ID_BASE+1, -1 );
      viewMenu.menu->insertItem4( ( text = Q2C( i18n("Save &Frame As...") ) ), 
                                  this, "saveFrame", 0, ID_BASE+2, -1 );
      viewMenu.menu->insertItem4( ( text = Q2C( i18n("Save &Background Image As...") ) ), 
                                  this, "saveBackground", 0, ID_BASE+3, -1 );
      viewMenu.menu->insertItem4( ( text = Q2C( i18n( "View Document Source" ) ) ), 
                                  this, "viewDocumentSource", 0, ID_BASE+4, -1 );
      viewMenu.menu->insertItem4( ( text = Q2C( i18n( "View Frame Source" ) ) ), 
                                  this, "viewFrameSource", 0, ID_BASE+5, -1 );
      
      m_vViewMenu = OpenPartsUI::Menu::_duplicate( viewMenu.menu );
      
      checkViewMenu();
    }
    else
    {
      viewMenu.menu->removeItem ( ID_BASE + 1 );
      viewMenu.menu->removeItem ( ID_BASE + 2 );
      viewMenu.menu->removeItem ( ID_BASE + 3 );
      viewMenu.menu->removeItem ( ID_BASE + 4 );
      viewMenu.menu->removeItem ( ID_BASE + 5 );
      m_vViewMenu = 0L;
    }
  }
  
  return true;
}

bool KonqHTMLView::mappingFillMenuEdit( Browser::View::EventFillMenu )
{
  // todo : add "Select All"
  return false;
}

bool KonqHTMLView::mappingRequestDocument( Konqueror::HTMLView::HTMLDocumentRequest docRequest )
{
  kdebug( 0, 1202, "bool KonqHTMLView::mappingRequestDocument()");
  KBrowser::openURL( C2Q( docRequest.url ), docRequest.reload, (int)docRequest.xOffset, (int)docRequest.yOffset, docRequest.postData );
  SIGNAL_CALL2( "setLocationBarURL", id(), CORBA::Any::from_string( (char *)C2Q( docRequest.url ).latin1(), 0 ) );
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

  if ( _button == RightButton && m_pMainView )
  {
    KURL u( url );

    QStringList lstPopupURLs;
    
    lstPopupURLs.append( url );
    
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

    m_pMainView->popupMenu( _global, lstPopupURLs, mode );
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
    
  frame->enableImages( settings->autoLoadImages() );
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

CORBA::Long KonqHTMLView::xOffset()
{
  return (CORBA::Long)KBrowser::xOffset();
}

CORBA::Long KonqHTMLView::yOffset()
{
  return (CORBA::Long)KBrowser::yOffset();
}

void KonqHTMLView::print()
{
  KHTMLView::print();
}

Konqueror::HTMLView::HTMLPageLinkInfoList *KonqHTMLView::pageLinkInfoList()
{
  Konqueror::HTMLView::HTMLPageLinkInfoList *lst = new Konqueror::HTMLView::HTMLPageLinkInfoList;
  
  //TODO
  lst->length( 0 );
  
  return lst;
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

    if ( srcURL.filename(false).isEmpty() )
      srcURL.setFileName( "index.html" );

    KFileDialog *dlg = new KFileDialog( QString::null, "*\n*.html\n*.htm",
					this , "filedialog", true, false );
    dlg->setCaption(i18n("Save as"));
    dlg->setSelection( dlg->dirPath() + srcURL.filename() );
    if ( dlg->exec() )
      {
	KURL destURL( dlg->selectedFileURL() );
	Browser::EventNewTransfer transfer;
	transfer.source = srcURL.url();
	transfer.destination = destURL.url();
	EMIT_EVENT( m_vParent, Browser::eventNewTransfer, transfer );
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

    if ( srcURL.filename(false).isEmpty() )
      srcURL.setFileName( "index.html" );

    KFileDialog *dlg = new KFileDialog( QString::null, "*\n*.html\n*.htm",
					this , "filedialog", true, false );
    dlg->setCaption(i18n("Save frameset as"));
    dlg->setSelection( dlg->dirPath() + srcURL.filename() );
    if ( dlg->exec() )
    {
      KURL destURL( dlg->selectedFileURL() );
      Browser::EventNewTransfer transfer;
      transfer.source = srcURL.url();
      transfer.destination = destURL.url();
      EMIT_EVENT( m_vParent, Browser::eventNewTransfer, transfer );
    }

    delete dlg;
  }
}

void KonqHTMLView::saveBackground()
{
  QString bURL;
  
  if ( isFrameSet() )
    bURL = getSelectedView()->getKHTMLWidget()->getBackground();
  else
    bURL = getKHTMLWidget()->getBackground();

  if ( bURL.isNull() || bURL.isEmpty() )
    return;
  
  KURL srcURL( bURL );
  
  KFileDialog *dlg = new KFileDialog( QString::null, "*",
				      this , "filedialog", true, false );
  dlg->setCaption(i18n("Save background image as"));
  dlg->setSelection( dlg->dirPath() + srcURL.filename() );
  if ( dlg->exec() )
    {
      KURL destURL( dlg->selectedFileURL() );
      Browser::EventNewTransfer transfer;
      transfer.source = srcURL.url();
      transfer.destination = destURL.url();
      EMIT_EVENT( m_vParent, Browser::eventNewTransfer, transfer );
    }
  
  delete dlg;
}

void KonqHTMLView::viewDocumentSource()
{
  openTxtView( getKHTMLWidget()->getDocumentURL().url() );
}

void KonqHTMLView::viewFrameSource()
{
  KHTMLView *v = getSelectedView();
  if ( v )
    openTxtView( v->getKHTMLWidget()->getDocumentURL().url() );
}

void KonqHTMLView::openTxtView( const QString &url )
{
  if ( m_pMainView )
  {
    KonqChildView *childView = m_pMainView->childView( id() );
    if ( childView )
    {
      Browser::View_var vView;
      QStringList serviceTypes;
      KonqChildView::createView( "text/plain", vView, serviceTypes, m_pMainView );
      childView->makeHistory( false );
      childView->changeView( vView, serviceTypes );
    }
  }
  else
  {
    KConfig *config = kapp->getConfig();
    config->setGroup( "Misc Defaults" );
    QString editor = config->readEntry( "Editor", DEFAULT_EDITOR );
    
    QString u = m_strWorkingURL;
    if ( u.isEmpty() )
      u = KBrowser::m_strURL;
    
    QCString cmd;
    cmd.sprintf( "%s %s &", editor.ascii(), u.ascii() );
    system( cmd.data() );
  }
}

void KonqHTMLView::beginDoc( const CORBA::WChar *url, CORBA::Long dx, CORBA::Long dy )
{
  KBrowser::begin( C2Q( url ), (int)dx, (int)dy );
}

void KonqHTMLView::writeDoc( const char *data )
{
  KBrowser::write( data );
}

void KonqHTMLView::endDoc()
{
  KBrowser::end();
}

void KonqHTMLView::parseDoc()
{
  KBrowser::parse();
}

void KonqHTMLView::openURL( QString _url, bool _reload, int _xoffset, int _yoffset, const char *_post_data )
{
  kdebug( 0, 1202, "void KonqHTMLView::openURL( QString _url, bool _reload, int _xoffset, int _yoffset, const char *_post_data )");
  Konqueror::HTMLView::HTMLDocumentRequest docRequest;
  docRequest.url = Q2C( _url );
  docRequest.reload = _reload;
  docRequest.xOffset = (CORBA::Long)_xoffset;
  docRequest.yOffset = (CORBA::Long)_yoffset;
  docRequest.postData = CORBA::string_dup( _post_data );
  EMIT_EVENT( this, Konqueror::HTMLView::eventRequestDocument, docRequest );
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
      m_vViewMenu->setItemEnabled( ID_BASE + 5, true );
    }      
    else
    {
      text = Q2C( i18n("&Save As...") );
      m_vViewMenu->changeItemText( text, ID_BASE + 1 );
      m_vViewMenu->setItemEnabled( ID_BASE + 2, false );
      m_vViewMenu->setItemEnabled( ID_BASE + 5, false );
    }

    QString bURL = QString::null;

    if ( isFrameSet() )
      {
	KHTMLView *v = getSelectedView();	
	if ( v )
	  bURL = v->getKHTMLWidget()->getBackground();
      }
    else
      bURL = getKHTMLWidget()->getBackground();
    
    m_vViewMenu->setItemEnabled( ID_BASE + 3, !bURL.isNull() && !bURL.isEmpty() );
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
