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
#include "konq_factory.h"
#include "konq_txtview.h"
#include "konq_defaults.h"
#include "konq_progressproxy.h"

#include <sys/stat.h>
#include <unistd.h>

#include <qstring.h>
#include <string.h>
#include <qstringlist.h>
#include <qdir.h>
#include <qclipboard.h>

#include <kcursor.h>
#include <khtml.h>
#include <khtmlsavedpage.h>
#include <klocale.h>
#include <kfiledialog.h>
#include <kurl.h>
#include <kio_error.h>
#include <kio_job.h>
#include <kmimetypes.h>
#include <kapp.h>
#include <kpixmapcache.h>

#include <opUIUtils.h>

#define TOOLBAR_LOADIMAGES_ID Browser::View::TOOLBAR_ITEM_ID_BEGIN

KonqHTMLView::KonqHTMLView( KonqMainView *mainView, KBrowser *parentBrowser, const char *name )
: KBrowser( 0L, 0L, parentBrowser )
{
  ADD_INTERFACE( "IDL:Konqueror/HTMLView:1.0" );
  ADD_INTERFACE( "IDL:Browser/PrintingExtension:1.0" );
  ADD_INTERFACE( "IDL:Browser/EditExtension:1.0" );

  SIGNAL_IMPL( "loadingProgress" );
  SIGNAL_IMPL( "speedProgress" );
  
  SIGNAL_IMPL( "selectionChanged" ); //part of the EditExtension (Simon)
  
  setWidget( this );

  QWidget::setFocusPolicy( StrongFocus );

  QObject::connect( this, SIGNAL( setTitle( QString ) ),
                    this, SLOT( slotSetTitle( QString ) ) );
  QObject::connect( this, SIGNAL( completed() ),
                    this, SLOT( slotCompleted() ) );
  QObject::connect( this, SIGNAL( started( const QString & ) ),
                    this, SLOT( slotStarted( const QString & ) ) );
  QObject::connect( this, SIGNAL( canceled() ),
                    this, SLOT( slotCanceled() ) );

  m_vViewMenu = 0L;
  
  m_pMainView = mainView;

  m_bAutoLoadImages = KfmViewSettings::defaultHTMLSettings()->autoLoadImages();

  enableSmartAnchorHandling( false );

  slotFrameInserted( this );
}

KonqHTMLView::~KonqHTMLView()
{
}

bool KonqHTMLView::event( const char *event, const CORBA::Any &value )
{
  EVENT_MAPPER( event, value );
  
  MAPPING( Browser::View::eventFillMenuEdit, Browser::View::EventFillMenu_ptr, mappingFillMenuEdit );
  MAPPING( Browser::View::eventFillMenuView, Browser::View::EventFillMenu_ptr, mappingFillMenuView );
  MAPPING( Browser::View::eventFillToolBar, Browser::View::EventFillToolBar, mappingFillToolBar );
  MAPPING( Browser::eventOpenURL, Browser::EventOpenURL, mappingOpenURL );
    
  END_EVENT_MAPPER;
  
  return false;
}


bool KonqHTMLView::mappingOpenURL( Browser::EventOpenURL eventURL )
{
  KonqBaseView::mappingOpenURL(eventURL);
  
  m_bAutoLoadImages = KfmViewSettings::defaultHTMLSettings()->autoLoadImages();
  enableImages( m_bAutoLoadImages );  
  
  QString url = eventURL.url.in();
  
  if ( !(bool)eventURL.reload && urlcmp( url, KBrowser::m_strURL, TRUE, TRUE ) )
  {
    KURL u( url );
    KBrowser::m_strWorkingURL = url;
    KBrowser::m_strURL = url;
    slotStarted( url );
    if ( !u.htmlRef().isEmpty() )
      gotoAnchor( u.htmlRef() );
    else //HACK (Simon)
      slotScrollVert( 0 );
    slotCompleted();
    return true;
  }
  else KBrowser::openURL( url, (bool)eventURL.reload, (int)eventURL.xOffset, (int)eventURL.yOffset );
  
  if ( m_jobId )
  {
    KIOJob *job = KIOJob::find( m_jobId );
    if ( job )
    {
      (void)new KonqProgressProxy( this, job );
      
      QObject::connect( job, SIGNAL( sigRedirection( int, const char * ) ),
                        this, SLOT( slotDocumentRedirection( int, const char * ) ) );
    }      
  }

  checkViewMenu();
  return true;
}

