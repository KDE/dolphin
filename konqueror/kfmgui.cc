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

#include <qdir.h>

#include "kfmgui.h"
#include "kfmguiprops.h"
#include "kfmpaths.h"
#include "kfmview.h"
#include "kbookmark.h"
#include "kfm_defaults.h"

#include <qmsgbox.h>
#include <qstring.h>
#include <qkeycode.h>

#include <kapp.h>
#include <k2url.h>
#include <kconfig.h>
#include <kio_cache.h>
#include <kurlcompletion.h>
#include <kpixmapcache.h>
#include <kio_linedit_dlg.h>
#include <kkeydialog.h>

#include <iostream>
#include <assert.h>

#define TOOLBAR_URL_ID 1000
#define TOOLBAR_UP_ID 0
#define TOOLBAR_BACK_ID 1
#define TOOLBAR_FORWARD_ID 2

QList<KfmGui>* KfmGui::s_lstWindows = 0L;
QList<QPixmap>* KfmGui::s_lstAnimatedLogo = 0L;

KfmGui::KfmGui( const char *_url ) : KTopLevelWidget()
{
  m_pAccel = new KAccel( this );
  m_pAccel->insertItem( i18n("Switch to left View"), "LeftView", CTRL+Key_1 );
  m_pAccel->insertItem( i18n("Switch to right View"), "RightView", CTRL+Key_2 );
  m_pAccel->insertItem( i18n("Directory up"), "DirUp", ALT+Key_Up );
  m_pAccel->insertItem( i18n("History Back"), "Back", ALT+Key_Left );
  m_pAccel->insertItem( i18n("History Forward"), "Forward", ALT+Key_Right );
  m_pAccel->connectItem( "LeftView", this, SLOT( slotFocusLeftView() ) );
  m_pAccel->connectItem( "RightView", this, SLOT( slotFocusRightView() ) );
  m_pAccel->connectItem( "DirUp", this, SLOT( slotUp() ) );
  m_pAccel->connectItem( "Back", this, SLOT( slotBack() ) );
  m_pAccel->connectItem( "Forward", this, SLOT( slotForward() ) );

  m_pAccel->readSettings();

  m_bInit = true;
  m_bBack = false;
  m_bForward = false;
  
  if ( !s_lstAnimatedLogo )
    s_lstAnimatedLogo = new QList<QPixmap>;
  if ( !s_lstWindows )
    s_lstWindows = new QList<KfmGui>;

  s_lstWindows->setAutoDelete( false );
  s_lstWindows->append( this );
  
  m_pPannerChild0GM = 0L;
  m_pPannerChild1GM = 0L;
  
  m_pCompletion = 0L;
  
  initConfig();

  initGui();

  m_bInit = false;

  m_pView->setFocus();
  m_pView->openURL( _url );
  m_currentView = m_pView;
  m_currentViewMode = m_Props->viewMode();
}

KfmGui::~KfmGui()
{
  delete m_pAccel;
  
  if ( m_pMenu )
    delete m_pMenu;

  if ( m_pMenuNew )
    delete m_pMenuNew;

  m_animatedLogoTimer.stop();
  s_lstWindows->removeRef( this );
}

void KfmGui::initConfig()
{
  // Read application config file if not already done
  if (!KfmGuiProps::m_pDefaultProps)
  {
    debug("Reading global config");
    KConfig *config = kapp->getConfig();
    config->setGroup( "Settings" );
    KfmGuiProps::m_pDefaultProps = new KfmGuiProps(config);
  }
  
  // For the moment, no local properties
  // Copy the default properties
  m_Props = new KfmGuiProps( *KfmGuiProps::m_pDefaultProps );
  
  if ( !m_bInit )
  {
    m_pView->setViewMode( m_Props->m_viewMode );
    m_pView2->setViewMode( m_Props->m_viewMode2 );
  }
  else
    this->resize(m_Props->m_width,m_Props->m_height);
}

void KfmGui::initGui()
{
  initPanner();
  initView();
  initMenu();
  initStatusBar();
  initToolBar();

  if ( m_Props->isShowingDirTree() )
  {    
    /* gl = new QGridLayout( pannerChild0, 1, 1 );
       gl->addWidget( treeView, 0, 0 ); */
  }
  else if ( m_Props->isSplitView() )
  {
    m_pPannerChild0GM = new QGridLayout( m_pPannerChild0, 1, 1 );
    m_pPannerChild0GM->addWidget( m_pView2, 0, 0 );
    m_pPannerChild1GM = new QGridLayout( m_pPannerChild1, 1, 1 );
    m_pPannerChild1GM->addWidget( m_pView, 0, 0 );
  }
  else
  {    
    m_pPannerChild1GM = new QGridLayout( m_pPannerChild1, 1, 1 );
    m_pPannerChild1GM->addWidget( m_pView, 0, 0 );
  }
}

