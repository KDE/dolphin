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

#include <qdir.h>

#include "kfmrun.h"
#include "knewmenu.h"
#include "konq_mainview.h"
#include "kbookmarkmenu.h"
#include "konq_defaults.h"
#include "konq_childview.h"
#include "konq_mainwindow.h"
#include "konq_iconview.h"
#include "konq_htmlview.h"
#include "konq_treeview.h"
#include "konq_partview.h"
#include "konq_txtview.h"
#include "konq_plugins.h"
#include "konq_propsmainview.h"
#include "konq_propsview.h"
#include "knewmenu.h"
#include "kpopupmenu.h"

#include <opUIUtils.h>
#include <opMenu.h>
#include <opMenuIf.h>
#include <opToolBar.h>
#include <opStatusBar.h>

#include <qkeycode.h>
#include <qlayout.h>
#include <qlist.h>
#include <qmsgbox.h>
#include <qpixmap.h>
#include <qpopmenu.h>
#include <qsplitter.h>
#include <qstring.h>
#include <qtimer.h>

#include <kaccel.h>
#include <kapp.h>
#include <kbookmark.h>
#include <kconfig.h>
#include <kio_cache.h>
#include <kio_paste.h>
#include <kkeydialog.h>
#include <klineeditdlg.h>
#include <kpixmapcache.h>
#include <kprocess.h>
#include <kpropsdlg.h>
#include <kstdaccel.h>
#include <kwm.h>

#include <assert.h>

enum _ids {
/////  toolbar gear and lineedit /////
    TOOLBAR_GEAR_ID, TOOLBAR_URL_ID,

////// menu items //////
    MFILE_NEW_ID, MFILE_NEWWINDOW_ID, MFILE_RUN_ID, MFILE_OPENTERMINAL_ID,
    MFILE_OPENLOCATION_ID, MFILE_FIND_ID, MFILE_PRINT_ID, MFILE_CLOSE_ID,

    MEDIT_COPY_ID, MEDIT_PASTE_ID, MEDIT_TRASH_ID, MEDIT_DELETE_ID, MEDIT_SELECT_ID,
    MEDIT_SELECTALL_ID, // MEDIT_FINDINPAGE_ID, MEDIT_FINDNEXT_ID,
    MEDIT_MIMETYPES_ID, MEDIT_APPLICATIONS_ID, 

    MVIEW_SPLITWINDOW_ID, MVIEW_ROWABOVE_ID, MVIEW_ROWBELOW_ID, MVIEW_REMOVEVIEW_ID, 
    MVIEW_SHOWDOT_ID, MVIEW_SHOWHTML_ID,
    MVIEW_LARGEICONS_ID, MVIEW_SMALLICONS_ID, MVIEW_TREEVIEW_ID, 
    MVIEW_RELOAD_ID, MVIEW_STOP_ID,
    // TODO : view frame source, view document source, document encoding

    MGO_UP_ID, MGO_BACK_ID, MGO_FORWARD_ID, MGO_HOME_ID,
    MGO_CACHE_ID, MGO_HISTORY_ID, MGO_MIMETYPES_ID, MGO_APPLICATIONS_ID,
    // TODO : add global mimetypes and apps here

    MOPTIONS_SHOWMENUBAR_ID, MOPTIONS_SHOWSTATUSBAR_ID, 
    MOPTIONS_SHOWTOOLBAR_ID, MOPTIONS_SHOWLOCATIONBAR_ID,
    MOPTIONS_SAVESETTINGS_ID,
    MOPTIONS_SAVELOCALSETTINGS_ID,
    // TODO : "Cache" submenu (clear cache, ...)
    MOPTIONS_CONFIGUREFILEMANAGER_ID,
    MOPTIONS_CONFIGUREBROWSER_ID,
    MOPTIONS_CONFIGUREKEYS_ID,
    
    MHELP_CONTENTS_ID,
    MHELP_ABOUT_ID
};

QList<KonqMainView>* KonqMainView::s_lstWindows = 0L;
QList<OpenPartsUI::Pixmap>* KonqMainView::s_lstAnimatedLogo = 0L;

KonqMainView::KonqMainView( const char *url, QWidget *_parent ) : QWidget( _parent )
{
  ADD_INTERFACE( "IDL:Konqueror/MainView:1.0" );

  setWidget( this );

  OPPartIf::setFocusPolicy( OpenParts::Part::ClickFocus );

  m_vMenuFile = 0L;
  m_vMenuFileNew = 0L;
  m_vMenuEdit = 0L;
  m_vMenuView = 0L;
  m_vMenuGo = 0L;
  m_vMenuBookmarks = 0L;
  m_vMenuOptions = 0L;

  m_vToolBar = 0L;
  m_vLocationBar = 0L;
  m_vMenuBar = 0L;
  m_vStatusBar = 0L;
  
  m_currentView = 0L;
  m_currentId = 0;

  m_sInitialURL = (url) ? url : QDir::homeDirPath().ascii();

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

  if ( !s_lstAnimatedLogo )
    s_lstAnimatedLogo = new QList<OpenPartsUI::Pixmap>;
  if ( !s_lstWindows )
    s_lstWindows = new QList<KonqMainView>;

  s_lstWindows->setAutoDelete( false );
  s_lstWindows->append( this );

  m_lstRows.setAutoDelete( true );
  
  initConfig();

  initPanner();
}

KonqMainView::~KonqMainView()
{
  cleanUp();
}

void KonqMainView::init()
{

  OpenParts::MenuBarManager_var menuBarManager = m_vMainWindow->menuBarManager();
  if ( !CORBA::is_nil( menuBarManager ) )
    menuBarManager->registerClient( id(), this );

  OpenParts::ToolBarManager_var toolBarManager = m_vMainWindow->toolBarManager();
  if ( !CORBA::is_nil( toolBarManager ) )
    toolBarManager->registerClient( id(), this );

  OpenParts::StatusBarManager_var statusBarManager = m_vMainWindow->statusBarManager();
  if ( !CORBA::is_nil( statusBarManager ) )
    m_vStatusBar = statusBarManager->registerClient( id() );

  CORBA::WString_var item = Q2C( i18n("Konqueror :-)") );
  m_vStatusBar->insertItem( item, 1 );

  // will KTM do it for us ?
  //  m_vStatusBar->enable( m_Props->m_bShowStatusBar ? OpenPartsUI::Show : OpenPartsUI::Hide );

  initGui();

  KonqPlugins::installKOMPlugins( this );
  m_bInit = false;
}

void KonqMainView::cleanUp()
{
  if ( m_bIsClean )
    return;

  OpenParts::MenuBarManager_var menuBarManager = m_vMainWindow->menuBarManager();
  if ( !CORBA::is_nil( menuBarManager ) )
    menuBarManager->unregisterClient( id() );

  OpenParts::ToolBarManager_var toolBarManager = m_vMainWindow->toolBarManager();
  if ( !CORBA::is_nil( toolBarManager ) )
    toolBarManager->unregisterClient( id() );

  OpenParts::StatusBarManager_var statusBarManager = m_vMainWindow->statusBarManager();
  if ( !CORBA::is_nil( statusBarManager ) )
    statusBarManager->unregisterClient( id() );

  delete m_pAccel;

  if ( m_vMainWindow->activePartId() == m_currentId )
    m_vMainWindow->setActivePart( 0 );
    
  MapViews::Iterator it = m_mapViews.begin();
  for (; it != m_mapViews.end(); it++ )
      {
	delete it.data();
      }	

  m_mapViews.clear();
  m_lstRows.clear();

  if ( m_pMenuNew )
    delete m_pMenuNew;

  if ( m_pBookmarkMenu )
    delete m_pBookmarkMenu;

  delete m_pMainSplitter;

  m_animatedLogoTimer.stop();
  s_lstWindows->removeRef( this );

  OPPartIf::cleanUp();
}