bool KonqHTMLView::mappingFillMenuView( Browser::View::EventFillMenu_ptr viewMenu )
{
  m_vViewMenu = OpenPartsUI::Menu::_duplicate( viewMenu );
  if ( !CORBA::is_nil( viewMenu ) )
  {
    CORBA::WString_var text;
    m_idSaveDocument = viewMenu->insertItem4( ( text = Q2C( i18n("&Save As...") ) ), 
                                              this, "saveDocument", 0, -1, -1 );
    m_idSaveFrame = viewMenu->insertItem4( ( text = Q2C( i18n("Save &Frame As...") ) ), 
                                           this, "saveFrame", 0, -1, -1 );
    m_idSaveBackground = viewMenu->insertItem4( ( text = Q2C( i18n("Save &Background Image As...") ) ), 
                                                this, "saveBackground", 0, -1, -1 );
    m_idViewDocument = viewMenu->insertItem4( ( text = Q2C( i18n( "View Document Source" ) ) ), 
                                              this, "viewDocumentSource", 0, -1, -1 );
    m_idViewFrame = viewMenu->insertItem4( ( text = Q2C( i18n( "View Frame Source" ) ) ), 
                                           this, "viewFrameSource", 0, -1, -1 );
      
    checkViewMenu();
  }
    
  return true;
}

bool KonqHTMLView::mappingFillMenuEdit( Browser::View::EventFillMenu_ptr )
{
  // todo : add "Select All"
  return false;
}

bool KonqHTMLView::mappingFillToolBar( Browser::View::EventFillToolBar viewToolBar )
{
  if ( KfmViewSettings::defaultHTMLSettings()->autoLoadImages() )
    return false;

  if ( CORBA::is_nil( viewToolBar.toolBar ) )
    return true;
    
  if ( viewToolBar.create )
  {
    CORBA::WString_var toolTip = Q2C( i18n( "Load Images" ) );
    OpenPartsUI::Pixmap_var pix = OPUIUtils::convertPixmap( *KPixmapCache::toolbarPixmap( "image.xpm" ) );
    viewToolBar.toolBar->insertButton2( pix, TOOLBAR_LOADIMAGES_ID, 
                                        SIGNAL(clicked()), this, "slotLoadImages", 
					true, toolTip, viewToolBar.startIndex++ );
  }
  else
    viewToolBar.toolBar->removeItem( TOOLBAR_LOADIMAGES_ID );

  return true;
}

void KonqHTMLView::slotMousePressed( const QString &_url, const QPoint &_global, int _button )
{
  QString url = _url;

  if ( _url.isEmpty() )
    if ( !KBrowser::m_strURL )
      return;
    else
      url = KBrowser::m_strURL;

  if ( _button == RightButton && m_pMainView )
  {
    KURL u( url );

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
    } else
    {
      QString cURL = u.url( 1 );
      int i = cURL.length();
      // A url ending with '/' is always a directory
      if ( i >= 1 && cURL[ i - 1 ] == '/' )
        mode = S_IFDIR; 
    }
    
    KFileItem item( "viewURL" /*whatever*/, mode, u );
    KFileItemList items;
    items.append( &item );
    m_pMainView->popupMenu( _global, items );
  }
}

void KonqHTMLView::slotFrameInserted( KBrowser *frame )
{
  QObject::connect( frame, SIGNAL( onURL( KHTMLView*, QString ) ), 
                    this, SLOT( slotShowURL( KHTMLView*, QString ) ) );

  QObject::connect( frame, SIGNAL( mousePressed( const QString &, const QPoint&, int ) ),
                    this, SLOT( slotMousePressed( const QString &, const QPoint&, int ) ) );
		    
  QObject::connect( frame, SIGNAL( frameInserted( KBrowser * ) ),
                    this, SLOT( slotFrameInserted( KBrowser * ) ) );		    

  QObject::connect( frame, SIGNAL( newWindow( const QString & ) ),
                    this, SLOT( slotNewWindow( const QString & ) ) );

  QObject::connect( frame, SIGNAL( textSelected( KHTMLView *, bool ) ),
                    this, SLOT( slotSelectionChanged() ) );

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
    
  frame->enableImages( m_bAutoLoadImages );
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
    QString decodedURL = _url;
    KURL::decode( decodedURL );
    CORBA::WString_var wurl = Q2C( decodedURL );
    SIGNAL_CALL1( "setStatusBarText", CORBA::Any::from_wstring( wurl.out(), 0 ) );
    return;
  }

  if ( url.isLocalFile() )
  {
    QString decodedPath( url.path() );
    QString decodedName( url.filename( true ) );
	
    struct stat buff;
    stat( decodedPath, &buff );

    struct stat lbuff;
    lstat( decodedPath, &lbuff );
    
    QString text = url.url();
    QString text2 = text;
    
    if (S_ISLNK( lbuff.st_mode ) )
    {
      QString tmp;
      if ( com.isNull() )
	tmp = i18n( "Symbolic Link");
      else
	tmp = i18n("%1 (Link)").arg(com);
      char buff_two[1024];
      text += " -> ";
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
      text += "  ";
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
    CORBA::WString_var wurl = Q2C( url.decodedURL() );
    SIGNAL_CALL1( "setStatusBarText", CORBA::Any::from_wstring( wurl.out(), 0 ) );
  }    
}