void KfmGui::initPanner()
{
  if ( m_Props->isShowingDirTree() )
    m_pPanner = new KPanner( this, "_panner", KPanner::O_VERTICAL, 30 );
  else if ( m_Props->isSplitView() )
    m_pPanner = new KPanner( this, "_panner", KPanner::O_VERTICAL, 50 );
  else
    m_pPanner = new KPanner( this, "_panner", KPanner::O_VERTICAL, 0 );
    
  m_pPannerChild0 = m_pPanner->child0();
  m_pPannerChild1 = m_pPanner->child1();    
  connect( m_pPanner, SIGNAL( positionChanged() ), this, SLOT( slotPannerChanged() ) );

  setView( m_pPanner );
  m_pPanner->show();
}

void KfmGui::initMenu()
{
  m_pMenu = new KMenuBar( this );  
  
  m_pMenuNew = new KNewMenu();

  QPopupMenu *go = new QPopupMenu;
  go->insertItem( i18n("&Cache"), this, SLOT( slotShowCache() ) );
  go->insertItem( i18n("&History"), this, SLOT( slotShowHistory() ) );
  
  QPopupMenu *file = new QPopupMenu;
  CHECK_PTR( file );
  file->insertItem( i18n("&New"), m_pMenuNew );
  file->insertSeparator();
  file->insertItem( i18n("New &Window"), 
		    this, SLOT(slotNewWindow()) );
  file->insertSeparator();
  file->insertItem( i18n("&Run..."), 
		    this, SLOT(slotRun()) );
  file->insertItem( i18n("Open &Terminal"), 
		    this, SLOT(slotTerminal()), CTRL+Key_T );
  file->insertSeparator();
  file->insertItem( i18n("&Goto"), go );
  file->insertItem( i18n("&Open Location..."),
		    this, SLOT(slotOpenLocation()), CTRL+Key_L );
  file->insertItem( i18n("&Find"), this, 
		    SLOT(slotToolFind()), CTRL+Key_F );
  file->insertSeparator();
  file->insertItem( i18n("&Print..."), 
		    this, SLOT(slotPrint()) );
  file->insertSeparator();        
  file->insertItem( i18n("&Close"), 
		    this, SLOT(slotClose()), CTRL+Key_W );
  file->insertItem( i18n("&Quit..."),  
		    this, SLOT(slotQuit()), CTRL+Key_Q );

  QPopupMenu *edit = new QPopupMenu;
  CHECK_PTR( edit );
  edit->insertItem( i18n("&Copy"), this, 
		    SLOT(slotCopy()), CTRL+Key_C );
  edit->insertItem( i18n("&Paste"), this, 
		    SLOT(slotPaste()), CTRL+Key_V );
  edit->insertItem( i18n("&Move to Trash"), 
		    this, SLOT(slotTrash()) );
  edit->insertItem( i18n("&Delete"), this, 
		    SLOT(slotDelete()) );
  edit->insertSeparator();
  edit->insertItem( i18n("&Select"), this, 
		    SLOT(slotSelect()), CTRL+Key_S );
  
  edit->insertSeparator();
  edit->insertItem( i18n("Mime Types"), this, 
		    SLOT(slotEditMimeTypes()) );
  edit->insertItem( i18n("Applications"), this, 
		    SLOT(slotEditApplications()) );

  edit->insertSeparator();
  edit->insertItem( i18n("Save Geometry"), this, 
		    SLOT(slotSaveGeometry()) );

  m_pViewMenu = new QPopupMenu;
  CHECK_PTR( m_pViewMenu );
  m_pViewMenu->setCheckable(true);
  m_pViewMenu->insertItem( klocale->translate("Show Directory Tr&ee"), 
			   this, SLOT(slotShowTree()) );
  m_pViewMenu->insertItem( klocale->translate("Split &window"), 
			   this, SLOT(slotSplitView()) );
  m_pViewMenu->insertSeparator();
  m_pViewMenu->insertItem( klocale->translate("Show &Dot Files"),
			   this, SLOT(slotShowDot()) );
  m_pViewMenu->insertItem( klocale->translate("Image &Preview"),
			   this, SLOT(slotShowSchnauzer()) );
  m_pViewMenu->insertItem( klocale->translate("&Always Show index.html"),
			   this, SLOT(slotShowHTML()) );
  m_pViewMenu->insertSeparator();
  m_pViewMenu->insertItem( klocale->translate("&Large Icons"),
			   this, SLOT(slotLargeIcons()) );
  m_pViewMenu->insertItem( klocale->translate("&Small Icons"),
			   this, SLOT(slotSmallIcons()) );
  m_pViewMenu->insertItem( klocale->translate("&Tree View"),
			   this, SLOT(slotTreeView()) );
  m_pViewMenu->insertSeparator();
  m_pViewMenu->insertItem( klocale->translate("&Use HTML"),
			   this, SLOT(slotHTMLView()) );
  m_pViewMenu->insertSeparator();
  m_pViewMenu->insertItem( klocale->translate("Rel&oad Tree"),
			   this, SLOT(slotReloadTree()) );
  m_pViewMenu->insertItem( klocale->translate("&Reload Document"),
			   this, SLOT(slotReload()) );
    
  m_pViewMenu->setItemChecked( m_pViewMenu->idAt( 0 ), m_Props->isShowingDirTree() );
  m_pViewMenu->setItemChecked( m_pViewMenu->idAt( 1 ), m_Props->isSplitView() );
  m_pViewMenu->setItemChecked( m_pViewMenu->idAt( 3 ), m_Props->isShowingDotFiles() );
  m_pViewMenu->setItemChecked( m_pViewMenu->idAt( 4 ), m_Props->isShowingImagePreview() );
  // m_pViewMenu->setItemChecked( m_pViewMenu->idAt( 3 ), m_bHTMLView );
  switch( m_pView->viewMode() )
    {
    case KfmView::HOR_ICONS:
      m_pViewMenu->setItemChecked( m_pViewMenu->idAt( 7 ), true );
      break;
    case KfmView::VERT_ICONS:
      m_pViewMenu->setItemChecked( m_pViewMenu->idAt( 8 ), true );
      break;
    case KfmView::FINDER:
      m_pViewMenu->setItemChecked( m_pViewMenu->idAt( 9 ), true );
      break;
    default:
      assert( 0 );
      break;
    }

  m_pViewMenu->setItemChecked( m_pViewMenu->idAt( 11 ), m_pView->isHTMLAllowed() );
  
  m_pBookmarkMenu = new KBookmarkMenu( m_pView );
  
  QPopupMenu *options = new QPopupMenu;
  options->insertItem( i18n("Configure &keys"), this, SLOT( slotConfigureKeys() ) );

  QPopupMenu* help = new QPopupMenu();

  int id = help->insertItem( i18n( "&Contents" ) );
  help->connectItem( id, kapp, SLOT( appHelpActivated() ) );
  help->setAccel( Key_F1, id );

  help->insertSeparator();

  id = help->insertItem( i18n( "&About KFM" ) );
  help->connectItem( id, this, SLOT( slotAboutApp() ) );

  id = help->insertItem( i18n( "About &KDE..." ) );
  help->connectItem( id, this, SLOT( aboutKDE() ) );

  m_pMenu->insertItem( i18n("&File"), file );
  m_pMenu->insertItem( i18n("&Edit"), edit );
  m_pMenu->insertItem( i18n("&View"), m_pViewMenu );
  m_pMenu->insertItem( i18n("&Bookmarks"), m_pBookmarkMenu );
  m_pMenu->insertItem( i18n("&Options"), options );
  m_pMenu->insertSeparator();
  m_pMenu->insertItem( i18n("&Help"), help );

  m_pMenu->show();
  if (m_Props->m_bShowMenuBar )
    m_pMenu->show();
  else
    m_pMenu->hide();
  m_pMenu->setMenuBarPos( m_Props->m_menuBarPos );
  setMenu( m_pMenu );
}