void KonqMainView::initConfig()
{
  // Read application config file if not already done
  if (!KonqPropsMainView::m_pDefaultProps)
  {
    kdebug(0,1202,"Reading global config");
    KConfig *config = kapp->getConfig();
    config->setGroup( "Settings" );
    KonqPropsMainView::m_pDefaultProps = new KonqPropsMainView(config);
  }

  // For the moment, no local properties
  // Copy the default properties
  m_Props = new KonqPropsMainView( *KonqPropsMainView::m_pDefaultProps );

  if ( !m_bInit )
  {
    // views will set their mode by themselves - to be checked
    //    m_rightView.m_pView->setViewMode( m_Props->rightViewMode() );
    //    m_leftView.m_pView->setViewMode( m_Props->leftViewMode() );
  }
  else
    this->resize(m_Props->m_width,m_Props->m_height);
}

void KonqMainView::initGui()
{
  openURL( m_sInitialURL, (CORBA::Boolean)true );

  if ( s_lstAnimatedLogo->count() == 0 )
  {
    s_lstAnimatedLogo->setAutoDelete( true );
    for ( int i = 1; i < 9; i++ )
    {
      QString n;
      n.sprintf( "kde%i.xpm", i );
      s_lstAnimatedLogo->append( OPUIUtils::convertPixmap( *KPixmapCache::toolbarPixmap( n ) ) );
    }
  }			

  m_animatedLogoCounter = 0;
  QObject::connect( &m_animatedLogoTimer, SIGNAL( timeout() ), this, SLOT( slotAnimatedLogoTimeout() ) );
}

Row * KonqMainView::newRow( bool append )
{
  Row * row = new QSplitter ( QSplitter::Horizontal, m_pMainSplitter );
  row->setOpaqueResize( TRUE ); // don't know whether this should be true or false (David)
  if ( append )
    m_lstRows.append( row );
  else
  {
    m_lstRows.insert( 0, row );
    m_pMainSplitter->moveToFirst( row );
  }
  if (isVisible()) { kdebug(0, 1202, "row->show!!!!"); row->show(); }
  kdebug(0,1202,"newRow() done");
  return row;
}

void KonqMainView::initPanner()
{
  // Create the main splitter
  m_pMainSplitter = new QSplitter ( QSplitter::Vertical, this, "mainsplitter" );
  //m_pMainSplitter->setOpaqueResize( TRUE ); 

  // Create a row, and its splitter
  m_lstRows.clear();
  (void) newRow(true);
}

bool KonqMainView::event( const char* event, const CORBA::Any& value )
{
  EVENT_MAPPER( event, value );

  MAPPING( OpenPartsUI::eventCreateMenuBar, OpenPartsUI::typeCreateMenuBar_ptr, mappingCreateMenubar );
  MAPPING( OpenPartsUI::eventCreateToolBar, OpenPartsUI::typeCreateToolBar_ptr, mappingCreateToolbar );
  MAPPING( OpenParts::eventChildGotFocus, OpenParts::Part_ptr, mappingChildGotFocus );
  MAPPING( OpenParts::eventParentGotFocus, OpenParts::Part_ptr, mappingParentGotFocus );
  MAPPING( Konqueror::eventOpenURL, Konqueror::EventOpenURL, mappingOpenURL );
  MAPPING( Konqueror::eventNewTransfer, Konqueror::EventNewTransfer, mappingNewTransfer );

  END_EVENT_MAPPER;

  return false;
}

