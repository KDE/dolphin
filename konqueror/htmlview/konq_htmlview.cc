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

#include <config.h>
#include "konq_htmlview.h"
#include "konq_propsview.h"
#include "konq_factory.h"
//#include "konq_searchdia.h"

#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>

#include <qstring.h>
#include <string.h>
#include <qstringlist.h>
#include <qdir.h>
#include <qclipboard.h>

#include <kcursor.h>
#include <dom/html_element.h>
#include <khtml.h>
#include <klocale.h>
#include <kfiledialog.h>
#include <kurl.h>
#include <kio_error.h>
#include <kio_job.h>
#include <kmimetype.h>
#include <konqdefaults.h>
#include <konq_htmlsettings.h>
#include <klibloader.h>
#include <kstddirs.h>
#include <kfileitem.h>

#include <kaction.h>
#include <kparts/partmanager.h>


class KonqHTMLViewFactory : public KLibFactory
{
public:
  KonqHTMLViewFactory()
  {
    KonqFactory::instanceRef();
  }
  virtual ~KonqHTMLViewFactory()
  {
    KonqFactory::instanceUnref();
  }

  virtual QObject* create( QObject *parent, const char *name, const char*, const QStringList & )
  {
    QObject *obj = new KonqHTMLView( (QWidget*)parent, name );
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

KonqHTMLWidget::KonqHTMLWidget( QWidget * parent, const char *name )
  : KHTMLWidget( parent, name )
{
  initConfig();
}

void KonqHTMLWidget::openURL( const KURL &url, bool reload, int xOffset, int yOffset, const char *post_data )
{
  m_bAutoLoadImages = KonqHTMLSettings::defaultHTMLSettings()->autoLoadImages();
  enableImages( m_bAutoLoadImages );

//warning remove this hack after krash (lars)
//no idea why openURL should emit openURLRequest - this loops ! (David)

    if(post_data)
    {
      // IMO KHTMLWidget should take a KURL as argument (David)
  	KHTMLWidget::openURL(url.url(), reload, xOffset, yOffset, post_data);
	return;
    }

/// Loop! (David)  emit openURLRequest( url.url(), reload, xOffset, yOffset );
  emit openURLRequest( url, reload, xOffset, yOffset );
}


KonqHTMLView::KonqHTMLView( QWidget *parent, const char *name )
 : KParts::ReadOnlyPart( parent, name )
{
  setInstance( KonqFactory::instance() );
  setXMLFile( "konq_htmlview.rc" );

  m_pWidget = new KonqHTMLWidget( parent, "konqhtmlview" );
  setWidget( m_pWidget );

  m_extension = new KonqHTMLViewExtension( this );

  m_iXOffset = 0;
  m_iYOffset = 0;

  QObject::connect( m_pWidget, SIGNAL( setTitle( const QString & ) ),
                    this, SLOT( slotSetTitle( const QString & ) ) );
  QObject::connect( m_pWidget, SIGNAL( completed() ),
                    //this, SIGNAL( completed() ) );
                    this, SLOT( slotCompleted() ) );
  QObject::connect( m_pWidget, SIGNAL( started( const QString & ) ),
                    this, SLOT( slotStarted( const QString & ) ) );
  QObject::connect( m_pWidget, SIGNAL( completed() ),
                    this, SLOT( updateActions() ) );
  QObject::connect( m_pWidget, SIGNAL( canceled() ),
                    this, SLOT( slotCanceled() ) );
                    //this, SIGNAL( canceled() ) );

  // pass signals from the widget directly to the extension
  connect( m_pWidget, SIGNAL( openURLRequest( const KURL &, bool, int, int, const QString & ) ),
           m_extension, SIGNAL( openURLRequest( const KURL &, bool, int, int, const QString & ) ) );
  connect( m_pWidget, SIGNAL( popupMenu( const QPoint &, const KFileItemList & ) ),
           m_extension, SIGNAL( popupMenu( const QPoint &, const KFileItemList & ) ) );
  connect( m_pWidget, SIGNAL( newWindow( const QString & ) ),
           m_extension, SIGNAL( createNewWindow( const QString & ) ) );
  // this one requires some processing...
  connect( m_pWidget, SIGNAL( onURL( const QString & ) ),
           this, SLOT( slotShowURL( const QString & ) ) );

  m_paViewDocument = new KAction( i18n( "View Document Source" ), 0, this, SLOT( viewDocumentSource() ), actionCollection(), "viewDocumentSource" );
  m_paViewFrame = new KAction( i18n( "View Frame Source" ), 0, this, SLOT( viewFrameSource() ), actionCollection(), "viewFrameSource" );
  m_paSaveBackground = new KAction( i18n( "Save &Background Image As.." ), 0, this, SLOT( saveBackground() ), actionCollection(), "saveBackground" );
  m_paSaveDocument = new KAction( i18n( "&Save As.." ), 0, this, SLOT( saveDocument() ), actionCollection(), "saveDocument" );
  m_paSaveFrame = new KAction( i18n( "Save &Frame As.." ), 0, this, SLOT( saveFrame() ), actionCollection(), "saveFrame" );

  // basically, it's inserted into itself :)
  m_pWidget->slotFrameInserted( m_pWidget );
}

KonqHTMLView::~KonqHTMLView()
{
}

bool KonqHTMLView::openURL( const KURL &url )
{
  m_url = url;
  // don't call our reimplementation as it emits openURLRequest! (Simon)
  m_pWidget->KHTMLWidget::openURL( url.url() /*,reload, xOffset, yOffset*/ );

  if ( m_pWidget->jobId() )
  {
    KIOJob *job = KIOJob::find( m_pWidget->jobId() );
    if ( job )
    {
      QObject::connect( job, SIGNAL( sigRedirection( int, const char * ) ),
                        this, SLOT( slotDocumentRedirection( int, const char * ) ) );
    }
  }

  updateActions();

  return true;
}

void KonqHTMLView::slotCanceled()
{
  // a method only because of that errMsg arg... Hmm...
  emit canceled( QString::null );
}

void KonqHTMLView::slotStarted( const QString & )
{
  emit started( m_pWidget->jobId() );
}

void KonqHTMLView::slotCompleted()
{
  // Lars said : not now but earlier (when the height is big enough)
  // and : not if the user already moved the scrollbars
  // Ok, since I'm lazy it will simple to start with (David)
  m_pWidget->setContentsPos( m_iXOffset, m_iYOffset );
  emit completed();
}

bool KonqHTMLView::closeURL()
{
  m_pWidget->slotStop();
  return true; // always true for a read-only part
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
  if ( KonqHTMLSettings::defaultHTMLSettings()->autoLoadImages() )
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

void KonqHTMLWidget::initConfig()
{
  m_bAutoLoadImages = KonqHTMLSettings::defaultHTMLSettings()->autoLoadImages();
  bool enableJava = KonqHTMLSettings::defaultHTMLSettings()->enableJava();
  QString javaPath = KonqHTMLSettings::defaultHTMLSettings()->javaPath();
  // ### hack... fix this
  QString path = getenv("PATH");
  //  if(path.find(javaPath) == -1)
      path += ":" + javaPath + "/bin/";
  javaPath = QString("/share/apps/kjava/kjava-classes.zip:")+javaPath;
  javaPath += "/lib";
  javaPath = getenv("KDEDIR") + javaPath;
  bool enableJavaScript = KonqHTMLSettings::defaultHTMLSettings()->enableJavaScript();

  this->enableJava(enableJava);
  printf("PATH = %s\n", path.latin1());
  printf("CLASSPATH = %s\n", javaPath.latin1());
  setenv("CLASSPATH",javaPath.latin1(), 1);
  setenv("PATH",path.latin1(), 1);
  this->enableJScript(enableJavaScript);
}

void KonqHTMLWidget::slotRightButtonPressed( const QString &_url,
					   const QPoint &_global)
{
    slotMousePressed(_url, _global, RightButton);
}

void KonqHTMLWidget::slotMousePressed( const QString &_url,
				     const QPoint &_global, int _button )
{
debug(" KonqHTMLWidget::slotMousePressed ");
  KURL u;
  if ( _url.isEmpty() )
    u = url();
  else
    u = _url;

  if ( _button == RightButton )
  {
    mode_t mode = 0;
    if ( u.isLocalFile() )
    {
      struct stat buff;
      if ( stat( u.path(), &buff ) == -1 )
      {
        kioErrorDialog( KIO::ERR_COULD_NOT_STAT, u.url() );
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
    KFileItem item( mode, u );
    KFileItemList items;
    items.append( &item );
    emit popupMenu( _global, items );
  }
}

void KonqHTMLWidget::slotFrameInserted( KHTMLWidget *frame )
{
  if ( frame != this )
  {
    // Transmit those frame signals directly to our parent
    QObject::connect( frame, SIGNAL( onURL( const QString &) ),
                      this, SIGNAL( onURL( const QString &) ) );
    QObject::connect( frame, SIGNAL( newWindow( const QString & ) ),
                      this, SIGNAL( newWindow( const QString & ) ) );
  }

  QObject::connect( frame, SIGNAL( mousePressed( const QString &, const QPoint&, int ) ),
                    this, SLOT( slotMousePressed( const QString &, const QPoint&, int ) ) );
  QObject::connect( frame, SIGNAL( popupMenu( const QString &, const QPoint& ) ),
                    this, SLOT( slotRightButtonPressed( const QString &, const QPoint& ) ) );
		
  QObject::connect( frame, SIGNAL( frameInserted( KHTMLWidget * ) ),
                    this, SLOT( slotFrameInserted( KHTMLWidget * ) ) );		

#ifdef __GNUC__
#warning TODO (emit enableAction...) (David)
#endif
//  QObject::connect( frame, SIGNAL( textSelected( KHTMLWidget *, bool ) ),
//                    this, SIGNAL( selectionChanged() ) );

  KonqHTMLSettings *settings = KonqHTMLSettings::defaultHTMLSettings();

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

  // I guess we need a slot in KonqHTMLView connected to "frameInserted" (David)
  //updateActions();

  frame->enableImages( m_bAutoLoadImages );
}

#if 0
void KonqHTMLView::slotSearch()
{
  m_pSearchDialog = new KonqSearchDialog( this );

  QObject::connect( m_pSearchDialog, SIGNAL( findFirst( const QString &, bool, bool ) ),
                    this, SLOT( slotFindFirst( const QString &, bool, bool ) ) );
  QObject::connect( m_pSearchDialog, SIGNAL( findNext( bool, bool ) ),
                    this, SLOT( slotFindNext( bool, bool ) ) );

  m_pSearchDialog->exec();

  delete m_pSearchDialog;
  m_pSearchDialog = 0L;
}
#endif

void KonqHTMLView::slotShowURL( const QString &_url )
{
  if ( !_url )
  {
    emit setStatusBarText( QString::null );
    return;
  }

  KURL url( url(), _url );
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
    // TODO : use KIO::stat() and create a KFileItem out of its result,
   // to use KFileItem::statusBarText()
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

void KonqHTMLView::slotSetTitle( const QString & title )
{
 // IMHO the caller should use myurl.decodedURL() instead (David)
  QString decodedTitle = title;
  KURL::decode( decodedTitle );
  emit setWindowCaption( decodedTitle );
}

void KonqHTMLView::slotDocumentRedirection( int, const char *url )
{
  //no need for special stuff, KonqHTMLWidget does everything for us. Let's just make
  //sure the url in the mainview gets updated
  QString decodedURL = url;
  KURL::decode( decodedURL );
  emit m_extension->setLocationBarURL( decodedURL );
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

void KonqHTMLView::saveDocument()
{
  if ( m_pWidget->isFrameSet() )
  {
    //TODO
  }
  else
  {
    KURL srcURL( url() );

    if ( srcURL.filename(false).isEmpty() )
      srcURL.setFileName( "index.html" );

    KFileDialog *dlg = new KFileDialog( QString::null, i18n("HTML files|* *.html *.htm"),
					m_pWidget , "filedialog", true );
    dlg->setCaption(i18n("Save as"));

    dlg->setSelection( srcURL.filename() );
    if ( dlg->exec() )
      {
	KURL destURL( dlg->selectedURL() );
	if ( !destURL.isMalformed() )
	{
    	  KIOJob *job = new KIOJob;
	  job->copy( url().url(), destURL.url() );
	}
      }

    delete dlg;
  }
}

void KonqHTMLView::saveFrame()
{
  KHTMLWidget *frame = m_pWidget->selectedFrame();

  if ( !frame )
    return;

  KURL srcURL( frame->url() );

  if ( srcURL.filename(false).isEmpty() )
    srcURL.setFileName( "index.html" );

  KFileDialog *dlg = new KFileDialog( QString::null, "*\n*.html\n*.htm",
					m_pWidget , "filedialog", true );
  dlg->setCaption(i18n("Save frameset as"));
  dlg->setSelection( srcURL.filename() );
  if ( dlg->exec() )
  {
    KURL destURL(dlg->selectedURL());
    if ( !destURL.isMalformed() )
    {
      KIOJob *job = new KIOJob;
      job->copy( url().url(), destURL.url() );
    }
  }

  delete dlg;
}

void KonqHTMLView::saveBackground()
{

  QString relURL = m_pWidget->htmlDocument().body().getAttribute( "background" ).string().ascii();

  KURL backgroundURL( url(), relURL );

  KFileDialog *dlg = new KFileDialog( QString::null, "*",
					m_pWidget , "filedialog", true );
  dlg->setCaption(i18n("Save background image as"));

  dlg->setSelection( backgroundURL.filename() );
  if ( dlg->exec() )
  {
    KURL destURL( dlg->selectedURL());
    if ( !destURL.isMalformed() )
    {
      KIOJob *job = new KIOJob;
      job->copy( url().url(), destURL.url() );
    }
  }

  delete dlg;
}

void KonqHTMLView::viewDocumentSource()
{
  openTxtView( url() );
}

void KonqHTMLView::viewFrameSource()
{
  KHTMLWidget *w = m_pWidget->selectedFrame();
  if ( w )
    openTxtView( w->url() );
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

void KonqHTMLView::openTxtView( const KURL &url )
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
    emit m_extension->openURLRequest( url, false, 0, 0, "text/plain" );
  else
  {
    KConfig *config = KonqFactory::instance()->config();
    config->setGroup( "Misc Defaults" );
    QString editor = config->readEntry( "Editor", DEFAULT_EDITOR );

    QCString cmd;
    cmd.sprintf( "%s %s &", editor.ascii(), url.url().ascii() );
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
  qDebug( "void KonqHTMLView::updateActions()" );

  m_paViewFrame->setEnabled( m_pWidget->isFrameSet() );

  QString bgURL;

  if ( m_pWidget->isFrameSet() )
  {
    KHTMLWidget *frame =  m_pWidget->selectedFrame();
    if(frame)
	bgURL = frame->htmlDocument().body().getAttribute( "background" ).string();

    m_paSaveDocument->setText( i18n( "&Save Frameset As.." ) );
    m_paSaveFrame->setEnabled( true );
  }
  else
  {
    bgURL = m_pWidget->htmlDocument().body().getAttribute( "background" ).string();

    m_paSaveDocument->setText( i18n( "&Save As.." ) );
    m_paSaveFrame->setEnabled( false );
  }

  m_paSaveBackground->setEnabled( !bgURL.isEmpty() );

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

/**********************************************
 *
 * KonqHTMLViewExtension
 *
 **********************************************/

KonqHTMLViewExtension::KonqHTMLViewExtension( KonqHTMLView *view, const char *name )
  : KParts::BrowserExtension( view, name )
{
  m_pView = view;
}

void KonqHTMLViewExtension::print()
{
    // ### FIXME
    //m_pView->htmlWidget()->print();
}

int KonqHTMLViewExtension::xOffset()
{
  return m_pView->htmlWidget()->contentsX();
}

int KonqHTMLViewExtension::yOffset()
{
  return m_pView->htmlWidget()->contentsY();
}

void KonqHTMLViewExtension::reparseConfiguration()
{
  // Called by "kfmclient configure".
  // TODO : some stuff not done in initConfig :)
  m_pView->htmlWidget()->initConfig();
}

void KonqHTMLViewExtension::saveLocalProperties()
{
  //  m_pView->m_pProps->saveLocal( KURL( m_HTMLView->url() ) );
}

void KonqHTMLViewExtension::savePropertiesAsDefault()
{
  //  m_pView->m_pProps->saveAsDefault();
}

void KonqHTMLViewExtension::saveState( QDataStream &stream )
{
    m_pView->htmlWidget()->saveState(stream);
}

void KonqHTMLViewExtension::restoreState( QDataStream &stream )
{
    m_pView->htmlWidget()->restoreState(stream);
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