void KfmGui::initStatusBar()
{
  m_pStatusBar = new KStatusBar( this );
 
  m_pStatusBar->insertItem( (char*)i18n("KFM"), 1 );
    
  m_pStatusBar->show();
  // Not implemented yet
  // m_pStatusBar->setPos( m_statusBarPos );
  setStatusBar( m_pStatusBar );
  if ( !m_Props->m_bShowStatusBar )
    m_pStatusBar->enable( KStatusBar::Hide );
}

void KfmGui::initToolBar()
{
  ////////////////////////
  // Add Toolbar
  ////////////////////////

  QString file, path;
  QPixmap pixmap;
  m_pToolbar = new KToolBar(this, "kfmwin-toolbar");
  path = kapp->kde_toolbardir() + "/";

  pixmap = *KPixmapCache::toolbarPixmap( "up.xpm");
  m_pToolbar->insertButton(pixmap, TOOLBAR_UP_ID, SIGNAL( clicked() ), this, 
			       SLOT( slotUp() ), false, 
			       i18n("Up"));
  
  pixmap = *KPixmapCache::toolbarPixmap( "back.xpm");
  m_pToolbar->insertButton(pixmap, TOOLBAR_BACK_ID, SIGNAL( clicked() ), this, 
			       SLOT( slotBack() ), false, 
			       i18n("Back"));
  
  pixmap = *KPixmapCache::toolbarPixmap( "forward.xpm");
  m_pToolbar->insertButton(pixmap, TOOLBAR_FORWARD_ID, SIGNAL( clicked() ), this, 
			       SLOT( slotForward() ), false, 
			       i18n("Forward"));
  
  pixmap = *KPixmapCache::toolbarPixmap( "home.xpm");
  m_pToolbar->insertButton(pixmap, 3, SIGNAL( clicked() ), this, 
			       SLOT( slotHome() ), true, 
			       i18n("Home") );
  
  m_pToolbar->insertSeparator();
  
  pixmap = *KPixmapCache::toolbarPixmap( "reload.xpm");
  m_pToolbar->insertButton(pixmap, 4, SIGNAL( clicked() ), this, 
			       SLOT( slotReload() ), true, 
			       i18n("Reload") );
  
  m_pToolbar->insertSeparator();
  
  pixmap = *KPixmapCache::toolbarPixmap( "editcopy.xpm");
  m_pToolbar->insertButton(pixmap, 5, SIGNAL( clicked() ), this, 
			       SLOT( slotCopy() ), true, 
			       i18n("Copy") );
  
  pixmap = *KPixmapCache::toolbarPixmap( "editpaste.xpm");
  m_pToolbar->insertButton(pixmap, 6, SIGNAL( clicked() ), this, 
			       SLOT( slotPaste() ), true, 
			       i18n("Paste") );
  
  m_pToolbar->insertSeparator();
  
  pixmap = *KPixmapCache::toolbarPixmap( "help.xpm");
  m_pToolbar->insertButton(pixmap, 7, SIGNAL( clicked() ), this, 
			       SLOT( slotHelp() ), true, 
			       i18n("Help"));
  
  m_pToolbar->insertSeparator();
  
  pixmap = *KPixmapCache::toolbarPixmap( "stop.xpm");
  m_pToolbar->insertButton(pixmap, 8, SIGNAL( clicked() ), this, 
			       SLOT( slotStop() ), false, 
			       i18n("Stop"));
  
  path = kapp->kde_datadir() + "/kfm/pics/";
  
  pixmap.load( path + "/kde1.xpm" );
  
  m_pToolbar->insertButton(pixmap, 9, SIGNAL( clicked() ), this, 
			       SLOT( slotNewWindow() ), false );
  m_pToolbar->setItemEnabled( 9, true );
  m_pToolbar->alignItemRight( 9, true );

  ////////////////////////
  // Add animated logo
  ////////////////////////
  if ( s_lstAnimatedLogo->count() == 0 )
  {
    s_lstAnimatedLogo->setAutoDelete( true );
    for ( int i = 1; i <= 9; i++ )
    {
      QString n;
      n.sprintf( "/kde%i.xpm", i );
      QPixmap *p = new QPixmap();
      p->load( path + n );
      if ( p->isNull() )
      {
	QString e;
	cerr << i18n( "Could not load icon\n" ) << n.data();
	QMessageBox::warning( this, i18n( "KFM Error" ), e.data(), i18n( "OK" ) );
      }
      s_lstAnimatedLogo->append( p );
    }
  }

  m_animatedLogoCounter = 0;
  connect( &m_animatedLogoTimer, SIGNAL( timeout() ), this, SLOT( slotAnimatedLogoTimeout() ) );
  
  addToolBar( m_pToolbar );
  m_pToolbar->show();                
  if ( !m_Props->m_bShowToolBar )
    m_pToolbar->enable( KToolBar::Hide );
  m_pToolbar->setBarPos( m_Props->m_toolBarPos );

  ////////////////////////
  // Add Location Bar
  ////////////////////////
  m_pLocationBar = new KToolBar( this, "URL History" );
  QLabel *locationLabel = new QLabel(i18n("Location:"), m_pLocationBar,
                                     "locationLabel");
  locationLabel->adjustSize();
  m_pLocationBar->insertWidget(0, locationLabel->width(), locationLabel); 
  m_pLocationBar->insertLined( "", TOOLBAR_URL_ID,
			       SIGNAL( returnPressed() ), this, SLOT( slotURLEntered() ) );
  KLined *lined = m_pLocationBar->getLined (TOOLBAR_URL_ID); //Sven
  m_pCompletion = new KURLCompletion();
  connect ( lined, SIGNAL ( completion() ),
	    m_pCompletion, SLOT( make_completion() ) );
  connect ( lined, SIGNAL (rotation() ),
	    m_pCompletion, SLOT( make_rotation() ) );
  connect ( lined, SIGNAL ( textChanged(const char *) ),
	    m_pCompletion, SLOT (edited(const char *) ) );
  connect ( m_pCompletion, SIGNAL (setText (const char *) ),
	    lined, SLOT ( setText (const char *) ) );
  addToolBar( m_pLocationBar );
  m_pLocationBar->setFullWidth( TRUE );
  m_pLocationBar->setItemAutoSized( TOOLBAR_URL_ID, TRUE );
  m_pLocationBar->setBarPos( m_Props->m_locationBarPos );
  m_pLocationBar->show();                
  if ( !m_Props->m_bShowLocationBar )
    m_pLocationBar->enable( KToolBar::Hide );
}