bool KonqMainView::mappingCreateMenubar( OpenPartsUI::MenuBar_ptr menuBar )
{
  m_vMenuBar = OpenPartsUI::MenuBar::_duplicate( menuBar );

  if ( CORBA::is_nil( menuBar ) )
  {
    m_vMenuFileNew->disconnect("activated", this, "slotFileNewActivated");
    m_vMenuFileNew->disconnect("aboutToShow", this, "slotFileNewAboutToShow");

    if ( m_pMenuNew )
    {
      delete m_pMenuNew;
      m_pMenuNew = 0L;
    }

    if ( m_pBookmarkMenu )
    {
      delete m_pBookmarkMenu;
      m_pBookmarkMenu = 0L;
    }

    m_vMenuFile = 0L;
    m_vMenuFileNew = 0L;
    m_vMenuEdit = 0L;
    m_vMenuView = 0L;
    m_vMenuGo = 0L;
    m_vMenuBookmarks = 0L;
    m_vMenuOptions = 0L;
    m_vMenuHelp = 0L;

    return true;
  }

  KStdAccel stdAccel;

  CORBA::WString_var text = Q2C( i18n("&File") );
  
  CORBA::Long m_idMenuFile = menuBar->insertMenu( text, m_vMenuFile, -1, -1 );

  text = Q2C( i18n("&New") );
  m_vMenuFile->insertItem8( text, m_vMenuFileNew, MFILE_NEW_ID, -1 );

  m_vMenuFileNew->connect("activated", this, "slotFileNewActivated");
  m_vMenuFileNew->connect("aboutToShow", this, "slotFileNewAboutToShow");
  
  m_pMenuNew = new KNewMenu( m_vMenuFileNew );

  text = Q2C( i18n("New &Window") );
  m_vMenuFile->insertItem4( text, this, "slotNewWindow", stdAccel.openNew(), MFILE_NEWWINDOW_ID, -1 );
  m_vMenuFile->insertSeparator( -1 );
  text = Q2C( i18n("&Run...") );
  m_vMenuFile->insertItem4( text, this, "slotRun", 0, MFILE_RUN_ID, -1  );
  text = Q2C( i18n("Open &Terminal") );
  m_vMenuFile->insertItem4( text, this, "slotTerminal", CTRL+Key_T, MFILE_OPENTERMINAL_ID, -1 );
  m_vMenuFile->insertSeparator( -1 );

  text = Q2C( i18n("&Open Location...") );
  m_vMenuFile->insertItem4( text, this, "slotOpenLocation", stdAccel.open(), MFILE_OPENLOCATION_ID, -1 );
  text = Q2C( i18n("&Find") );
  m_vMenuFile->insertItem4( text, this, "slotToolFind", stdAccel.find(), MFILE_FIND_ID, -1 );
  m_vMenuFile->insertSeparator( -1 );
  text = Q2C( i18n("&Print...") );
  m_vMenuFile->insertItem4( text, this, "slotPrint", stdAccel.print(), MFILE_PRINT_ID, -1 );
  m_vMenuFile->insertSeparator( -1 );
  text = Q2C( i18n("&Close") );
  m_vMenuFile->insertItem4( text, this, "slotClose", stdAccel.close(), MFILE_CLOSE_ID, -1 );

  menuBar->setFileMenu( m_idMenuFile );

  text = Q2C( i18n("&Edit") );
  menuBar->insertMenu( text, m_vMenuEdit, -1, -1 );

  text = Q2C( i18n("&Copy") );
  m_vMenuEdit->insertItem4( text, this, "slotCopy", stdAccel.copy(), MEDIT_COPY_ID, -1 );
  text = Q2C( i18n("&Paste") );
  m_vMenuEdit->insertItem4( text, this, "slotPaste", stdAccel.paste(), MEDIT_PASTE_ID, -1 );
  text = Q2C( i18n("&Move to Trash") );
  m_vMenuEdit->insertItem4( text, this, "slotTrash", stdAccel.cut(), MEDIT_TRASH_ID, -1 );
  text = Q2C( i18n("&Delete") );
  m_vMenuEdit->insertItem4( text, this, "slotDelete", CTRL+Key_Delete, MEDIT_DELETE_ID, -1 );
  m_vMenuEdit->insertSeparator( -1 );

  text = Q2C( i18n("&View") );
  menuBar->insertMenu( text, m_vMenuView, -1, -1 );  
  
  createViewMenu();
  
  text = Q2C( i18n("&Go") );
  menuBar->insertMenu( text, m_vMenuGo, -1, -1 );

  text = Q2C( i18n("&Up") );
  m_vMenuGo->insertItem4( text, this, "slotUp", 0, MGO_UP_ID, -1 );
  text = Q2C( i18n("&Back") );
  m_vMenuGo->insertItem4( text, this, "slotBack", 0, MGO_BACK_ID, -1 );
  text = Q2C( i18n("&Forward") );
  m_vMenuGo->insertItem4( text, this, "slotForward", 0, MGO_FORWARD_ID, -1 );
  text = Q2C( i18n("&Home") );
  m_vMenuGo->insertItem4( text, this, "slotHome", 0, MGO_HOME_ID, -1 );
  m_vMenuGo->insertSeparator( -1 );
  text = Q2C( i18n("&Cache") );
  m_vMenuGo->insertItem4( text, this, "slotShowCache", 0, MGO_CACHE_ID, -1 );
  text = Q2C( i18n("&History") );
  m_vMenuGo->insertItem4( text, this, "slotShowHistory", 0, MGO_HISTORY_ID, -1 );
  text = Q2C( i18n("Mime &Types") );
  m_vMenuGo->insertItem4( text, this, "slotEditMimeTypes", 0, MGO_MIMETYPES_ID, -1 );
  text = Q2C( i18n("App&lications") );
  m_vMenuGo->insertItem4( text, this, "slotEditApplications", 0, MGO_APPLICATIONS_ID, -1 );
  //TODO: Global mime types and applications for root

  text = Q2C( i18n("&Bookmarks") );
  menuBar->insertMenu( text, m_vMenuBookmarks, -1, -1 );
  m_pBookmarkMenu = new KBookmarkMenu( this, m_vMenuBookmarks, this, true );

  text = Q2C( i18n("&Options") );
  menuBar->insertMenu( text, m_vMenuOptions, -1, -1 );

  text = Q2C( i18n("Show &Menubar") );
  m_vMenuOptions->insertItem4( text, this, "slotShowMenubar", 0, MOPTIONS_SHOWMENUBAR_ID, -1 );
  text = Q2C( i18n("Show &Statusbar") );
  m_vMenuOptions->insertItem4( text, this, "slotShowStatusbar", 0, MOPTIONS_SHOWSTATUSBAR_ID, -1 );
  text = Q2C( i18n("Show &Toolbar") );
  m_vMenuOptions->insertItem4( text, this, "slotShowToolbar", 0, MOPTIONS_SHOWTOOLBAR_ID, -1 );
  text = Q2C( i18n("Show &Locationbar") );
  m_vMenuOptions->insertItem4( text, this, "slotShowLocationbar", 0, MOPTIONS_SHOWLOCATIONBAR_ID, -1 );
  m_vMenuOptions->insertSeparator( -1 );
  text = Q2C( i18n("Sa&ve Settings") );
  m_vMenuOptions->insertItem4( text, this, "slotSaveSettings", 0, MOPTIONS_SAVESETTINGS_ID, -1 );
  text = Q2C( i18n("Save Settings for this &URL") );
  m_vMenuOptions->insertItem4( text, this, "slotSaveLocalSettings", 0, MOPTIONS_SAVELOCALSETTINGS_ID, -1 );
  m_vMenuOptions->insertSeparator( -1 );
  // TODO : cache submenu
  text = Q2C( i18n("&Configure File Manager...") );
  m_vMenuOptions->insertItem4( text, this, "slotConfigureFileManager", 0, MOPTIONS_CONFIGUREFILEMANAGER_ID, -1 );
  text = Q2C( i18n("Configure &Browser...") );
  m_vMenuOptions->insertItem4( text, this, "slotConfigureBrowser", 0, MOPTIONS_CONFIGUREBROWSER_ID, -1 );
  text = Q2C( i18n("Configure &keys") );
  m_vMenuOptions->insertItem4( text, this, "slotConfigureKeys", 0, MOPTIONS_CONFIGUREKEYS_ID, -1 );

  text = Q2C( i18n( "&Help" ) );
  CORBA::Long helpId = m_vMenuBar->insertMenu( text, m_vMenuHelp, -1, -1 );
  m_vMenuBar->setHelpMenu( helpId );
  
  text = Q2C( i18n("&Contents" ) );
  m_vMenuHelp->insertItem4( text, this, "slotHelpContents", 0, MHELP_CONTENTS_ID, -1 );

  m_vMenuHelp->insertSeparator( -1 );
  
  text = Q2C( i18n("&About Konqueror" ) );
  m_vMenuHelp->insertItem4( text, this, "slotHelpAbout", 0, MHELP_ABOUT_ID, -1 );

  // Ok, this is wrong. But I don't see where to do it properly
  // (i.e. checking for m_v*Bar->isVisible())
  // We might need a call from KonqMainWindow::readProperties ...
  m_vMenuOptions->setItemChecked( MOPTIONS_SHOWMENUBAR_ID, true );
  m_vMenuOptions->setItemChecked( MOPTIONS_SHOWSTATUSBAR_ID, true );
  m_vMenuOptions->setItemChecked( MOPTIONS_SHOWTOOLBAR_ID, true );
  m_vMenuOptions->setItemChecked( MOPTIONS_SHOWLOCATIONBAR_ID, true );

  if ( m_currentView )
  {
    Konqueror::View_var view = m_currentView->view();
    setItemEnabled( m_vMenuFile, MFILE_PRINT_ID, view->supportsInterface( "IDL:Konqueror/PrintingExtension:1.0" ) );
  }
      
  return true;
}

