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
#include "konq_frame.h"
#include "konq_childview.h"
#include "konq_factory.h"
#include "konq_progressproxy.h"

#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>

#include <qstring.h>
#include <string.h>
#include <qstringlist.h>
#include <qdir.h>
#include <qclipboard.h>

#include <kcursor.h>
#include <khtml.h>
#include <klocale.h>
#include <kfiledialog.h>
#include <kurl.h>
#include <kio_error.h>
#include <kio_job.h>
#include <kmimetype.h>
#include <konqdefaults.h>
#include <konqsettings.h>
#include <klibloader.h>
#include <kstddirs.h>

class KonqHTMLViewFactory : public KLibFactory
{
public:
  KonqHTMLViewFactory() {}

  virtual QObject* create( QObject*, const char*, const char*, const QStringList & )
  {
    QObject *obj = new KonqHTMLView;
    emit objectCreated( obj );
    return obj;
  }

};

extern "C"
{
  void *init_libkonqhtmlview()
  {
    return new KonqHTMLViewFactory;
  }
};

KonqBrowser::KonqBrowser( KonqHTMLView *htmlView, const char *name )
: KHTMLWidget( htmlView, name )
{
  m_pHTMLView = htmlView;
}

void KonqBrowser::openURL( const QString &url, bool reload, int xOffset, int yOffset, const char * )
{
  emit m_pHTMLView->openURLRequest( url, reload, xOffset, yOffset );
}

KonqHTMLView::KonqHTMLView()
{
  m_pBrowser = new KonqBrowser( this, "konqbrowser" );

  QObject::connect( m_pBrowser, SIGNAL( setTitle( QString ) ),
                    this, SLOT( slotSetTitle( QString ) ) );
  QObject::connect( m_pBrowser, SIGNAL( completed() ),
                    this, SIGNAL( completed() ) );
  QObject::connect( m_pBrowser, SIGNAL( started( const QString & ) ),
                    this, SIGNAL( started() ) );
  QObject::connect( m_pBrowser, SIGNAL( canceled() ),
                    this, SIGNAL( canceled() ) );

  m_bAutoLoadImages = KonqSettings::defaultHTMLSettings()->autoLoadImages();
  bool enableJava = KonqSettings::defaultHTMLSettings()->enableJava();
  QString javaPath = KonqSettings::defaultHTMLSettings()->javaPath();
  javaPath = "$KDEDIR/share/apps/kjava/kjava-classes.zip:"+javaPath+"/lib";
  bool enableJavaScript = KonqSettings::defaultHTMLSettings()->enableJavaScript();

  m_pBrowser->enableJava(enableJava);
  setenv("CLASSPATH",javaPath.latin1(), 1);
  m_pBrowser->enableJScript(enableJavaScript);

  m_paViewDocument = new KAction( i18n( "View Document Source" ), 0, this, SLOT( viewDocumentSource() ), this );
  m_paViewFrame = new KAction( i18n( "View Frame Source" ), 0, this, SLOT( viewFrameSource() ), this );

  actions()->append( BrowserView::ViewAction( m_paViewDocument, BrowserView::MenuView ) );
  actions()->append( BrowserView::ViewAction( m_paViewFrame, BrowserView::MenuView ) );

  slotFrameInserted( m_pBrowser );
}

KonqHTMLView::~KonqHTMLView()
{
  delete m_pBrowser;
}

void KonqHTMLView::openURL( const QString &url, bool reload,
                            int xOffset, int yOffset )
{
  m_bAutoLoadImages = KonqSettings::defaultHTMLSettings()->autoLoadImages();
  m_pBrowser->enableImages( m_bAutoLoadImages );

  m_strURL = url;
  m_pBrowser->KHTMLWidget::openURL( url, reload, xOffset, yOffset );

  if ( m_pBrowser->jobId() )
  {
    KIOJob *job = KIOJob::find( m_pBrowser->jobId() );
    if ( job )
    {
      (void)new KonqProgressProxy( this, job );

      QObject::connect( job, SIGNAL( sigRedirection( int, const char * ) ),
                        this, SLOT( slotDocumentRedirection( int, const char * ) ) );
    }
  }

  updateActions();
}

QString KonqHTMLView::url()
{
  return m_strURL;
}

int KonqHTMLView::xOffset()
{
  return m_pBrowser->contentsX();
}

int KonqHTMLView::yOffset()
{
  return m_pBrowser->contentsY();
}

void KonqHTMLView::stop()
{
  m_pBrowser->slotStop();
}