void KfmGui::initView()
{
  m_pView = new KfmView( this, m_pPannerChild1 );

  KConfig *config = kapp->getConfig();
  config->setGroup( "Settings" );
  m_pView->setHTMLAllowed( config->readBoolEntry( "AllowHTML", true ) );

  m_pView->show();
  // setView( m_pView );
  m_pView->setFocus();
  
  connect( m_pView, SIGNAL( canceled() ) , 
	   this, SLOT( slotStopAnimation() ) );
  connect( m_pView, SIGNAL( completed() ) , 
	   this, SLOT( slotStopAnimation() ) );
  connect( m_pView, SIGNAL( started() ) , 
	   this, SLOT( slotStartAnimation() ) );
  connect( m_pView, SIGNAL( upURL() ), 
	   this, SLOT( slotUp() ) );
  connect( m_pView, SIGNAL( backHistory() ), 
	   this, SLOT( slotBack() ) );
  connect( m_pView, SIGNAL( forwardHistory() ), 
	   this, SLOT( slotForward() ) );
  connect( m_pView, SIGNAL( gotFocus( KfmView* ) ), 
	   this, SLOT( slotGotFocus( KfmView* ) ) );
}

void KfmGui::slotAboutApp()
{
  kapp->invokeHTMLHelp( "kfm3/about.html", "" );
}