bool KonqMainView::mappingCreateToolbar( OpenPartsUI::ToolBarFactory_ptr factory )
{
  kdebug(0,1202,"KonqMainView::mappingCreateToolbar");
  OpenPartsUI::Pixmap_var pix;

  if ( CORBA::is_nil(factory) )
     {
       m_vToolBar = 0L;
       m_vLocationBar = 0L;

       return true;
     }

  m_vToolBar = factory->create( OpenPartsUI::ToolBarFactory::Transient );

  m_vToolBar->setFullWidth( true ); // was false (why?). Changed by David so
                                    // that alignItemRight works

  CORBA::WString_var toolTip;
  
  toolTip = Q2C( i18n("Up") );
  pix = OPUIUtils::convertPixmap( *KPixmapCache::toolbarPixmap( "up.xpm" ) );
  m_vToolBar->insertButton2( pix, MGO_UP_ID, SIGNAL(clicked()),
                             this, "slotUp", false, toolTip, -1);

  pix = OPUIUtils::convertPixmap( *KPixmapCache::toolbarPixmap( "back.xpm" ) );
  toolTip = Q2C( i18n("Back") );
  m_vToolBar->insertButton2( pix, MGO_BACK_ID, SIGNAL(clicked()),
                             this, "slotBack", false, toolTip, -1);

  pix = OPUIUtils::convertPixmap( *KPixmapCache::toolbarPixmap( "forward.xpm" ) );
  toolTip = Q2C( i18n("Forward") );
  m_vToolBar->insertButton2( pix, MGO_FORWARD_ID, SIGNAL(clicked()),
                             this, "slotForward", false, toolTip, -1);

  pix = OPUIUtils::convertPixmap( *KPixmapCache::toolbarPixmap( "home.xpm" ) );
  toolTip = Q2C( i18n("Home") );
  m_vToolBar->insertButton2( pix, MGO_HOME_ID, SIGNAL(clicked()),
                             this, "slotHome", true, toolTip, -1);

  m_vToolBar->insertSeparator( -1 );

  pix = OPUIUtils::convertPixmap( *KPixmapCache::toolbarPixmap( "reload.xpm" ) );
  toolTip = Q2C( i18n("Reload") );
  m_vToolBar->insertButton2( pix, MVIEW_RELOAD_ID, SIGNAL(clicked()),
                             this, "slotReload", true, toolTip, -1);

  m_vToolBar->insertSeparator( -1 );

  pix = OPUIUtils::convertPixmap( *KPixmapCache::toolbarPixmap( "editcopy.xpm" ) );
  toolTip = Q2C( i18n("Copy") );
  m_vToolBar->insertButton2( pix, MEDIT_COPY_ID, SIGNAL(clicked()),
                             this, "slotCopy", true, toolTip, -1);

  pix = OPUIUtils::convertPixmap( *KPixmapCache::toolbarPixmap( "editpaste.xpm" ) );
  toolTip = Q2C( i18n("Paste") );
  m_vToolBar->insertButton2( pix, MEDIT_PASTE_ID, SIGNAL(clicked()),
                             this, "slotPaste", true, toolTip, -1);
 				
  m_vToolBar->insertSeparator( -1 );				

  pix = OPUIUtils::convertPixmap( *KPixmapCache::toolbarPixmap( "help.xpm" ) );
  toolTip = Q2C( i18n("Help") );
  m_vToolBar->insertButton2( pix, MHELP_CONTENTS_ID, SIGNAL(clicked()),
                             this, "slotHelpContents", true, toolTip, -1);
				
  m_vToolBar->insertSeparator( -1 );				

  pix = OPUIUtils::convertPixmap( *KPixmapCache::toolbarPixmap( "stop.xpm" ) );
  toolTip = Q2C( i18n("Stop") );
  m_vToolBar->insertButton2( pix, MVIEW_STOP_ID, SIGNAL(clicked()),
                             this, "slotStop", false, toolTip, -1);

  pix = OPUIUtils::convertPixmap( *KPixmapCache::toolbarPixmap( "kde1.xpm" ) );
  m_vToolBar->insertButton2( pix, TOOLBAR_GEAR_ID, SIGNAL(clicked()),
                             this, "slotNewWindow", true, 0L, -1 );
  m_vToolBar->alignItemRight( TOOLBAR_GEAR_ID, true );

  /* will KTM do it for us ?
  m_vToolBar->enable( m_Props->m_bShowToolBar ? OpenPartsUI::Show : OpenPartsUI::Hide );
  m_vToolBar->setBarPos( (OpenPartsUI::BarPosition)(m_Props->m_toolBarPos) );
  */

  m_vLocationBar = factory->create( OpenPartsUI::ToolBarFactory::Transient );

  m_vLocationBar->setFullWidth( true );

  CORBA::WString_var text = Q2C( i18n("Location : ") );
  m_vLocationBar->insertTextLabel( text, -1, -1 );

  toolTip = Q2C( i18n("Current Location") );
  m_vLocationBar->insertLined(0L, TOOLBAR_URL_ID, SIGNAL(returnPressed()), this, "slotURLEntered", true, toolTip, 70, -1 );
  m_vLocationBar->setItemAutoSized( TOOLBAR_URL_ID, true );
  if ( m_currentView )
  {
    text = Q2C( m_currentView->locationBarURL() );
    m_vLocationBar->setLinedText( TOOLBAR_URL_ID, text );
  }
    
  //TODO: support completion in opToolBar->insertLined....

  /* will KTM do it for us ?
  m_vLocationBar->setBarPos( (OpenPartsUI::BarPosition)(m_Props->m_locationBarPos) );
  m_vLocationBar->enable( m_Props->m_bShowLocationBar ? OpenPartsUI::Show : OpenPartsUI::Hide );
  */

  kdebug(0,1202,"KonqMainView::mappingCreateToolbar : done !");
  return true;
}

bool KonqMainView::mappingChildGotFocus( OpenParts::Part_ptr child )
{
  kdebug(0, 1202, "bool KonqMainView::mappingChildGotFocus( OpenParts::Part_ptr child )");
  setActiveView( child->id() );
  return true;
}

bool KonqMainView::mappingParentGotFocus( OpenParts::Part_ptr  )
{
  kdebug(0, 1202, "bool KonqMainView::mappingParentGotFocus( OpenParts::Part_ptr child )");
  // removing view-specific menu entries (view will probably be destroyed !)
  if (m_currentView)
  {
    m_currentView->emitMenuEvents( m_vMenuView, m_vMenuEdit, false );
    m_currentView->repaint();
  }

  // no more active view (even temporarily)
  setUpEnabled( "/", 0 );
  setItemEnabled( m_vMenuGo, MGO_BACK_ID, false );
  setItemEnabled( m_vMenuGo, MGO_FORWARD_ID, false );

  return true;
}

bool KonqMainView::mappingOpenURL( Konqueror::EventOpenURL eventURL )
{
  openURL( eventURL.url, eventURL.reload );
  return true;
}

bool KonqMainView::mappingNewTransfer( Konqueror::EventNewTransfer transfer )
{
  //TODO: provide transfer status information somewhere (statusbar?...needs extension in OpenParts)
  
  KIOJob *job = new KIOJob;
  job->copy( transfer.source.in(), transfer.destination.in() );
  
  return true;
}

void KonqMainView::insertView( Konqueror::View_ptr view,
                               NewViewPosition newViewPosition,
			       const QStringList &serviceTypes )
{
  Row * currentRow;
  if ( m_currentView )
    currentRow = m_currentView->row();
  else // complete beginning, we don't even have a view
    currentRow = m_lstRows.first();

  if (newViewPosition == above || 
      newViewPosition == below)
  {
    kdebug(0,1202,"Creating a new row");
    currentRow = newRow( (newViewPosition == below) ); // append if below
    // Now insert a view, say on the right (doesn't matter)
    newViewPosition = right;
  }

  KonqChildView *v = new KonqChildView( view, currentRow, newViewPosition,
                                        this, serviceTypes );
  QObject::connect( v, SIGNAL(sigIdChanged( KonqChildView *, OpenParts::Id, OpenParts::Id )), 
                    this, SLOT(slotIdChanged( KonqChildView * , OpenParts::Id, OpenParts::Id ) ));

  m_mapViews.insert( view->id(), v );

  v->lockHistory(); // first URL won't go into history

  if (isVisible()) v->show();
  
  setItemEnabled( m_vMenuView, MVIEW_REMOVEVIEW_ID, 
	(m_mapViews.count() > 1) );
}
                                       