void KonqHTMLView::saveState( QDataStream &stream )
{
    m_pBrowser->saveState(stream);
}

void KonqHTMLView::restoreState( QDataStream &stream )
{
    m_pBrowser->restoreState(stream);
}

/*
bool KonqHTMLView::mappingFillMenuView( Browser::View::EventFillMenu_ptr viewMenu )
{
  m_vViewMenu = OpenPartsUI::Menu::_duplicate( viewMenu );
  if ( !CORBA::is_nil( viewMenu ) )
  {
    QString text;
    m_idSaveDocument = viewMenu->insertItem4( ( text = i18n("&Save As...") ),
                                              this, "saveDocument", 0, -1, -1 );
    m_idSaveFrame = viewMenu->insertItem4( ( text = i18n("Save &Frame As..." ) ),
                                           this, "saveFrame", 0, -1, -1 );
    m_idSaveBackground = viewMenu->insertItem4( ( text = i18n("Save &Background Image As...") ),
                                                this, "saveBackground", 0, -1, -1 );
    m_idViewDocument = viewMenu->insertItem4( ( text = i18n( "View Document Source" ) ),
                                              this, "viewDocumentSource", 0, -1, -1 );
    m_idViewFrame = viewMenu->insertItem4( ( text = i18n( "View Frame Source" ) ),
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
  if ( KonqSettings::defaultHTMLSettings()->autoLoadImages() )
    return false;

  if ( CORBA::is_nil( viewToolBar.toolBar ) )
    return true;

  if ( viewToolBar.create )
  {
    QString toolTip = i18n( "Load Images" );
    OpenPartsUI::Pixmap_var pix = OPUIUtils::convertPixmap( *KPixmapCache::toolbarPixmap( "image.png" ) );
    viewToolBar.toolBar->insertButton2( pix, TOOLBAR_LOADIMAGES_ID,
                                        SIGNAL(clicked()), this, "slotLoadImages",
					true, toolTip, viewToolBar.startIndex++ );
  }
  else
    viewToolBar.toolBar->removeItem( TOOLBAR_LOADIMAGES_ID );

  return true;
}
*/
void KonqHTMLView::slotMousePressed( const QString &_url,
				     const QPoint &, int _button )
{
  QString url = _url;

  if ( _url.isEmpty() )
    url = m_strURL;

  if ( _button == RightButton )
  {
    KURL u( url );

    mode_t mode = 0;
    if ( u.isLocalFile() )
    {
      struct stat buff;
      if ( stat( u.path(), &buff ) == -1 )
      {
        kioErrorDialog( KIO::ERR_COULD_NOT_STAT, url );
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
#if 0
    KFileItem item( "viewURL" /*whatever*/ , mode, u );
    KFileItemList items;
    items.append( &item );
    m_pMainView->popupMenu( _global, items );
#endif
  }
}

void KonqHTMLView::slotFrameInserted( KHTMLWidget *frame )
{
  QObject::connect( frame, SIGNAL( onURL( const QString &) ),
                    this, SLOT( slotShowURL( const QString &) ) );

  QObject::connect( frame, SIGNAL( mousePressed( const QString &, const QPoint&, int ) ),
                    this, SLOT( slotMousePressed( const QString &, const QPoint&, int ) ) );
		
  QObject::connect( frame, SIGNAL( frameInserted( KBrowser * ) ),
                    this, SLOT( slotFrameInserted( KBrowser * ) ) );		

  QObject::connect( frame, SIGNAL( newWindow( const QString & ) ),
                    this, SLOT( slotNewWindow( const QString & ) ) );

#warning TODO (extension!) (Simon)
//  QObject::connect( frame, SIGNAL( textSelected( KHTMLView *, bool ) ),
//                    this, SIGNAL( selectionChanged() ) );

  KonqSettings *settings = KonqSettings::defaultHTMLSettings();

  frame->setDefaultBGColor( settings->bgColor() );
  frame->setDefaultTextColors( settings->textColor(),
				    settings->linkColor(),
				    settings->vLinkColor() );
  frame->setStandardFont( settings->stdFontName() );
  frame->setFixedFont( settings->fixedFontName() );

  frame->setUnderlineLinks( settings->underlineLink() );

  if ( settings->changeCursor() )
    frame->setURLCursor( KCursor().handCursor() );
  else
    frame->setURLCursor( KCursor().arrowCursor() );		

  updateActions();

  frame->enableImages( m_bAutoLoadImages );
}

void KonqHTMLView::slotShowURL( const QString &_url )
{
  if ( !_url )
  {
    emit setStatusBarText( QString::null );
    return;
  }

  KURL url( _url );
  QString com;

  KMimeType::Ptr typ = KMimeType::findByURL( url );

  if ( typ )
    com = typ->comment( url, false );

  if ( url.isMalformed() )
  {
    QString decodedURL = _url;
    KURL::decode( decodedURL );
    emit setStatusBarText( decodedURL );
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
	emit setStatusBarText( text2 );
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
    emit setStatusBarText( text );
  }
  else
    emit setStatusBarText( url.decodedURL() );
}

void KonqHTMLView::slotSetTitle( QString )
{
#warning TODO (in the Canossa framework) (Simon)
/*
  QString decodedTitle = title;
  KURL::decode( decodedTitle );

  if ( m_pMainView ) //builtin view?
    decodedTitle.prepend( "Konqueror: " );

  m_vMainWindow->setPartCaption( id(), decodedTitle );
*/
}

void KonqHTMLView::slotDocumentRedirection( int, const char *url )
{
  //no need for special stuff, KBrowser does everything for us. Let's just make
  //sure the url in the mainview gets updated
  QString decodedURL = url;
  KURL::decode( decodedURL );
  emit setLocationBarURL( decodedURL );
}

void KonqHTMLView::slotNewWindow( const QString &url )
{
  emit createNewWindow( url );
}

void KonqHTMLView::resizeEvent( QResizeEvent * )
{
  m_pBrowser->setGeometry( 0, 0, width(), height() );
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

#if 0 // ### FIXME (lars)
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
#endif

void KonqHTMLView::print()
{
    // ### FIXME
    //m_pBrowser->print();
}

void KonqHTMLView::saveDocument()
{
/*
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
*/
}

void KonqHTMLView::saveFrame()
{
/*
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
*/
}

void KonqHTMLView::saveBackground()
{
/*
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
*/
}

void KonqHTMLView::viewDocumentSource()
{
  openTxtView( m_strURL );
}

void KonqHTMLView::viewFrameSource()
{
 //FIXME: getSelectedFrame() is not implemented in KHTMLWidget (Simon)
/*
  KHTMLWidget *w = m_pBrowser->getSelectedFrame();
  if ( w )
    openTxtView( w->url() );
*/
}

void KonqHTMLView::slotLoadImages()
{
/*
  m_bAutoLoadImages = true;
  enableImages( m_bAutoLoadImages );

  Browser::EventOpenURL ev;
  ev.url = url();
  ev.reload = true;
  ev.xOffset = xOffset();
  ev.yOffset = yOffset();
  EMIT_EVENT( this, Browser::eventOpenURL, ev );
*/
}

void KonqHTMLView::openTxtView( const QString &url )
{

  // hmmm, tricky, but why not ;-) (Simon)

  QObject *obj = parent();
  while ( obj )
  {
    if ( obj->inherits( "KonqFrame" ) )
      break;

    obj = obj->parent();
  }

  if ( obj && obj->inherits( "KonqFrame" ) )
  {
    KonqChildView *childView = ((KonqFrame *)obj)->childView();
    childView->changeViewMode( "text/plain", url, false );
  }
  else
  {
    KConfig *config = KonqFactory::instance()->config();
    config->setGroup( "Misc Defaults" );
    QString editor = config->readEntry( "Editor", DEFAULT_EDITOR );

    QCString cmd;
    cmd.sprintf( "%s %s &", editor.ascii(), url.ascii() );
    system( cmd.data() );
  }
}

/*
void KonqHTMLView::can( bool &copy, bool &paste, bool &move )
{

  KHTMLView *selectedView = getSelectedView();
  if ( selectedView )
    copy = selectedView->isTextSelected();
  else
    copy = isTextSelected();

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

void KonqHTMLView::moveSelection( const QCString & )
{
  assert( 0 );
}
*/

void KonqHTMLView::updateActions()
{
  m_paViewFrame->setEnabled( m_pBrowser->isFrameSet() );

/*
  if ( !CORBA::is_nil( m_vViewMenu ) )
  {
    QString text;
    if ( isFrameSet() )
    {
      text = i18n("&Save Frameset As...");
      m_vViewMenu->changeItemText( text, m_idSaveDocument );
      m_vViewMenu->setItemEnabled( m_idSaveFrame, true );
      m_vViewMenu->setItemEnabled( m_idViewFrame, true );
    }
    else
    {
      text = i18n("&Save As...");
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
*/
}

#if 0
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
#endif

#include "konq_htmlview.moc"