void KfmGui::slotURLEntered()
{
  string url = m_pLocationBar->getLinedText( TOOLBAR_URL_ID );

  // Exit if the user did not enter an URL
  if ( url.empty() )
    return;

  // Root directory?
  if ( url[0] == '/' )
  {
    K2URL::encode( url );
  }
  // Home directory?
  else if ( url[0] == '~' )
  {
    QString tmp( QDir::homeDirPath().data() );
    tmp += m_pLocationBar->getLinedText( TOOLBAR_URL_ID ) + 1;
    K2URL u( tmp );
    url = u.url();
  }
  else if ( strncmp( url.c_str(), "www.", 4 ) == 0 )
  {
    string tmp = "http://";
    K2URL::encode( url );
    tmp += url;
    url = tmp;
  }
  else if ( strncmp( url.c_str(), "ftp.", 4 ) == 0 )
  {
    string tmp = "ftp://";
    K2URL::encode( url );
    tmp += url;
    url = tmp;
  }
  
  K2URL u( url.c_str() );
  if ( u.isMalformed() )
  {
    string tmp = i18n("Malformed URL\n");
    tmp += m_pLocationBar->getLinedText( TOOLBAR_URL_ID );
    QMessageBox::critical( (QWidget*)0L, i18n( "KFM Error" ), tmp.c_str(), i18n( "OK" ) );
    return;
  }
	
  m_bBack = false;
  m_bForward = false;

  m_currentView->openURL( url.c_str() );
}