void KonqMainView::setActiveView( OpenParts::Id id )
{
  kdebug(0, 1202, "KonqMainView::setActiveView( %d )", id);
  KonqChildView* previousView = m_currentView;
  // clean view-specific part of the view menu
  if ( previousView != 0L )
    previousView->emitMenuEvents( m_vMenuView, m_vMenuEdit, false );

  MapViews::Iterator it = m_mapViews.find( id );
  assert( it != m_mapViews.end() );
  
  m_currentView = it.data();
  assert( m_currentView );
  m_currentId = id;

  setUpEnabled( m_currentView->url(), id );
  setItemEnabled( m_vMenuGo, MGO_BACK_ID, m_currentView->canGoBack() );
  setItemEnabled( m_vMenuGo, MGO_FORWARD_ID, m_currentView->canGoForward() );

  Konqueror::View_var view = m_currentView->view();
  setItemEnabled( m_vMenuFile, MFILE_PRINT_ID, view->supportsInterface( "IDL:Konqueror/PrintingExtension:1.0" ) );

  if ( !CORBA::is_nil( m_vLocationBar ) )
  {
    CORBA::WString_var text = Q2C( m_currentView->locationBarURL() );
    m_vLocationBar->setLinedText( TOOLBAR_URL_ID, text );
  }    

  m_currentView->emitMenuEvents( m_vMenuView, m_vMenuEdit, true );
  if ( isVisible() )
  {
    if (previousView != 0L) // might be 0L e.g. if we just removed the current view
      previousView->repaint();
    m_currentView->repaint();
  }
}

Konqueror::View_ptr KonqMainView::activeView()
{
  if ( m_currentView )
    return m_currentView->view(); //KonqChildView does the necessary duplicate already
  else
    return Konqueror::View::_nil();
}

OpenParts::Id KonqMainView::activeViewId()
{
  return m_currentId;
}

Konqueror::ViewList *KonqMainView::viewList()
{
  Konqueror::ViewList *seq = new Konqueror::ViewList;
  int i = 0;
  seq->length( i );

  MapViews::Iterator it = m_mapViews.begin();

  for (; it != m_mapViews.end(); it++ )
  {
    seq->length( i++ );
    (*seq)[ i ] = it.data()->view();
  }

  return seq;
}

void KonqMainView::removeView( OpenParts::Id id )
{
  if ( m_mapViews.count() == 1 ) //do _NOT_ remove the last view!
    return;

  MapViews::Iterator it = m_mapViews.find( id );
  if ( it != m_mapViews.end() )
  {
    if ( id == m_currentId )
    {
      // give focus to mainwindow. Shouldn't we pick up another view
      // instead ? 
      m_vMainWindow->setActivePart( this->id() );
      m_currentView = 0L;
    }
    QSplitter *splitter = it.data()->row();  
    delete it.data();
    m_mapViews.remove( it );
    
    if ( !splitter->children() )
      m_lstRows.removeRef( splitter );
  }

  setItemEnabled( m_vMenuView, MVIEW_REMOVEVIEW_ID, 
	(m_mapViews.count() > 1) );
}

void KonqMainView::slotIdChanged( KonqChildView * childView, OpenParts::Id oldId, OpenParts::Id newId )
{
  m_mapViews.remove( oldId );
  m_mapViews.insert( newId, childView );
  if ( oldId == m_currentId )
    m_currentId = newId;
}

void KonqMainView::openURL( const Konqueror::URLRequest &url )
{
  CORBA::String_var u = url.url;
  openURL( u.in(), url.reload );
}

void KonqMainView::openURL( const char * _url, CORBA::Boolean )
{
  /////////// First, modify the URL if necessary (adding protocol, ...) //////
  QString url = _url;

  // Root directory?
  if ( url[0] == '/' )
  {
    KURL::encode( url );
  }
  // Home directory?
  else if ( url[0] == '~' )
  {
    QString tmp( QDir::homeDirPath() );
    tmp += _url + 1;
    KURL u( tmp );
    url = u.url();
  }
  else if ( strncmp( url, "www.", 4 ) == 0 )
  {
    QString tmp = "http://";
    KURL::encode( url );
    tmp += url;
    url = tmp;
  }
  else if ( strncmp( url, "ftp.", 4 ) == 0 )
  {
    QString tmp = "ftp://";
    KURL::encode( url );
    tmp += url;
    url = tmp;
  }

  KURL u( url );
  if ( u.isMalformed() )
  {
    QString tmp = i18n("Malformed URL\n%1").arg(_url);
    QMessageBox::critical( (QWidget*)0L, i18n( "Error" ), tmp, i18n( "OK" ) );
    return;
  }

  //FIXME... this is far for being complete (Simon)
  //(for example: obey the reload flag...)
  
  slotStop(); //hm....
    
  KfmRun *run = new KfmRun( this, m_currentView, url, 0, false, false );
  if ( m_currentView )
    m_currentView->setKfmRun( run );
}

void KonqMainView::setStatusBarText( const CORBA::WChar *_text )
{
  if ( !CORBA::is_nil( m_vStatusBar ) )
    m_vStatusBar->changeItem( _text, 1 );
}

void KonqMainView::setLocationBarURL( OpenParts::Id id, const char *_url )
{
  MapViews::Iterator it = m_mapViews.find( id );
  assert( it != m_mapViews.end() );

  CORBA::WString_var wurl = Q2C( QString( _url ) );
    
  it.data()->setLocationBarURL( _url );
  
  if ( ( id == m_currentId ) && (!CORBA::is_nil( m_vLocationBar ) ) )
    m_vLocationBar->setLinedText( TOOLBAR_URL_ID, wurl );
}

void KonqMainView::setItemEnabled( OpenPartsUI::Menu_ptr menu, int id, bool enable )
{
  // enable menu item, and if present in toolbar, related button
  // this will allow configurable toolbar buttons later
  if ( !CORBA::is_nil( menu ) ) 
    menu->setItemEnabled( id, enable );
 
  if ( !CORBA::is_nil( m_vToolBar ) )
    m_vToolBar->setItemEnabled( id, enable );
} 

void KonqMainView::setUpEnabled( QString _url, OpenParts::Id _id )
{
  if ( _id != m_currentId )
    return;

  bool bHasUpURL = false;
  
  if ( !_url.isNull() )
  {
    KURL u( _url );
    if ( u.hasPath() )
      bHasUpURL = ( u.path() != "/");
  }

  setItemEnabled( m_vMenuGo, MGO_UP_ID, bHasUpURL );
}

void KonqMainView::createNewWindow( const char *url )
{
  KonqMainWindow *m_pShell = new KonqMainWindow( url );
  m_pShell->show();
}