void KonqHTMLView::slotSetTitle( QString title )
{
  QString decodedTitle = title;
  KURL::decode( decodedTitle );
  
  if ( m_pMainView ) //builtin view?
    decodedTitle.prepend( "Konqueror: " );
  
  CORBA::WString_var ctitle = Q2C( decodedTitle );
  m_vMainWindow->setPartCaption( id(), ctitle );
}

void KonqHTMLView::slotStarted( const QString &url )
{
  SIGNAL_CALL2( "started", id(), CORBA::Any::from_string( (char *)url.ascii(), 0 ) );
}

void KonqHTMLView::slotCompleted()
{
  SIGNAL_CALL1( "completed", id() );
  checkViewMenu();
}

void KonqHTMLView::slotCanceled()
{
  // we need this because the destructor of KBrowser calls slotStop, which
  // against emits the canceled signal. However at this time the KOM stuff
  // is already cleaned up and we can't deal with signal stuff anymore
  // (Simon)
  if ( !m_bIsClean )
    SIGNAL_CALL1( "canceled", id() );
}

void KonqHTMLView::slotDocumentRedirection( int, const char *url )
{
  //no need for special stuff, KBrowser does everything for us. Let's just make
  //sure the url in the mainview gets updated
  QString decodedURL = url;
  KURL::decode( decodedURL );
  SIGNAL_CALL2( "setLocationBarURL", id(), (char *)decodedURL.ascii() );
}

void KonqHTMLView::slotNewWindow( const QString &url )
{
  SIGNAL_CALL1( "createNewWindow", CORBA::Any::from_string( (char *)url.ascii(), 0 ) );
}

void KonqHTMLView::slotSelectionChanged()
{
  SIGNAL_CALL0( "selectionChanged" );
}
/*
KBrowser *KonqHTMLView::createFrame( QWidget *_parent, const char *_name )
{
  KonqHTMLView *v = new KonqHTMLView( m_pMainView, _name );
  v->reparent( _parent, 0, QPoint( 0, 0 ) );
  
  v->setParent( this );
  
  v->connect
  
  return v;
}
*/
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

void KonqHTMLView::slotLoadImages()
{
  m_bAutoLoadImages = true;
  enableImages( m_bAutoLoadImages );  
  
  Browser::EventOpenURL ev;
  ev.url = url();
  ev.reload = true;
  ev.xOffset = xOffset();
  ev.yOffset = yOffset();
  EMIT_EVENT( this, Browser::eventOpenURL, ev );
}

void KonqHTMLView::openTxtView( const QString &url )
{
  if ( m_pMainView )
  {
    KonqChildView *childView = m_pMainView->childView( id() );
    if ( childView )
    {
      QStringList serviceTypes;
      Browser::View_var vView = KonqFactory::createView( "text/plain", serviceTypes, m_pMainView );
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

void KonqHTMLView::beginDoc( const char *url, CORBA::Long dx, CORBA::Long dy )
{
  KBrowser::begin( url, (int)dx, (int)dy );
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
  
  Browser::URLRequest req;
  req.url = CORBA::string_dup( _url.ascii() );
  req.reload = (CORBA::Boolean)_reload;
  req.xOffset = (CORBA::Long)_xoffset;
  req.yOffset = (CORBA::Long)_yoffset;


  SIGNAL_CALL2( "openURL", id(), req );
}

void KonqHTMLView::can( CORBA::Boolean &copy, CORBA::Boolean &paste, CORBA::Boolean &move )
{
  KHTMLView *selectedView = getSelectedView();
  if ( selectedView )
    copy = (CORBA::Boolean)selectedView->isTextSelected();
  else
    copy = (CORBA::Boolean)isTextSelected();

  paste = false;    
  move = false;
}

void KonqHTMLView::copySelection()
{
  QString text;
  KHTMLView *selectedView = getSelectedView();
  if ( selectedView )
    selectedView->getSelectedText( text );
  else
    getSelectedText( text );
    
  QApplication::clipboard()->setText( text );
}

void KonqHTMLView::pasteSelection()
{
  assert( 0 );
}

void KonqHTMLView::moveSelection( const char * )
{
  assert( 0 );
}

void KonqHTMLView::checkViewMenu()
{
  if ( !CORBA::is_nil( m_vViewMenu ) )
  {
    CORBA::WString_var text;
    if ( isFrameSet() )
    {
      text = Q2C( i18n("&Save Frameset As...") );
      m_vViewMenu->changeItemText( text, m_idSaveDocument );
      m_vViewMenu->setItemEnabled( m_idSaveFrame, true );
      m_vViewMenu->setItemEnabled( m_idViewFrame, true );
    } 
    else
    {
      text = Q2C( i18n("&Save As...") );
      m_vViewMenu->changeItemText( text, m_idSaveDocument );
      m_vViewMenu->setItemEnabled( m_idSaveFrame, false );
      m_vViewMenu->setItemEnabled( m_idViewFrame, false );
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
    
    m_vViewMenu->setItemEnabled( m_idSaveBackground, !bURL.isNull() && !bURL.isEmpty() );
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