void KfmGui::slotSplitView()
{
  if ( m_Props->m_bSplitView )
  {
    m_Props->m_bSplitView = false;
    m_pViewMenu->setItemChecked( m_pViewMenu->idAt( 1 ), false );

    m_pPanner->setSeparator( 0 );
    
    delete m_pPannerChild0GM;
    m_pPannerChild0GM = 0L;
    cerr << "!!!!!!!!! Layout done !!!!!!!!!" << endl;

    delete m_pView2;
    m_pView2 = 0L;
    
    m_pView->setFocus();
    m_currentView = m_pView;
    m_currentViewMode = m_Props->m_viewMode;

    return;
  }
  
  if ( m_Props->m_bDirTree )
  {
    m_pViewMenu->setItemChecked( m_pViewMenu->idAt( 0 ), false );
    // TODO: Delete dir tree
  }
  
  m_pViewMenu->setItemChecked( m_pViewMenu->idAt( 1 ), true );
  m_Props->m_bSplitView = true;
  
  QString url = m_pView->currentURL();
  m_Props->m_viewMode = m_pView->viewMode();
  m_pView->clearFocus();
  
  m_pView2 = new KfmView( this, m_pPannerChild0 );
  m_Props->m_viewMode2 = m_Props->m_viewMode;
  m_pView2->setViewMode( m_Props->m_viewMode2, false );
  m_pPannerChild0GM = new QGridLayout( m_pPannerChild0, 1, 1 );
  m_pPannerChild0GM->addWidget( m_pView2, 0, 0 );
  m_pView2->show();
  
  m_pPanner->setSeparator( 50 );

  connect( m_pView2, SIGNAL( canceled() ) , 
	   this, SLOT( slotStopAnimation() ) );
  connect( m_pView2, SIGNAL( completed() ) , 
	   this, SLOT( slotStopAnimation() ) );
  connect( m_pView2, SIGNAL( started() ) , 
	   this, SLOT( slotStartAnimation() ) );
  connect( m_pView2, SIGNAL( upURL() ), 
	   this, SLOT( slotUp() ) );
  connect( m_pView2, SIGNAL( backHistory() ), 
	   this, SLOT( slotBack() ) );
  connect( m_pView2, SIGNAL( forwardHistory() ), 
	   this, SLOT( slotForward() ) );
  connect( m_pView2, SIGNAL( gotFocus( KfmView* ) ), 
	   this, SLOT( slotGotFocus( KfmView* ) ) );

  cerr << "########## Opening " << url << endl;
  
  m_pView2->openURL( url );
  m_pView2->setFocus();
  m_currentView = m_pView2;
  m_currentViewMode = m_Props->m_viewMode2;
}

void KfmGui::slotLargeIcons()
{
  m_pViewMenu->setItemChecked( m_pViewMenu->idAt( 7 ), true );
  m_pViewMenu->setItemChecked( m_pViewMenu->idAt( 8 ), false );
  m_pViewMenu->setItemChecked( m_pViewMenu->idAt( 9 ), false );
  // m_pViewMenu->setItemChecked( m_pViewMenu->idAt( 11 ), false );

  m_currentViewMode = KfmView::HOR_ICONS;
  m_currentView->setViewMode( m_currentViewMode );
}

void KfmGui::slotSmallIcons()
{
  m_pViewMenu->setItemChecked( m_pViewMenu->idAt( 7 ), false );
  m_pViewMenu->setItemChecked( m_pViewMenu->idAt( 8 ), true );
  m_pViewMenu->setItemChecked( m_pViewMenu->idAt( 9 ), false );
  // m_pViewMenu->setItemChecked( m_pViewMenu->idAt( 11 ), false );

  m_currentViewMode = KfmView::VERT_ICONS;
  m_currentView->setViewMode( m_currentViewMode );
}

void KfmGui::slotTreeView()
{
  m_pViewMenu->setItemChecked( m_pViewMenu->idAt( 7 ), false );
  m_pViewMenu->setItemChecked( m_pViewMenu->idAt( 8 ), false );
  m_pViewMenu->setItemChecked( m_pViewMenu->idAt( 9 ), true );
  // m_pViewMenu->setItemChecked( m_pViewMenu->idAt( 11 ), false );

  m_currentViewMode = KfmView::FINDER;
  m_currentView->setViewMode( m_currentViewMode );
}

void KfmGui::slotHTMLView()
{
  // m_pViewMenu->setItemChecked( m_pViewMenu->idAt( 7 ), false );
  // m_pViewMenu->setItemChecked( m_pViewMenu->idAt( 8 ), false );
  // m_pViewMenu->setItemChecked( m_pViewMenu->idAt( 9 ), false );
  m_pViewMenu->setItemChecked( m_pViewMenu->idAt( 11 ), !m_pView->isHTMLAllowed() );

  m_pView->setHTMLAllowed( !m_pView->isHTMLAllowed() );
}

void KfmGui::slotSaveGeometry()
{
  KConfig *config = kapp->getConfig();
  config->setGroup( "Settings" );

  // Update the values in m_Props, if necessary :
  m_Props->m_width = this->width();
  m_Props->m_height = this->height();
  m_Props->m_toolBarPos = m_pToolbar->barPos();
  // m_Props->m_statusBarPos = m_pStatusBar->barPos(); doesn't exist. Hum.
  m_Props->m_menuBarPos = m_pMenu->menuBarPos();
  m_Props->m_locationBarPos = m_pLocationBar->barPos();
  m_Props->saveProps(config);  
}