bool KonqMainView::openView( const QString &serviceType, const QString &url, KonqChildView *childView )
{
  QString indexFile;
  KURL u( url );

  if ( !m_sInitialURL.isEmpty() )
  {
    Konqueror::View_var vView;
    QStringList serviceTypes;

    if ( ( serviceType == "inode/directory" ) &&
         ( KonqPropsView::defaultProps()->isHTMLAllowed() ) &&
         ( u.isLocalFile() ) &&
	 ( ( indexFile = findIndexFile( u.path() ) ) != QString::null ) )
    {
      KonqChildView::createView( "text/html", vView, serviceTypes, this );
      m_sInitialURL = indexFile;
    }
    else if (!KonqChildView::createView( serviceType, vView, serviceTypes, this ) )
      return false;
      
    insertView( vView, left, serviceTypes );

    MapViews::Iterator it = m_mapViews.find( vView->id() );
    it.data()->openURL( m_sInitialURL );
  
    setActiveView( vView->id() );
    
    m_sInitialURL = QString::null;
    return true;
  }
  
  //first check whether the current view can display this type directly, then
  //try to change the view mode. if this fails, too, then Konqueror cannot
  //display the data addressed by the URL
  if ( childView->supportsServiceType( serviceType ) )
  {
    if ( ( serviceType == "inode/directory" ) &&
         ( childView->allowHTML() ) &&
         ( u.isLocalFile() ) &&
	 ( ( indexFile = findIndexFile( u.path() ) ) != QString::null ) )
      childView->changeViewMode( "text/html", indexFile );
    else
      childView->openURL( url );
      
    childView->setKfmRun( 0L );
    return true;
  }

  if ( childView->changeViewMode( serviceType, url ) )
  {
    childView->setKfmRun( 0L );
    return true;
  }
    
  return false;
}

// protected
void KonqMainView::splitView ( NewViewPosition newViewPosition )
{
  QString url = m_currentView->url();
  const QString serviceType = m_currentView->serviceTypes().first();

  Konqueror::View_var vView;
  QStringList serviceTypes;
  
  if ( !KonqChildView::createView( serviceType, vView, serviceTypes, this ) )
    return; //do not split the view at all if we can't clone the current view
  
  insertView( vView, newViewPosition, serviceTypes );
  MapViews::Iterator it = m_mapViews.find( vView->id() );
  it.data()->openURL( url );
}

void KonqMainView::createViewMenu()
{
  if ( !CORBA::is_nil( m_vMenuView ) )
  {
    m_vMenuView->clear();
  
    CORBA::WString_var text;
    
    m_vMenuView->setCheckable( true );
    //  m_vMenuView->insertItem4( i18n("Show Directory Tr&ee"), this, "slotShowTree" , 0 );
    text = Q2C( i18n("Split &window") );
    m_vMenuView->insertItem4( text, this, "slotSplitView" , 0, MVIEW_SPLITWINDOW_ID, -1 );
    text = Q2C( i18n("Add row &above") );
    m_vMenuView->insertItem4( text, this, "slotRowAbove" , 0, MVIEW_ROWABOVE_ID, -1 );
    text = Q2C( i18n("Add row &below") );
    m_vMenuView->insertItem4( text, this, "slotRowBelow" , 0, MVIEW_ROWBELOW_ID, -1 );
    text = Q2C( i18n("Remove view") );
    m_vMenuView->insertItem4( text, this, "slotRemoveView" , 0, MVIEW_REMOVEVIEW_ID, -1 );
    m_vMenuView->insertSeparator( -1 );
    
    // Two namings for the same thing ! We have to decide ourselves. 
    // I prefer the second one, because of .kde.html
    //m_vMenuView->insertItem4( i18n("&Always Show index.html"), this, "slotShowHTML" , 0, MVIEW_SHOWHTML_ID, -1 );
    text = Q2C( i18n("&Use HTML") );
    m_vMenuView->insertItem4( text, this, "slotShowHTML" , 0, MVIEW_SHOWHTML_ID, -1 );
    
    m_vMenuView->insertSeparator( -1 );

    text = Q2C( i18n("&Large Icons") );
    m_vMenuView->insertItem4( text, this, "slotLargeIcons" , 0, MVIEW_LARGEICONS_ID, -1 );
    text = Q2C( i18n("&Small Icons") );
    m_vMenuView->insertItem4( text, this, "slotSmallIcons" , 0, MVIEW_SMALLICONS_ID, -1 );
    text = Q2C( i18n("&Tree View") );
    m_vMenuView->insertItem4( text, this, "slotTreeView" , 0, MVIEW_TREEVIEW_ID, -1 );
    m_vMenuView->insertSeparator( -1 );

    text = Q2C( i18n("&Reload Document") );
    m_vMenuView->insertItem4( text, this, "slotReload" , Key_F5, MVIEW_RELOAD_ID, -1 );
    text = Q2C( i18n("Sto&p loading") );
    m_vMenuView->insertItem4( text, this, "slotStop" , 0, MVIEW_STOP_ID, -1 );
    //TODO: view frame source, view document source, document encoding

    setItemEnabled( m_vMenuView, MVIEW_REMOVEVIEW_ID, 
	(m_mapViews.count() > 1) );

    if ( m_currentView )
      m_vMenuView->setItemChecked( MVIEW_SHOWHTML_ID, (m_currentView->allowHTML() ) );
  }
}

QString KonqMainView::findIndexFile( const QString &dir )
{
  QDir d( dir );
  
  QString f = d.filePath( "index.html", false );
  if ( QFile::exists( f ) )
    return f;
  
  f = d.filePath( ".kde.html", false );
  if ( QFile::exists( f ) )
    return f; 

  return QString::null;
}

/////////////////////// MENUBAR AND TOOLBAR SLOTS //////////////////

void KonqMainView::slotNewWindow()
{
  QString url = m_currentView->url();
  KonqMainWindow *m_pShell = new KonqMainWindow( url );
  m_pShell->show();
}

void KonqMainView::slotRun()
{
  // HACK: The command is not executed in the directory
  // we are in currently. KWM does not support that yet
  KWM::sendKWMCommand("execute");
}

void KonqMainView::slotTerminal()
{
    KConfig *config = KApplication::getKApplication()->getConfig();
    config->setGroup( "Misc Defaults" ); // TODO : change this in kcmkonq too
    QString term = config->readEntry( "Terminal", DEFAULT_TERMINAL );
 
    QString dir ( QDir::homeDirPath() );
 
    KURL u( m_currentView->url() );
    if ( u.isLocalFile() )
      dir = u.path();

    QString cmd;
    cmd.sprintf("cd \"%s\" ; %s &", dir.data(), term.data());
    system( cmd.data() ); 
}

void KonqMainView::slotOpenLocation()
{
  QString u;
  if (m_currentView)
    u = m_currentView->url();

  KLineEditDlg l( i18n("Open Location:"), u, this, true );
  int x = l.exec();
  if ( x )
  {
    u = l.text();
    u = u.stripWhiteSpace();
    // Exit if the user did not enter an URL
    if ( u.isEmpty() )
      return;
    openURL( u.ascii(), (CORBA::Boolean)false );
  }
}

void KonqMainView::slotToolFind()
{
  KShellProcess proc;
  proc << "kfind";
  
  KURL url;
  if ( m_currentView )
    url = m_currentView->url();

  if( url.isLocalFile() )
    proc << url.directory();

  proc.start(KShellProcess::DontCare);
}

void KonqMainView::slotPrint()
{
  // TODO
  // It was simple in kfm : call KHTMLWidget::print()
  // but now it depends on the view...
  // PROBLEM : KHTMLWidget::print() does too much.
  // It calls QPrinter::setup() AND prints the HTML page

  // We need to : 
  // if current view is a HTML view, call that method
  // otherwise call QPrinter::setup ourselves, and
  // implement print() in icon/tree/text/(part??) views
  // But it's silly to print an icon view, isn't it ?
  // Then why not simply disable "print" if not a HTML view ?
  //  ... ideas please ! (David)
  // Here's a proposal :-) (Simon)
  Konqueror::View_var view = m_currentView->view();
  
  if ( view->supportsInterface( "IDL:Konqueror/PrintingExtension:1.0" ) )
  {
    CORBA::Object_var obj = view->getInterface( "IDL:Konqueror/PrintingExtension:1.0" );
    Konqueror::PrintingExtension_var printExt = Konqueror::PrintingExtension::_narrow( obj );
    printExt->print();
  }
}