void KfmGui::slotAnimatedLogoTimeout()
{
  m_animatedLogoCounter++;
  if ( m_animatedLogoCounter == s_lstAnimatedLogo->count() )
    m_animatedLogoCounter = 0;
  m_pToolbar->setButtonPixmap( 9, *( s_lstAnimatedLogo->at( m_animatedLogoCounter ) ) );
}

void KfmGui::slotStartAnimation()
{
  m_animatedLogoCounter = 0;
  m_animatedLogoTimer.start( 50 );
  m_pToolbar->setItemEnabled( 8, true );
}

void KfmGui::slotStopAnimation()
{
  m_animatedLogoTimer.stop();
  m_pToolbar->setButtonPixmap( 9, *( s_lstAnimatedLogo->at( 0 ) ) );
  m_pToolbar->setItemEnabled( 8, false );
  setStatusBarText( i18n("Document: Done") );
}

void KfmGui::setStatusBarText( const char *_text )
{
  m_pStatusBar->changeItem( (char*)_text, 1 );
}

void KfmGui::setLocationBarURL( const char *_url )
{
  m_pLocationBar->setLinedText( TOOLBAR_URL_ID, _url );
}

void KfmGui::setUpURL( const char *_url )
{
  if ( _url == 0 )
    m_strUpURL = "";
  else
    m_strUpURL = _url;

  if ( m_strUpURL.isEmpty() )
    m_pToolbar->setItemEnabled( TOOLBAR_UP_ID, false );
  else
    m_pToolbar->setItemEnabled( TOOLBAR_UP_ID, true );
}

void KfmGui::slotStop()
{
  m_pView->stop();
}

void KfmGui::slotNewWindow()
{
  QString url = m_pView->currentURL();
  KfmGui* g = new KfmGui( url );
  g->show();
}

void KfmGui::slotOpenLocation()
{
  QString url = m_pView->currentURL();
    
  KLineEditDlg l( i18n("Open Location:"), url, this, true );
  int x = l.exec();
  if ( x )
  {
    QString url = l.text();
    url = url.stripWhiteSpace();
    // Exit if the user did not enter an URL
    if ( url.data()[0] == 0 )
      return;
    // Root directory?
    if ( url.data()[0] == '/' )
    {
      url = "file:";
      url += l.text();
    }
    // Home directory?
    else if ( url.data()[0] == '~' )
    {
      url = "file:";
      url += QDir::homeDirPath().data();
      url += l.text() + 1;
    }

    // Some kludge to add protocol specifier on
    // well known Locations
    if ( url.left(4) == "www." )
    {
      url = "http://";
      url += l.text();
    }
    if ( url.left(4) == "ftp." )
    {
      url = "ftp://";
      url += l.text();
    }
    /**
     * Something for fun :-)
     */
    if ( url == "about:kde" )
    {
      url = getenv( "KDEURL" );
      if ( url.isEmpty() )
	url = "http://www.kde.org";
    }
	
    K2URL u( url.data() );
    if ( u.isMalformed() )
    {
      warning(klocale->translate("ERROR: Malformed URL"));
      return;
    }
	
    m_pView->openURL( url );
  }
}

void KfmGui::slotShowCache()
{
  QString file = KIOCache::storeIndex();
  if ( file.isEmpty() )
  {
    QMessageBox::critical( 0L, i18n("KFM Error"), i18n( "Could not write index file" ), i18n( "OK" ) );
    return;
  }
  
  string f = file.data();
  K2URL::encode( f );
  m_pView->openURL( f.c_str() );
}

void KfmGui::slotShowHistory()
{
  // TODO
}

void KfmGui::slotUp()
{
  assert( !m_strUpURL.isEmpty() );
  m_currentView->openURL( m_strUpURL );
}

void KfmGui::slotHome()
{
  QString tmp( QDir::homeDirPath().data() );
  m_currentView->openURL( tmp );
}

void KfmGui::slotBack()
{
  assert( m_lstBack.size() != 0 );
  // m_lstForward.push_front( m_currentHistory );
  History h = m_lstBack.back();
  m_lstBack.pop_back();
  
  if( m_lstBack.size() == 0 )
    m_pToolbar->setItemEnabled( TOOLBAR_BACK_ID, false );
  // if( m_lstForward.size() != 0 )
  // m_pToolbar->setItemEnabled( TOOLBAR_FORWARD_ID, true );

  m_bBack = true;
  
  m_currentView->openURL( h.m_strURL, 0, false, h.m_iXOffset, h.m_iYOffset );
}

void KfmGui::slotForward()
{
  assert( m_lstForward.size() != 0 );
  // m_lstBack.push_back( m_currentHistory() );
  History h = m_lstForward.front();
  m_lstForward.pop_front();

  // if( m_lstBack.size() != 0 )
  // m_pToolbar->setItemEnabled( TOOLBAR_BACK_ID, true );
  if( m_lstForward.size() == 0 )
    m_pToolbar->setItemEnabled( TOOLBAR_FORWARD_ID, false );
  
  m_bForward = true;
  
  m_currentView->openURL( h.m_strURL, 0, false, h.m_iXOffset, h.m_iYOffset );
}

void KfmGui::slotReload()
{
  m_bForward = false;
  m_bBack = false;

  m_currentView->reload();
}

void KfmGui::addHistory( const char *_url, int _xoffset, int _yoffset )
{
  History h;
  h.m_strURL = _url;
  h.m_iXOffset = _xoffset;
  h.m_iYOffset = _yoffset;
  
  if ( m_bBack )
  {
    m_bBack = false;
    
    m_lstForward.push_front( h );
    m_pToolbar->setItemEnabled( TOOLBAR_FORWARD_ID, true );  
    return;
  }

  if ( m_bForward )
  {
    m_bForward = false;
    
    m_lstBack.push_back( h );
    m_pToolbar->setItemEnabled( TOOLBAR_BACK_ID, true );  
    return;
  }
  
  m_lstForward.clear();
  m_lstBack.push_back( h );

  m_pToolbar->setItemEnabled( TOOLBAR_FORWARD_ID, false );  
  m_pToolbar->setItemEnabled( TOOLBAR_BACK_ID, true );
}

void KfmGui::slotConfigureKeys()
{
  KKeyDialog::configureKeys( m_pAccel );
}

void KfmGui::slotFocusLeftView()
{
  slotGotFocus( m_pView2 );
}

void KfmGui::slotFocusRightView()
{
  slotGotFocus( m_pView );
}

void KfmGui::slotGotFocus( KfmView* _view )
{
  if ( m_pView == _view )
  {
    if ( m_pView2 )
      m_pView2->clearFocus();

    m_Props->m_viewMode = m_pView->viewMode();
    m_pView->setFocus();
    m_currentView = m_pView;
    m_currentViewMode = m_Props->m_viewMode;

    setViewModeMenu( m_currentViewMode );
    setLocationBarURL( m_pView->currentURL() );
  }
  else if ( m_pView2 == _view )
  {
    if ( m_pView )
      m_pView->clearFocus();

    m_Props->m_viewMode2 = m_pView2->viewMode();
    m_pView2->setFocus();
    m_currentView = m_pView2;
    m_currentViewMode = m_Props->m_viewMode2;

    setViewModeMenu( m_currentViewMode );
    setLocationBarURL( m_pView2->currentURL() );
  }
}

void KfmGui::setViewModeMenu( KfmView::ViewMode _viewMode )
{
  switch( _viewMode )
  {
  case KfmView::HOR_ICONS:
    m_pViewMenu->setItemChecked( m_pViewMenu->idAt( 7 ), true );
    m_pViewMenu->setItemChecked( m_pViewMenu->idAt( 8 ), false );
    m_pViewMenu->setItemChecked( m_pViewMenu->idAt( 9 ), false );
    break;
  case KfmView::VERT_ICONS:
    m_pViewMenu->setItemChecked( m_pViewMenu->idAt( 7 ), false );
    m_pViewMenu->setItemChecked( m_pViewMenu->idAt( 8 ), true );
    m_pViewMenu->setItemChecked( m_pViewMenu->idAt( 9 ), false );
    break;
  case KfmView::FINDER:
    m_pViewMenu->setItemChecked( m_pViewMenu->idAt( 7 ), false );
    m_pViewMenu->setItemChecked( m_pViewMenu->idAt( 8 ), false );
    m_pViewMenu->setItemChecked( m_pViewMenu->idAt( 9 ), true );
    break;
  default:
    //    assert( 0 );
    break;
  }
}

void KfmGui::createGUI( const char *_url )
{
  KfmGui* g = new KfmGui( _url );
  g->show();
}

/******************************************************
 *
 * Global functions
 *
 ******************************************************/

void openFileManagerWindow( const char *_url )
{
  KfmGui* g = new KfmGui( _url );
  g->show();
}

#include "kfmgui.moc"