void KonqMainView::slotClose()
{
  delete this;
}

void KonqMainView::slotCopy()
{
  // TODO
}

void KonqMainView::slotPaste()
{
  // TODO
}

void KonqMainView::slotTrash()
{
  // TODO
}

void KonqMainView::slotDelete()
{
  // TODO
}

void KonqMainView::slotSplitView()
{
  // Create new view, same URL as current view, on its right.
  splitView( right );
}

void KonqMainView::slotRowAbove()
{
  // Create new row above, with a view, same URL as current view.
  splitView( above );
}

void KonqMainView::slotRowBelow()
{
  // Create new row below, with a view, same URL as current view.
  splitView( below );
}

void KonqMainView::slotRemoveView()
{
  removeView( m_currentId );
}

void KonqMainView::slotShowHTML()
{
  assert( !CORBA::is_nil( m_vMenuView ) );
  assert( m_currentView );
  
  bool b = !m_currentView->allowHTML();
  
  m_currentView->setAllowHTML( b );
  m_vMenuView->setItemChecked( MVIEW_SHOWHTML_ID, b );

  if ( b && m_currentView->supportsServiceType( "inode/directory" ) )
  {
    m_currentView->lockHistory();
    openURL( m_currentView->url(), (CORBA::Boolean)false );
  }
  else if ( !b && m_currentView->supportsServiceType( "text/html" ) )
  {
    KURL u( m_currentView->url() );
    m_currentView->lockHistory();
    openURL( u.directory(), (CORBA::Boolean)false );
  }
}

void KonqMainView::slotLargeIcons()
{
  Konqueror::View_var v;

  if ( m_currentView->viewName() != "KonquerorKfmIconView" )
  {
    v = Konqueror::View::_duplicate( new KonqKfmIconView( this ) );
    QStringList serviceTypes;
    serviceTypes.append( "inode/directory" );
    m_currentView->changeView( v, serviceTypes );
  }
  
  v = m_currentView->view();
  Konqueror::KfmIconView_var iv = Konqueror::KfmIconView::_narrow( v );
  
  iv->slotLargeIcons();
}

void KonqMainView::slotSmallIcons()
{
  Konqueror::View_var v;
  
  if ( m_currentView->viewName() != "KonquerorKfmIconView" )
  {
    v = Konqueror::View::_duplicate( new KonqKfmIconView( this ) );
    QStringList serviceTypes;
    serviceTypes.append( "inode/directory" );
    m_currentView->changeView( v, serviceTypes );
  }
  
  v = m_currentView->view();
  Konqueror::KfmIconView_var iv = Konqueror::KfmIconView::_narrow( v );
  
  iv->slotSmallIcons();
}

void KonqMainView::slotTreeView()
{
  if ( m_currentView->viewName() != "KonquerorKfmTreeView" )
  {
    Konqueror::View_var v = Konqueror::View::_duplicate( new KonqKfmTreeView( this ) );
    QStringList serviceTypes;
    serviceTypes.append( "inode/directory" );
    m_currentView->changeView( v, serviceTypes );
  }
}

void KonqMainView::slotReload()
{
  m_currentView->reload();
}

void KonqMainView::slotStop()
{
  if ( m_currentView )
  {
    m_currentView->stop();
    if ( m_currentView->kfmRun() )
      delete m_currentView->kfmRun();
    m_currentView->setKfmRun( 0L );
  }    
}

void KonqMainView::slotUp()
{
  kdebug(0, 1202, "KonqMainView::slotUp()");
  QString url = m_currentView->url();
  KURL u( url );
  u.cd(".."); // KURL does it for us
  
  openURL( u.url(), false ); // not m_currentView->openURL since the view mode might be different
}

void KonqMainView::slotBack()
{ 
  m_currentView->goBack();

  if( !m_currentView->canGoBack() )
    setItemEnabled( m_vMenuGo, MGO_BACK_ID, false );
}

void KonqMainView::slotForward()
{
  m_currentView->goForward();
  if( !m_currentView->canGoForward() )
    setItemEnabled( m_vMenuGo, MGO_FORWARD_ID, false );
}

void KonqMainView::slotHome()
{
  QString tmp( QDir::homeDirPath() );
  tmp.prepend( "file:" );
  openURL(tmp,(CORBA::Boolean)false); // might need a view-mode change...
}

void KonqMainView::slotShowCache()
{
  QString file = KIOCache::storeIndex();
  if ( file.isEmpty() )
  {
    QMessageBox::critical( 0L, i18n("Error"), i18n( "Could not write index file" ), i18n( "OK" ) );
    return;
  }

  QString f = file;
  KURL::encode( f );
  openURL( f, (CORBA::Boolean)false );
}

void KonqMainView::slotShowHistory()
{
  // TODO
}

void KonqMainView::slotEditMimeTypes()
{
    openURL( kapp->kde_mimedir(), (CORBA::Boolean)false );
}

void KonqMainView::slotEditApplications()
{
    openURL( kapp->kde_appsdir(), (CORBA::Boolean)false );
}

void KonqMainView::slotShowMenubar()
{
  m_vMenuBar->enable( OpenPartsUI::Toggle );
  m_vMenuOptions->setItemChecked( MOPTIONS_SHOWMENUBAR_ID, m_vMenuBar->isVisible() );
}

void KonqMainView::slotShowStatusbar()
{
  m_vStatusBar->enable( OpenPartsUI::Toggle );
  m_vMenuOptions->setItemChecked( MOPTIONS_SHOWSTATUSBAR_ID, m_vStatusBar->isVisible() );
}

void KonqMainView::slotShowToolbar()
{
  m_vToolBar->enable( OpenPartsUI::Toggle );
  m_vMenuOptions->setItemChecked( MOPTIONS_SHOWTOOLBAR_ID, m_vToolBar->isVisible() );
}

void KonqMainView::slotShowLocationbar()
{
  m_vLocationBar->enable( OpenPartsUI::Toggle );
  m_vMenuOptions->setItemChecked( MOPTIONS_SHOWLOCATIONBAR_ID, m_vLocationBar->isVisible() );
}

void KonqMainView::slotSaveSettings()
{
  KConfig *config = kapp->getConfig();
  config->setGroup( "Settings" );

  // Update the values in m_Props, if necessary :
  m_Props->m_width = this->width();
  m_Props->m_height = this->height();
//  m_Props->m_toolBarPos = m_pToolbar->barPos();
  // m_Props->m_statusBarPos = m_pStatusBar->barPos(); doesn't exist. Hum.
//  m_Props->m_menuBarPos = m_pMenu->menuBarPos();
//  m_Props->m_locationBarPos = m_pLocationBar->barPos();
  m_Props->saveProps(config);
}

void KonqMainView::slotSaveLocalSettings()
{
//TODO
}

void KonqMainView::slotConfigureFileManager()
{
//TODO
}

void KonqMainView::slotConfigureBrowser()
{
//TODO
}

void KonqMainView::slotConfigureKeys()
{
  KKeyDialog::configureKeys( m_pAccel );
}

void KonqMainView::slotHelpContents()
{
  kapp->invokeHTMLHelp( "konqueror/index.html", "" );
}

void KonqMainView::slotHelpAbout()
{
  QMessageBox::about( 0L, i18n( "About Konqueror" ), i18n(
"Konqueror Version 0.1\n"
"Author: Torben Weis <weis@kde.org>\n"
"Current maintainer: David Faure <faure@kde.org>\n\n"
"Current team:\n"
"David Faure <faure@kde.org>\n"
"Simon Hausmann <hausmann@kde.org>\n"
"Michael Reiher <michael.reiher@gmx.de>\n"
"Matthias Welk <welk@fokus.gmd.de>\n"
"Waldo Bastian <bastian@kde.org> , Lars Knoll <knoll@mpi-hd.mpg.de> (khtml library)\n"
"Matt Koss <koss@napri.sk>, Alex Zepeda <garbanzo@hooked.net> (kio library/slaves)\n"
  ));
}

void KonqMainView::slotURLEntered()
{
  CORBA::WString_var _url = m_vLocationBar->linedText( TOOLBAR_URL_ID );
  QString url = C2Q( _url.in() );

  // Exit if the user did not enter an URL
  if ( url.isEmpty() )
    return;

  // Root directory?
  if ( url[0] == '/' )
  {
    KURL::encode( url );
  }
  // Home directory?
  else if ( url[0] == '~' )
  {
    QString tmp( QDir::homeDirPath() );
    tmp += C2Q( _url.in() ).remove( 0, 1 );
    KURL u( tmp );
    url = u.url();
  }
  else if ( strncmp( url, "www.", 4 ) == 0 )
  {
    QString tmp = "http://";
    KURL::encode( url );
    tmp += url;
    url = tmp;
  }
  else if ( strncmp( url, "ftp.", 4 ) == 0 )
  {
    QString tmp = "ftp://";
    KURL::encode( url );
    tmp += url;
    url = tmp;
  }

  KURL u( url );
  if ( u.isMalformed() )
  {
    QString tmp = i18n("Malformed URL\n%1").arg(C2Q(_url.in()));
    QMessageBox::critical( (QWidget*)0L, i18n( "Error" ), tmp, i18n( "OK" ) );
    return;
  }
	
  /*
    m_currentView->m_bBack = false;
    m_currentView->m_bForward = false;
    Why this ? Seems not necessary ... (David)
  */ 

  openURL( url, (CORBA::Boolean)false );
}

void KonqMainView::slotBookmarkSelected( CORBA::Long id )
{
  if ( m_pBookmarkMenu )
    m_pBookmarkMenu->slotBookmarkSelected( id );
}

void KonqMainView::slotEditBookmarks()
{
  KBookmarkManager::self()->slotEditBookmarks();
}

void KonqMainView::slotURLStarted( OpenParts::Id id, const char *url )
{
  kdebug(0, 1202, "KonqMainView::slotURLStarted( %d, %s )", id, url);
  if ( !url )
    return;

  MapViews::Iterator it = m_mapViews.find( id );
  
  assert( it != m_mapViews.end() );
  
  if ( id == m_currentId )
    slotStartAnimation();

  it.data()->makeHistory( false /* not completed */, url );
  if ( id == m_currentId )
  {
    setUpEnabled( url, id );
    setItemEnabled( m_vMenuGo, MGO_BACK_ID, m_currentView->canGoBack() );
    setItemEnabled( m_vMenuGo, MGO_FORWARD_ID, m_currentView->canGoForward() );
  }
}

void KonqMainView::slotURLCompleted( OpenParts::Id id )
{
  kdebug(0, 1202, "void KonqMainView::slotURLCompleted( OpenParts::Id id )");

  MapViews::Iterator it = m_mapViews.find( id );
  
  assert( it != m_mapViews.end() );

  if ( id == m_currentId )
    slotStopAnimation();

  it.data()->makeHistory( true /* completed */, QString::null /* not used */);
  if ( id == m_currentId )
  {
    setItemEnabled( m_vMenuGo, MGO_BACK_ID, m_currentView->canGoBack() );
    setItemEnabled( m_vMenuGo, MGO_FORWARD_ID, m_currentView->canGoForward() );
  }
}

void KonqMainView::slotAnimatedLogoTimeout()
{
  m_animatedLogoCounter++;
  if ( m_animatedLogoCounter == s_lstAnimatedLogo->count() )
    m_animatedLogoCounter = 0;

  if ( !CORBA::is_nil( m_vToolBar ) )
    m_vToolBar->setButtonPixmap( TOOLBAR_GEAR_ID, *( s_lstAnimatedLogo->at( m_animatedLogoCounter ) ) );
}

void KonqMainView::slotStartAnimation()
{
  m_animatedLogoCounter = 0;
  m_animatedLogoTimer.start( 50 );
  setItemEnabled( m_vMenuView, MVIEW_STOP_ID, true );
}

void KonqMainView::slotStopAnimation()
{
  m_animatedLogoTimer.stop();

  if ( !CORBA::is_nil( m_vToolBar ) )
  {
    m_vToolBar->setButtonPixmap( TOOLBAR_GEAR_ID, *( s_lstAnimatedLogo->at( 0 ) ) );
    setItemEnabled( m_vMenuView, MVIEW_STOP_ID, false );
  }

  CORBA::WString_var msg = Q2C( i18n("Document: Done") );
  setStatusBarText( msg );
}

void KonqMainView::popupMenu( const QPoint &_global, const QStringList &_urls, mode_t _mode )
{
  KonqPopupMenu * popupMenu = new KonqPopupMenu( _urls, 
                                                 _mode,
                                                 m_currentView->url(),
                                                 m_currentView->canGoBack(),
                                                 m_currentView->canGoForward(),
                                                 !m_vMenuBar->isVisible() ); // hidden ?

  kdebug(0, 1202, "exec()");
  int iSelected = popupMenu->exec( _global );
  kdebug(0, 1202, "deleting popupMenu object");
  delete popupMenu;
  /* Test for konqueror-specific entries. A normal signal/slot mechanism doesn't work here,
     because those slots are virtual. */
  switch (iSelected) {
    case KPOPUPMENU_UP_ID : slotUp(); break;
    case KPOPUPMENU_BACK_ID : slotBack(); break;
    case KPOPUPMENU_FORWARD_ID : slotForward(); break;
    case KPOPUPMENU_SHOWMENUBAR_ID : slotShowMenubar(); break;
  }
}

void KonqMainView::slotFileNewActivated( CORBA::Long id )
{
  if ( m_pMenuNew )
     {
       QStringList urls;
       urls.append( m_currentView->url() );

       m_pMenuNew->setPopupFiles( urls );

       m_pMenuNew->slotNewFile( (int)id );
     }
}

void KonqMainView::slotFileNewAboutToShow()
{
  if ( m_pMenuNew )
    m_pMenuNew->slotCheckUpToDate();
}


void KonqMainView::resizeEvent( QResizeEvent * )
{
  m_pMainSplitter->setGeometry( 0, 0, width(), height() ); 
}

void KonqMainView::openBookmarkURL( const char *url )
{
  kdebug(0,1202,"KonqMainView::openBookmarkURL(%s)",url);
  openURL( url, false );
}
 
QString KonqMainView::currentTitle()
{
  CORBA::WString_var t = m_vMainWindow->partCaption( m_currentId );
  QString title = C2Q( t );
  return title;
}
 
QString KonqMainView::currentURL()
{
  return m_currentView->url();
}

#include "konq_mainview.moc"
