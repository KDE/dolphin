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
#include "konq_defaults.h"
#include "konq_childview.h"
#include "konq_factory.h"
#include "konq_mainwindow.h"
#include "konq_plugins.h"
#include "konq_propsmainview.h"
#include "konq_propsview.h"
#include "kpopupmenu.h"
#include "konq_viewmgr.h"
#include "konq_profiledlg.h"

#include <kbookmarkmenu.h>
#include <kpropsdlg.h>

#include <opUIUtils.h>

#include <qkeycode.h>
#include <qpixmap.h>
#include <qpoint.h>
#include <qregexp.h>
#include <qclipboard.h>

#include <kaccel.h>
#include <kapp.h>
#include <kbookmark.h>
#include <kconfig.h>
#include <kio_cache.h>
#include <kkeydialog.h>
#include <klineeditdlg.h>
#include <kpixmapcache.h>
#include <kprocess.h>
#include <kstdaccel.h>
#include <kstddirs.h>
#include <kwm.h>
#include <kmessagebox.h>
#include <kglobal.h>
#include <userpaths.h>
#include <kprogress.h>

#include <assert.h>
#include <pwd.h>

enum _ids {
/////  toolbar gear and lineedit /////
    TOOLBAR_GEAR_ID, TOOLBAR_URL_ID,

////// menu items //////
    MFILE_NEW_ID, MFILE_NEWWINDOW_ID, MFILE_RUN_ID, MFILE_OPENTERMINAL_ID,
    MFILE_OPENLOCATION_ID, MFILE_FIND_ID, MFILE_PRINT_ID,

    MEDIT_COPY_ID, MEDIT_PASTE_ID, MEDIT_TRASH_ID, MEDIT_DELETE_ID, MEDIT_SELECT_ID,
    MEDIT_SELECTALL_ID, // MEDIT_FINDINPAGE_ID, MEDIT_FINDNEXT_ID,
    MEDIT_MIMETYPES_ID, MEDIT_APPLICATIONS_ID, 

    MVIEW_SHOWDOT_ID, MVIEW_SHOWHTML_ID,
    MVIEW_LARGEICONS_ID, MVIEW_SMALLICONS_ID, MVIEW_SMALLVICONS_ID, MVIEW_TREEVIEW_ID, 
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
    MOPTIONS_RELOADPLUGINS_ID,
    MOPTIONS_CONFIGUREPLUGINS_ID,
    
    MWINDOW_SPLITVIEWHOR_ID, MWINDOW_SPLITVIEWVER_ID, 
    MWINDOW_SPLITWINHOR_ID, MWINDOW_SPLITWINVER_ID, 
    MWINDOW_REMOVEVIEW_ID,
    MWINDOW_DEFAULTPROFILE_ID, 
    MWINDOW_PROFILEDLG_ID,
    MWINDOW_LINKVIEW_ID,

    MHELP_CONTENTS_ID,
    MHELP_ABOUT_ID
};

#define STATUSBAR_LOAD_ID 1
#define STATUSBAR_SPEED_ID 2
#define STATUSBAR_MSG_ID 3

QList<KonqMainView>* KonqMainView::s_lstWindows = 0L;
QList<OpenPartsUI::Pixmap>* KonqMainView::s_lstAnimatedLogo = 0L;

KonqMainView::KonqMainView( const char *url, QWidget *parent ) : QWidget( parent )
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

  m_sInitialURL = (url) ? QString( url ) : QString::null;

  m_pAccel = new KAccel( this );
  m_pAccel->insertItem( i18n("Directory up"), "DirUp", ALT+Key_Up );
  m_pAccel->insertItem( i18n("History Back"), "Back", ALT+Key_Left );
  m_pAccel->insertItem( i18n("History Forward"), "Forward", ALT+Key_Right );
  m_pAccel->insertItem( i18n("Stop Loading"), "Stop", Key_Escape );
  
  m_pAccel->insertItem( i18n("Select"), "Select", 0 );
  m_pAccel->insertItem( i18n("Select All"), "SelectAll", CTRL+Key_A );
  m_pAccel->insertItem( i18n("Unselect"), "Unselect", 0 );
  m_pAccel->insertItem( i18n("Unselect All"), "UnselectAll", 0);

  m_pAccel->insertItem( "Select View 1", CTRL+Key_1, false );
  m_pAccel->insertItem( "Select View 2", CTRL+Key_2, false );
  m_pAccel->insertItem( "Select View 3", CTRL+Key_3, false );
  m_pAccel->insertItem( "Select View 4", CTRL+Key_4, false );
  m_pAccel->insertItem( "Select View 5", CTRL+Key_5, false );
  m_pAccel->insertItem( "Select View 6", CTRL+Key_6, false );
  m_pAccel->insertItem( "Select View 7", CTRL+Key_7, false );
  m_pAccel->insertItem( "Select View 8", CTRL+Key_8, false );
  m_pAccel->insertItem( "Select View 9", CTRL+Key_9, false );
  m_pAccel->insertItem( "Select View 10", CTRL+Key_0, false );
  
  m_pAccel->connectItem( "DirUp", this, SLOT( slotUp() ) );
  m_pAccel->connectItem( "Back", this, SLOT( slotBack() ) );
  m_pAccel->connectItem( "Forward", this, SLOT( slotForward() ) );
  m_pAccel->connectItem( "Stop", this, SLOT( slotStop2() ) );

  m_pAccel->connectItem( "Select", this, SLOT( slotUp() ) );
  m_pAccel->connectItem( "SelectAll", this, SLOT( slotUp() ) );
  m_pAccel->connectItem( "Unselect", this, SLOT( slotUp() ) );
  m_pAccel->connectItem( "UnselectAll", this, SLOT( slotUp() ) );

  m_pAccel->connectItem( "Select View 1", this, SLOT( slotSelectView1() ) );
  m_pAccel->connectItem( "Select View 2", this, SLOT( slotSelectView2() ) );
  m_pAccel->connectItem( "Select View 3", this, SLOT( slotSelectView3() ) );
  m_pAccel->connectItem( "Select View 4", this, SLOT( slotSelectView4() ) );
  m_pAccel->connectItem( "Select View 5", this, SLOT( slotSelectView5() ) );
  m_pAccel->connectItem( "Select View 6", this, SLOT( slotSelectView6() ) );
  m_pAccel->connectItem( "Select View 7", this, SLOT( slotSelectView7() ) );
  m_pAccel->connectItem( "Select View 8", this, SLOT( slotSelectView8() ) );
  m_pAccel->connectItem( "Select View 9", this, SLOT( slotSelectView9() ) );
  m_pAccel->connectItem( "Select View 10", this, SLOT( slotSelectView10() ) );

  m_pAccel->readSettings();

  m_bInit = true;

  if ( !s_lstAnimatedLogo )
    s_lstAnimatedLogo = new QList<OpenPartsUI::Pixmap>;
  if ( !s_lstWindows )
    s_lstWindows = new QList<KonqMainView>;

  s_lstWindows->setAutoDelete( false );
  s_lstWindows->append( this );

  m_pViewManager = 0L;
  m_pProgressBar = 0L;

  initConfig();
  
  QObject::connect( QApplication::clipboard(), SIGNAL( dataChanged() ),
                    this, SLOT( checkEditExtension() ) );
}

KonqMainView::~KonqMainView()
{
  kdebug(0,1202,"KonqMainView::~KonqMainView()");
  cleanUp();
  kdebug(0,1202,"KonqMainView::~KonqMainView() done");
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

  if ( !CORBA::is_nil( m_vStatusBar ) )
  {
    m_pProgressBar = new KProgress( 0, 100, 0, KProgress::Horizontal );
    m_vStatusBar->insertWidget( m_pProgressBar->winId(), 120, STATUSBAR_LOAD_ID );
    CORBA::WString_var item = Q2C( QString::fromLatin1( "XXXXXXXX" ) );
    m_vStatusBar->insertItem( item, STATUSBAR_SPEED_ID );
    m_vStatusBar->insertItem( 0L, STATUSBAR_MSG_ID );
  }

  m_pViewManager = new KonqViewManager( this );

  initGui();

  KonqPlugins::installKOMPlugins( this );
  m_bInit = false;

  m_vMainWindow->setSize(m_Props->m_width,m_Props->m_height);
}

void KonqMainView::cleanUp()
{
  if ( m_bIsClean )
    return;

  kdebug(0,1202,"void KonqMainView::cleanUp()");

  KConfig *config = kapp->getConfig();
  config->setGroup( "Settings" );
  config->writeEntry( "ToolBarCombo", locationBarCombo() );
  config->sync();

  if ( m_mapViews.contains( m_vMainWindow->activePartId() ) )
    m_vMainWindow->setActivePart( id() );

  m_vStatusBar = 0L;

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

  if ( m_pMenuNew )
    delete m_pMenuNew;

  if ( m_pBookmarkMenu )
    delete m_pBookmarkMenu;

  if ( m_pProgressBar )
    delete m_pProgressBar;

  delete m_pViewManager;

  m_currentView = 0L;
  m_currentId = 0;

  m_animatedLogoTimer.stop();
  s_lstWindows->removeRef( this );

  OPPartIf::cleanUp();

  kdebug(0,1202,"void KonqMainView::cleanUp() done");
}

bool KonqMainView::event( const char* event, const CORBA::Any& value )
{
  EVENT_MAPPER( event, value );

  MAPPING( OpenPartsUI::eventCreateMenuBar, OpenPartsUI::typeCreateMenuBar_ptr, mappingCreateMenubar );
  MAPPING( OpenPartsUI::eventCreateToolBar, OpenPartsUI::typeCreateToolBar_ptr, mappingCreateToolbar );
  MAPPING( OpenParts::eventChildGotFocus, OpenParts::Part_ptr, mappingChildGotFocus );
  MAPPING( OpenParts::eventParentGotFocus, OpenParts::Part_ptr, mappingParentGotFocus );
  MAPPING( Browser::eventOpenURL, Browser::EventOpenURL, mappingOpenURL );
  MAPPING( Browser::eventNewTransfer, Browser::EventNewTransfer, mappingNewTransfer );
  MAPPING_WSTRING( Konqueror::eventURLEntered, mappingURLEntered );

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
    m_vMenuEdit->disconnect("aboutToShow", this, "slotMenuEditAboutToShow");
    m_vMenuView->disconnect("aboutToShow", this, "slotMenuViewAboutToShow");
    m_vMenuWindowProfiles->disconnect( "activated", this, "slotViewProfileActivated" );

    m_vMenuFile->disconnect( "highlighted", this, "slotItemHighlighted" );
    m_vMenuEdit->disconnect( "highlighted", this, "slotItemHighlighted" );
    m_vMenuView->disconnect( "highlighted", this, "slotItemHighlighted" );
    m_vMenuGo->disconnect( "highlighted", this, "slotItemHighlighted" );

    if ( m_currentView )
    {
      Browser::View_ptr view = m_currentView->view();
      EMIT_EVENT( view, Browser::View::eventFillMenuEdit, 0L );
      EMIT_EVENT( view, Browser::View::eventFillMenuView, 0L );
    }

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
    m_vMenuWindow = 0L;
    m_vMenuWindowProfiles = 0L;
    m_vMenuHelp = 0L;

    return true;
  }

  OpenPartsUI::Pixmap_var pix;

  KStdAccel stdAccel;

  CORBA::WString_var text = Q2C( i18n("&File") );
  
  CORBA::Long m_idMenuFile = menuBar->insertMenu( text, m_vMenuFile, -1, -1 );

  m_vMenuFile->connect( "highlighted", this, "slotItemHighlighted" );

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
  pix = OPUIUtils::convertPixmap( *KPixmapCache::toolbarPixmap( "fileprint.png" ) );
  text = Q2C( i18n("&Print...") );
  m_vMenuFile->insertItem6( pix, text, this, "slotPrint", stdAccel.print(), MFILE_PRINT_ID, -1 );

  menuBar->setFileMenu( m_idMenuFile );

  text = Q2C( i18n("&Edit") );
  menuBar->insertMenu( text, m_vMenuEdit, -1, -1 );

  m_vMenuEdit->connect( "highlighted", this, "slotItemHighlighted" );

  m_vMenuEdit->connect("aboutToShow", this, "slotMenuEditAboutToShow");
  
  m_bEditMenuDirty = true;
  
  createEditMenu();

  text = Q2C( i18n("&View") );
  menuBar->insertMenu( text, m_vMenuView, -1, -1 );  

  m_vMenuView->connect( "highlighted", this, "slotItemHighlighted" );

  m_vMenuView->connect("aboutToShow", this, "slotMenuViewAboutToShow");
  
  m_bViewMenuDirty = true;
  
  createViewMenu();
  
  text = Q2C( i18n("&Go") );
  menuBar->insertMenu( text, m_vMenuGo, -1, -1 );

  m_vMenuGo->connect( "highlighted", this, "slotItemHighlighted" );

  pix = OPUIUtils::convertPixmap( *KPixmapCache::toolbarPixmap( "up.png" ) );
  text = Q2C( i18n("&Up") );
  m_vMenuGo->insertItem6( pix, text, this, "slotUp", 0, MGO_UP_ID, -1 );
  pix = OPUIUtils::convertPixmap( *KPixmapCache::toolbarPixmap( "back.png" ) );
  text = Q2C( i18n("&Back") );
  m_vMenuGo->insertItem6( pix, text, this, "slotBack", 0, MGO_BACK_ID, -1 );
  pix = OPUIUtils::convertPixmap( *KPixmapCache::toolbarPixmap( "forward.png" ) );
  text = Q2C( i18n("&Forward") );
  m_vMenuGo->insertItem6( pix, text, this, "slotForward", 0, MGO_FORWARD_ID, -1 );
  pix = OPUIUtils::convertPixmap( *KPixmapCache::toolbarPixmap( "home.png" ) );
  text = Q2C( i18n("&Home") );
  m_vMenuGo->insertItem6( pix, text, this, "slotHome", 0, MGO_HOME_ID, -1 );
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
  text = Q2C( i18n("Reload Plugins") );
  m_vMenuOptions->insertItem4( text, this, "slotReloadPlugins", 0, MOPTIONS_RELOADPLUGINS_ID, -1 );
  text = Q2C( i18n("Configure Plugins...") );
  m_vMenuOptions->insertItem4( text, this, "slotConfigurePlugins", 0, MOPTIONS_CONFIGUREPLUGINS_ID, -1 );

  text = Q2C( i18n( "&Window" ) );
  m_vMenuBar->insertMenu( text, m_vMenuWindow, -1, -1 );
  
  text = Q2C( i18n( "Split View &Horizontally" ) );
  m_vMenuWindow->insertItem4( text, this, "slotSplitViewHorizontal", 0, MWINDOW_SPLITVIEWHOR_ID, -1 );
  text = Q2C( i18n( "Split View &Vertically" ) );
  m_vMenuWindow->insertItem4( text, this, "slotSplitViewVertical", 0, MWINDOW_SPLITVIEWVER_ID, -1 );
  text = Q2C( i18n( "Split Window &Horizontally" ) );
  m_vMenuWindow->insertItem4( text, this, "slotSplitWindowHorizontal", 0, MWINDOW_SPLITWINHOR_ID, -1 );
  text = Q2C( i18n( "Split Window &Vertically" ) );
  m_vMenuWindow->insertItem4( text, this, "slotSplitWindowVertical", 0, MWINDOW_SPLITWINVER_ID, -1 );
  text = Q2C( i18n( "Remove Active View" ) );
  m_vMenuWindow->insertItem4( text, this, "slotRemoveView", CTRL+Key_R, MWINDOW_REMOVEVIEW_ID, -1 );
  
  m_vMenuWindow->insertSeparator( -1 );
  
  text = Q2C( i18n( "Save Current Profile As Default" ) );
  m_vMenuWindow->insertItem4( text, this, "slotSaveDefaultProfile", 0, MWINDOW_DEFAULTPROFILE_ID, -1 );
  
  m_vMenuWindow->insertSeparator( -1 );
  
  text = Q2C( i18n( "Save/Remove View Profile" ) );
  m_vMenuWindow->insertItem4( text, this, "slotProfileDlg", 0, MWINDOW_PROFILEDLG_ID, -1 );
  text = Q2C( i18n( "Load View Profile" ) );
  m_vMenuWindow->insertItem8( text, m_vMenuWindowProfiles, -1, -1 );
  
  m_vMenuWindowProfiles->connect( "activated", this, "slotViewProfileActivated" );
  
  fillProfileMenu();

  m_vMenuBar->insertSeparator( -1 );
  
  text = Q2C( i18n( "&Help" ) );
  CORBA::Long helpId = m_vMenuBar->insertMenu( text, m_vMenuHelp, -1, -1 );
  m_vMenuBar->setHelpMenu( helpId );
  
  text = Q2C( i18n("&Contents" ) );
  m_vMenuHelp->insertItem4( text, this, "slotHelpContents", stdAccel.help(), MHELP_CONTENTS_ID, -1 );

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

  checkPrintingExtension();

  return true;
}

bool KonqMainView::mappingCreateToolbar( OpenPartsUI::ToolBarFactory_ptr factory )
{
  kdebug(0,1202,"KonqMainView::mappingCreateToolbar");
  OpenPartsUI::Pixmap_var pix;

  if ( CORBA::is_nil(factory) )
     {
       m_vToolBar->disconnect( "highlighted", this, "slotItemHighlighted" );
     
       m_vHistoryBackPopupMenu->disconnect( "aboutToShow", this, "slotHistoryBackwardAboutToShow" );
       m_vHistoryBackPopupMenu->disconnect( "activated", this, "slotHistoryBackwardActivated" );
       m_vHistoryForwardPopupMenu->disconnect( "aboutToShow", this, "slotHistoryForwardAboutToShow" );
       m_vHistoryForwardPopupMenu->disconnect( "activated", this, "slotHistoryForwardActivated" );
       
       m_vUpPopupMenu->disconnect( "aboutToShow", this, "slotUpAboutToShow" );
       m_vUpPopupMenu->disconnect( "activated", this, "slotUpActivated" );
       
       m_vHistoryBackPopupMenu = 0L;
       m_vHistoryForwardPopupMenu = 0L;
       
       m_vUpPopupMenu = 0L;
     
       m_lstLocationBarCombo = locationBarCombo();
     
       if ( m_currentView )
       {
         Browser::View::EventFillToolBar ev;
	 ev.create = (CORBA::Boolean)false;
	 ev.toolBar = OpenPartsUI::ToolBar::_duplicate( m_vToolBar );
	 EMIT_EVENT( m_currentView->view(), Browser::View::eventFillToolBar, ev );
       }
     
       m_vToolBar = 0L;
       m_vLocationBar = 0L;
       
       return true;
     }

  if ( m_Props->bigToolBar() )
  {
    m_vToolBar = factory->create2( OpenPartsUI::ToolBarFactory::Transient, 40 );

    m_vToolBar->setIconText( 3 );
  }
  else
    m_vToolBar = factory->create( OpenPartsUI::ToolBarFactory::Transient );

  m_vToolBar->setFullWidth( true ); // was false (why?). Changed by David so
                                    // that alignItemRight works

  m_vToolBar->connect( "highlighted", this, "slotItemHighlighted" );
				    
  CORBA::WString_var toolTip;
  
  toolTip = Q2C( i18n("Up") );
  pix = OPUIUtils::convertPixmap( *KPixmapCache::toolbarPixmap( "up.png" ) );
  m_vToolBar->insertButton2( pix, MGO_UP_ID, SIGNAL(clicked()),
                             this, "slotUp", false, toolTip, -1);

  m_vToolBar->setDelayedPopup( MGO_UP_ID, m_vUpPopupMenu );

  m_vUpPopupMenu->connect( "aboutToShow", this, "slotUpAboutToShow" );
  m_vUpPopupMenu->connect( "activated", this, "slotUpActivated" );

  pix = OPUIUtils::convertPixmap( *KPixmapCache::toolbarPixmap( "back.png" ) );
  toolTip = Q2C( i18n("Back") );
  m_vToolBar->insertButton2( pix, MGO_BACK_ID, SIGNAL(clicked()),
                             this, "slotBack", false, toolTip, -1);


  m_vToolBar->setDelayedPopup( MGO_BACK_ID, m_vHistoryBackPopupMenu );
  
  m_vHistoryBackPopupMenu->connect( "aboutToShow", this, "slotHistoryBackwardAboutToShow" );
  m_vHistoryBackPopupMenu->connect( "activated", this, "slotHistoryBackwardActivated" );

  pix = OPUIUtils::convertPixmap( *KPixmapCache::toolbarPixmap( "forward.png" ) );
  toolTip = Q2C( i18n("Forward") );
  m_vToolBar->insertButton2( pix, MGO_FORWARD_ID, SIGNAL(clicked()),
                             this, "slotForward", false, toolTip, -1);

  m_vToolBar->setDelayedPopup( MGO_FORWARD_ID, m_vHistoryForwardPopupMenu );
  
  m_vHistoryForwardPopupMenu->connect( "aboutToShow", this, "slotHistoryForwardAboutToShow" );
  m_vHistoryForwardPopupMenu->connect( "activated", this, "slotHistoryForwardActivated" );

  pix = OPUIUtils::convertPixmap( *KPixmapCache::toolbarPixmap( "home.png" ) );
  toolTip = Q2C( i18n("Home") );
  m_vToolBar->insertButton2( pix, MGO_HOME_ID, SIGNAL(clicked()),
                             this, "slotHome", true, toolTip, -1);

  m_vToolBar->insertSeparator( -1 );

  pix = OPUIUtils::convertPixmap( *KPixmapCache::toolbarPixmap( "reload.png" ) );
  toolTip = Q2C( i18n("Reload") );
  m_vToolBar->insertButton2( pix, MVIEW_RELOAD_ID, SIGNAL(clicked()),
                             this, "slotReload", true, toolTip, -1);

  m_vToolBar->insertSeparator( -1 );

  pix = OPUIUtils::convertPixmap( *KPixmapCache::toolbarPixmap( "editcopy.png" ) );
  toolTip = Q2C( i18n("Copy") );
  m_vToolBar->insertButton2( pix, MEDIT_COPY_ID, SIGNAL(clicked()),
                             this, "slotCopy", true, toolTip, -1);

  pix = OPUIUtils::convertPixmap( *KPixmapCache::toolbarPixmap( "editpaste.png" ) );
  toolTip = Q2C( i18n("Paste") );
  m_vToolBar->insertButton2( pix, MEDIT_PASTE_ID, SIGNAL(clicked()),
                             this, "slotPaste", true, toolTip, -1);

  pix = OPUIUtils::convertPixmap( *KPixmapCache::toolbarPixmap( "fileprint.png" ) );
  toolTip = Q2C( i18n( "Print" ) );
  m_vToolBar->insertButton2( pix, MFILE_PRINT_ID, SIGNAL(clicked()),
                             this, "slotPrint", true, toolTip, -1 );
 				
  m_vToolBar->insertSeparator( -1 );				

  pix = OPUIUtils::convertPixmap( *KPixmapCache::toolbarPixmap( "help.png" ) );
  toolTip = Q2C( i18n("Help") );
  m_vToolBar->insertButton2( pix, MHELP_CONTENTS_ID, SIGNAL(clicked()),
                             this, "slotHelpContents", true, toolTip, -1);
				
  m_vToolBar->insertSeparator( -1 );				

  pix = OPUIUtils::convertPixmap( *KPixmapCache::toolbarPixmap( "stop.png" ) );
  toolTip = Q2C( i18n("Stop") );
  m_vToolBar->insertButton2( pix, MVIEW_STOP_ID, SIGNAL(clicked()),
                             this, "slotStop", false, toolTip, -1);

  pix = OPUIUtils::convertPixmap( *KPixmapCache::toolbarPixmap( "kde1.png" ) );
  CORBA::Long gearIndex = m_vToolBar->insertButton2( pix, TOOLBAR_GEAR_ID, SIGNAL(clicked()),
                                                     this, "slotNewWindow", true, 0L, -1 );
  m_vToolBar->alignItemRight( TOOLBAR_GEAR_ID, true );

  //all items of the views should be inserted between the second-last and the
  //last item.
  m_lToolBarViewStartIndex = gearIndex;
  
  if ( m_currentView )
    createViewToolBar( m_currentView );

  m_vLocationBar = factory->create( OpenPartsUI::ToolBarFactory::Transient );

  m_vLocationBar->setFullWidth( true );

  CORBA::WString_var text = Q2C( i18n("Location : ") );
  m_vLocationBar->insertTextLabel( text, -1, -1 );

  toolTip = Q2C( i18n("Current Location") );
  
  text = Q2C( QString::null );
  OpenPartsUI::WStrList items;
  items.length( 0 );
  if ( m_currentView )
  {
    items.length( 1 );
    items[ 0 ] = Q2C( m_currentView->locationBarURL() );
  }
  
  
  m_vLocationBar->insertCombo3( items, TOOLBAR_URL_ID, true, SIGNAL( activated(const QString &) ),
                                this, "slotURLEntered", true, toolTip, 70, -1,
				OpenPartsUI::AfterCurrent );

  m_vLocationBar->setComboAutoCompletion( TOOLBAR_URL_ID, true );
  m_vLocationBar->setItemAutoSized( TOOLBAR_URL_ID, true );

  m_vLocationBar->setComboMaxCount( TOOLBAR_URL_ID, 10 );

  setLocationBarCombo( m_lstLocationBarCombo );

  checkPrintingExtension();

  kdebug(0,1202,"KonqMainView::mappingCreateToolbar : done !");
  return true;
}

bool KonqMainView::mappingChildGotFocus( OpenParts::Part_ptr child )
{
  kdebug(0, 1202, "bool KonqMainView::mappingChildGotFocus( OpenParts::Part_ptr child )");
  setActiveView( child->id() );

  setItemEnabled( m_vMenuWindow, MWINDOW_SPLITVIEWHOR_ID, true );
  setItemEnabled( m_vMenuWindow, MWINDOW_SPLITVIEWVER_ID, true );
  setItemEnabled( m_vMenuWindow, MWINDOW_SPLITWINHOR_ID, true );
  setItemEnabled( m_vMenuWindow, MWINDOW_SPLITWINVER_ID, true );
  setItemEnabled( m_vMenuWindow, MWINDOW_REMOVEVIEW_ID, ( m_mapViews.count() > 1 ) );

  setItemEnabled( m_vMenuView, MVIEW_SHOWDOT_ID, true );
  setItemEnabled( m_vMenuView, MVIEW_SHOWHTML_ID, true );
  setItemEnabled( m_vMenuView, MVIEW_LARGEICONS_ID, true );
  setItemEnabled( m_vMenuView, MVIEW_SMALLICONS_ID, true );
  setItemEnabled( m_vMenuView, MVIEW_SMALLVICONS_ID, true );
  setItemEnabled( m_vMenuView, MVIEW_TREEVIEW_ID, true );
  setItemEnabled( m_vMenuView, MVIEW_RELOAD_ID, true );
  setItemEnabled( m_vMenuGo, MGO_HOME_ID, true );
  setItemEnabled( m_vMenuGo, MGO_CACHE_ID, true );
  setItemEnabled( m_vMenuGo, MGO_HISTORY_ID, true );
  setItemEnabled( m_vMenuGo, MGO_MIMETYPES_ID, true );
  setItemEnabled( m_vMenuGo, MGO_APPLICATIONS_ID, true );
  
  return true;
}

bool KonqMainView::mappingParentGotFocus( OpenParts::Part_ptr  )
{
  kdebug(0, 1202, "bool KonqMainView::mappingParentGotFocus( OpenParts::Part_ptr child )");
  
  KonqChildView *oldView = m_currentView;
  
  m_currentView = 0L;
  m_currentId = 0;
  
  // removing view-specific menu entries (view will probably be destroyed !)
  if ( oldView )
  {
    clearViewGUIElements( oldView );
    
    m_bViewMenuDirty = true;
    m_bEditMenuDirty = true;
    
    createViewMenu();
    createEditMenu();
    
    oldView->repaint();
  }

  // no more active view (even temporarily)
  setUpEnabled( "/", 0 );
  setItemEnabled( m_vMenuGo, MGO_BACK_ID, false );
  setItemEnabled( m_vMenuGo, MGO_FORWARD_ID, false );
  setItemEnabled( m_vMenuWindow, MWINDOW_SPLITVIEWHOR_ID, false );
  setItemEnabled( m_vMenuWindow, MWINDOW_SPLITVIEWVER_ID, false );
  setItemEnabled( m_vMenuWindow, MWINDOW_SPLITWINHOR_ID, false );
  setItemEnabled( m_vMenuWindow, MWINDOW_SPLITWINVER_ID, false );
  setItemEnabled( m_vMenuWindow, MWINDOW_REMOVEVIEW_ID, false );
  
  setItemEnabled( m_vMenuView, MVIEW_SHOWDOT_ID, false );
  setItemEnabled( m_vMenuView, MVIEW_SHOWHTML_ID, false );
  setItemEnabled( m_vMenuView, MVIEW_LARGEICONS_ID, false );
  setItemEnabled( m_vMenuView, MVIEW_SMALLICONS_ID, false );
  setItemEnabled( m_vMenuView, MVIEW_SMALLVICONS_ID, false );
  setItemEnabled( m_vMenuView, MVIEW_TREEVIEW_ID, false );
  setItemEnabled( m_vMenuView, MVIEW_RELOAD_ID, false );
  setItemEnabled( m_vMenuView, MVIEW_STOP_ID, false );
  setItemEnabled( m_vMenuGo, MGO_HOME_ID, false );
  setItemEnabled( m_vMenuGo, MGO_CACHE_ID, false );
  setItemEnabled( m_vMenuGo, MGO_HISTORY_ID, false );
  setItemEnabled( m_vMenuGo, MGO_MIMETYPES_ID, false );
  setItemEnabled( m_vMenuGo, MGO_APPLICATIONS_ID, false );

  if ( m_pProgressBar )
    m_pProgressBar->setValue( -1 );

  if ( !CORBA::is_nil( m_vStatusBar ) )
    m_vStatusBar->changeItem( 0L, STATUSBAR_SPEED_ID );

  return true;
}

bool KonqMainView::mappingOpenURL( Browser::EventOpenURL eventURL )
{
  openURL( m_currentId, eventURL );
  return true;
}

bool KonqMainView::mappingNewTransfer( Browser::EventNewTransfer transfer )
{
  KIOJob *job = new KIOJob;
  job->copy( transfer.source.in(), transfer.destination.in() );
  
  return true;
}

bool KonqMainView::mappingURLEntered( const CORBA::WChar *_url )
{
  QString url = C2Q( _url );

  // Exit if the user did not enter an URL
  if ( url.isEmpty() )
    return false;

  openURL( url.ascii() );
  
  m_vLocationBar->setCurrentComboItem( TOOLBAR_URL_ID, 0 );
  m_vLocationBar->changeComboItem( TOOLBAR_URL_ID, _url, 0 );
  return true;
}

void KonqMainView::setActiveView( OpenParts::Id id )
{
  kdebug(0, 1202, "KonqMainView::setActiveView( %d )", id);
  KonqChildView* previousView = m_currentView;
  // clean view-specific part of the view/edit menus
  if ( previousView != 0L )
    clearViewGUIElements( previousView );

  MapViews::Iterator it = m_mapViews.find( id );
  assert( it != m_mapViews.end() );
  
  m_currentView = it.data();
  m_currentId = id;
  
  assert( m_currentView );
  
  setUpEnabled( m_currentView->url(), id );
  setItemEnabled( m_vMenuGo, MGO_BACK_ID, m_currentView->canGoBack() );
  setItemEnabled( m_vMenuGo, MGO_FORWARD_ID, m_currentView->canGoForward() );
  setItemEnabled( m_vMenuView, MVIEW_STOP_ID, m_currentView->isLoading() );

  setItemEnabled( m_vMenuFile, MFILE_PRINT_ID, m_currentView->view()->supportsInterface( "IDL:Browser/PrintingExtension:1.0" ) );
  checkEditExtension();

  if ( !CORBA::is_nil( m_vLocationBar ) )
  {
    CORBA::WString_var text = Q2C( m_currentView->locationBarURL() );
    m_vLocationBar->changeComboItem( TOOLBAR_URL_ID, text, 0 );
  }    

  if ( m_pProgressBar )
    m_pProgressBar->setValue( m_currentView->progress() );

  if ( !CORBA::is_nil( m_vStatusBar ) )
    m_vStatusBar->changeItem( 0L, STATUSBAR_SPEED_ID );

  m_bEditMenuDirty = true;
  m_bViewMenuDirty = true;
  createViewToolBar( m_currentView );
  if ( isVisible() )
  {
    if (previousView != 0L) // might be 0L e.g. if we just removed the current view
      previousView->repaint();
    m_currentView->repaint();
  }
}

Browser::View_ptr KonqMainView::activeView()
{
  if ( m_currentView )
    //KonqChildView::view() does *not* call _duplicate, so we have to do it here (Simon)
    return Browser::View::_duplicate( m_currentView->view() );
  else
    return Browser::View::_nil();
}

OpenParts::Id KonqMainView::activeViewId()
{
  return m_currentId;
}

void KonqMainView::selectViewByNumber( CORBA::Long number )
{
  OpenParts::Id viewId = m_pViewManager->viewIdByNumber( number );
  if ( viewId != 0 )
    m_vMainWindow->setActivePart( viewId );
}

void KonqMainView::openURL( OpenParts::Id id, const Browser::URLRequest &_urlreq )
{
  KonqChildView *view = 0L;
  MapViews::ConstIterator it = m_mapViews.find( id );
  if ( it != m_mapViews.end() )
    view = it.data();

  openURL( _urlreq.url.in(), (bool)_urlreq.reload, (int)_urlreq.xOffset,
          (int)_urlreq.yOffset, view );
}

void KonqMainView::openURL( const char * _url, bool reload, int xOffset, int yOffset, KonqChildView *_view )
{
  /////////// First, modify the URL if necessary (adding protocol, ...) //////
  QString url = _url;

  // Root directory?
  if ( url[0] == '/' )
  {
    KURL::encode( url );
  }
  // Home directory?
  else if ( url.find( QRegExp( "^~.*" ) ) == 0 )
  {
    QString user;
    QString path;
    int index;
    
    index = url.find( "/" );
    if ( index != -1 )
    {
      user = url.mid( 1, index-1 );
      path = url.mid( index+1 );
    }
    else
      user = url.mid( 1 );
      
    if ( user.isEmpty() )
      user = QDir::homeDirPath();
    else
    {
      struct passwd *pwe = getpwnam( user.latin1() );
      if ( !pwe )
      {
	KMessageBox::sorry( this, i18n( "User %1 doesn't exist" ).arg( user ));
	return;
      }
      user = QString::fromLatin1( pwe->pw_dir );
    }
    
    KURL u( user + '/' + path );
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
    KMessageBox::error(0, tmp);
    return;
  }

  KonqChildView *view = _view;
  if ( !view )
    view = m_currentView;

  //temporarily commented out. Michael.
  //if ( view && m_pViewManager->isLinked( view ) )
  //  view = m_pViewManager->readLink( view );

  if ( view )
  {
    if ( view->id() == m_currentId )
      //will do all the stuff below plus GUI stuff
      slotStop();
    else
    {
      view->stop();
      if ( view->kfmRun() )
        delete view->kfmRun();
    }	
  }

  KfmRun *run = new KfmRun( this, view, url, 0, false, true );
  
  if ( view )
  {
    setLocationBarURL( view->id(), url );
    view->setKfmRun( run );
    view->setMiscURLData( reload, xOffset, yOffset );
  }
}

void KonqMainView::setStatusBarText( const CORBA::WChar *_text )
{
  if ( !CORBA::is_nil( m_vStatusBar ) )
    m_vStatusBar->changeItem( _text, STATUSBAR_MSG_ID );
}

void KonqMainView::setLocationBarURL( OpenParts::Id id, const char *_url )
{
  MapViews::Iterator it = m_mapViews.find( id );
  assert( it != m_mapViews.end() );

  CORBA::WString_var wurl = Q2C( QString( _url ) );
    
  it.data()->setLocationBarURL( _url );
  
  if ( ( id == m_currentId ) && (!CORBA::is_nil( m_vLocationBar ) ) )
    m_vLocationBar->changeComboItem( TOOLBAR_URL_ID, wurl, 0 );
}

void KonqMainView::createNewWindow( const char *url )
{
  KonqMainWindow *m_pShell = new KonqMainWindow( url );
  m_pShell->show();
}

void KonqMainView::popupMenu( const QPoint &_global, KFileItemList _items )
{
  QString url = m_currentView->url();
  bool bBack = m_currentView->canGoBack();
  bool bForward = m_currentView->canGoForward();
  bool bMenuBar = !m_vMenuBar->isVisible();

  KonqPopupMenu * popupMenu;
  
  if ( m_currentView->view()->supportsInterface( "IDL:Browser/EditExtension:1.0" ) )
    popupMenu = new KonqPopupMenu( _items,
                                   url,
                                   bBack,
				   bForward,
				   bMenuBar,
				   false,
				   m_vMenuEdit->isItemEnabled( MEDIT_COPY_ID ),
				   m_vMenuEdit->isItemEnabled( MEDIT_PASTE_ID ),
				   m_vMenuEdit->isItemEnabled( MEDIT_DELETE_ID ) );
  else
    popupMenu = new KonqPopupMenu( _items,
                                   url,
                                   bBack,
				   bForward,
				   bMenuBar );

  //kdebug(0, 1202, "exec()");
  int iSelected = popupMenu->exec( _global );
  //kdebug(0, 1202, "deleting popupMenu object");
  delete popupMenu;
  /* Test for konqueror-specific entries. A normal signal/slot mechanism doesn't work here,
     because those slots are virtual. */
  switch (iSelected) {
    case KPOPUPMENU_UP_ID : slotUp(); break;
    case KPOPUPMENU_BACK_ID : slotBack(); break;
    case KPOPUPMENU_FORWARD_ID : slotForward(); break;
    case KPOPUPMENU_SHOWMENUBAR_ID : slotShowMenubar(); break;
    case KPOPUPMENU_COPY_ID : slotCopy(); break;
    case KPOPUPMENU_PASTE_ID : slotPaste(); break;
    case KPOPUPMENU_TRASH_ID : slotTrash(); break;
    case KPOPUPMENU_DELETE_ID : slotDelete(); break;
  }
}

bool KonqMainView::openView( const QString &serviceType, const QString &url, KonqChildView *childView )
{
  QString indexFile;
  KURL u( url );
/*
  if ( !m_sInitialURL.isEmpty() )
  {
    Browser::View_var vView;
    QStringList serviceTypes;

    if ( ( serviceType == "inode/directory" ) &&
         ( KonqPropsView::defaultProps()->isHTMLAllowed() ) &&
         ( u.isLocalFile() ) &&
	 ( ( indexFile = findIndexFile( u.path() ) ) != QString::null ) )
    {
      vView = KonqFactory::createView( "text/html", serviceTypes, this );
      m_sInitialURL = indexFile.prepend( "file:" );
    }
    else if ( CORBA::is_nil( ( vView = KonqFactory::createView( serviceType, serviceTypes, this ) ) ) )
      return false;
      
    m_pViewManager->splitView( Qt::Horizontal, vView, serviceTypes );

    MapViews::Iterator it = m_mapViews.find( vView->id() );
    it.data()->openURL( m_sInitialURL );
  
    m_vMainWindow->setActivePart( vView->id() );
    
    m_sInitialURL = QString::null;
    
    return true;
  }
*/  
  //first check whether the current view can display this type directly, then
  //try to change the view mode. if this fails, too, then Konqueror cannot
  //display the data addressed by the URL
  if ( childView->supportsServiceType( serviceType ) )
  {
    if ( ( serviceType == "inode/directory" ) &&
         ( childView->allowHTML() ) &&
         ( u.isLocalFile() ) &&
	 ( ( indexFile = findIndexFile( u.path() ) ) != QString::null ) )
      childView->changeViewMode( "text/html", indexFile.prepend( "file:" ) );
    else
    {
      childView->makeHistory( false );
      childView->openURL( url, true );
    }
      
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

KonqChildView *KonqMainView::childView( OpenParts::Id id )
{
  MapViews::ConstIterator it = m_mapViews.find( id );
  if ( it != m_mapViews.end() )
    return it.data();
  else
    return 0L;
}

void KonqMainView::insertChildView( KonqChildView *view )
{
  m_mapViews.insert( view->id(), view );

  setItemEnabled( m_vMenuWindow, MWINDOW_REMOVEVIEW_ID, 
                 (m_mapViews.count() > 1) );
}

void KonqMainView::removeChildView( OpenParts::Id id )
{
  MapViews::Iterator it = m_mapViews.find( id );
  m_mapViews.remove( it );
  
  setItemEnabled( m_vMenuWindow, MWINDOW_REMOVEVIEW_ID, 
                 (m_mapViews.count() > 1) );
}

void KonqMainView::openBookmarkURL( const char *url )
{
  kdebug(0,1202,"KonqMainView::openBookmarkURL(%s)",url);
  openURL( url );
}

QString KonqMainView::currentTitle()
{
  CORBA::WString_var t = m_vMainWindow->partCaption( m_currentId );
  QString title = C2Q( t );
  if ( title.left( 11 ) == "Konqueror: " )
    return title.mid( 11 );
  return title;
}
 
QString KonqMainView::currentURL()
{
  return m_currentView->url();
}

void KonqMainView::saveProperties( KConfig *config )
{
  m_pViewManager->saveViewProfile( *config );
  
  config->writeEntry( "ToolBarCombo", locationBarCombo() );
}

void KonqMainView::readProperties( KConfig *config )
{

  if ( m_mapViews.contains( m_vMainWindow->activePartId() ) )
    m_vMainWindow->setActivePart( id() );

  m_pViewManager->loadViewProfile( *config );

  setLocationBarCombo( config->readListEntry( "ToolBarCombo" ) );
}

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
    config->setGroup( "Misc Defaults" );
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
    openURL( u.ascii() );
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
  Browser::View_ptr view = m_currentView->view();
  
  if ( view->supportsInterface( "IDL:Browser/PrintingExtension:1.0" ) )
  {
    CORBA::Object_var obj = view->getInterface( "IDL:Browser/PrintingExtension:1.0" );
    Browser::PrintingExtension_var printExt = Browser::PrintingExtension::_narrow( obj );
    printExt->print();
  }
}

void KonqMainView::slotCopy()
{
  CORBA::Object_var obj = m_currentView->view()->getInterface( "IDL:Browser/EditExtension:1.0" );
  Browser::EditExtension_var editExtension = Browser::EditExtension::_narrow( obj );
  editExtension->copySelection();
}

void KonqMainView::slotPaste()
{
  CORBA::Object_var obj = m_currentView->view()->getInterface( "IDL:Browser/EditExtension:1.0" );
  Browser::EditExtension_var editExtension = Browser::EditExtension::_narrow( obj );
  editExtension->pasteSelection();
}

void KonqMainView::slotTrash()
{
  CORBA::Object_var obj = m_currentView->view()->getInterface( "IDL:Browser/EditExtension:1.0" );
  Browser::EditExtension_var editExtension = Browser::EditExtension::_narrow( obj );
  editExtension->moveSelection( UserPaths::trashPath().utf8() );
}

void KonqMainView::slotDelete()
{
  CORBA::Object_var obj = m_currentView->view()->getInterface( "IDL:Browser/EditExtension:1.0" );
  Browser::EditExtension_var editExtension = Browser::EditExtension::_narrow( obj );
  editExtension->moveSelection( 0L );
}

void KonqMainView::slotSplitViewHorizontal()
{
  m_pViewManager->splitView( Qt::Horizontal );
}
 
void KonqMainView::slotSplitViewVertical()
{
  m_pViewManager->splitView( Qt::Vertical );
}

void KonqMainView::slotSplitWindowHorizontal()
{
  //TODO Just want to have it working at all! Multiple inheritence is
  //evil!! But it saves a lot of code:) Michael.
}
 
void KonqMainView::slotSplitWindowVertical()
{
  //TODO
}

void KonqMainView::slotRemoveView()
{
  if ( m_mapViews.count() == 1 )
    return;

  KonqChildView *nextView = m_pViewManager->chooseNextView( m_currentView );
  KonqChildView *prevView = m_currentView;

  m_vMainWindow->setActivePart( nextView->id() );

  m_pViewManager->removeView( prevView );
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
    openURL( m_currentView->url() );
  }
  else if ( !b && m_currentView->supportsServiceType( "text/html" ) )
  {
    KURL u( m_currentView->url() );
    m_currentView->lockHistory();
    openURL( u.directory() );
  }
}

void KonqMainView::slotLargeIcons()
{
  if ( !m_currentView->view()->supportsInterface( "IDL:Konqueror/KfmIconView:1.0" ) )
  {
    m_currentView->lockHistory();
    m_currentView->changeViewMode( "inode/directory", QString::null, false,
                                   Konqueror::LargeIcons );
  }
  else
  {
    Konqueror::KfmIconView_var iv = Konqueror::KfmIconView::_narrow( m_currentView->view() );
    iv->setViewMode( Konqueror::LargeIcons );
  }    

  m_vMenuView->setItemChecked( MVIEW_LARGEICONS_ID, true );
  m_vMenuView->setItemChecked( MVIEW_SMALLICONS_ID, false );
  m_vMenuView->setItemChecked( MVIEW_SMALLVICONS_ID, false );
  m_vMenuView->setItemChecked( MVIEW_TREEVIEW_ID, false );
}

void KonqMainView::slotSmallIcons()
{
  if ( !m_currentView->view()->supportsInterface( "IDL:Konqueror/KfmIconView:1.0" ) )
  {
    m_currentView->lockHistory();
    m_currentView->changeViewMode( "inode/directory", QString::null, false,
                                   Konqueror::SmallIcons );
  }
  else
  {
    Konqueror::KfmIconView_var iv = Konqueror::KfmIconView::_narrow( m_currentView->view() );
    iv->setViewMode( Konqueror::SmallIcons );
  }
  
  m_vMenuView->setItemChecked( MVIEW_LARGEICONS_ID, false );
  m_vMenuView->setItemChecked( MVIEW_SMALLICONS_ID, true );
  m_vMenuView->setItemChecked( MVIEW_SMALLVICONS_ID, false );
  m_vMenuView->setItemChecked( MVIEW_TREEVIEW_ID, false );
}

void KonqMainView::slotSmallVerticalIcons()
{
  if ( !m_currentView->view()->supportsInterface( "IDL:Konqueror/KfmIconView:1.0" ) )
  {
    m_currentView->lockHistory();
    m_currentView->changeViewMode( "inode/directory", QString::null, false,
                                   Konqueror::SmallVerticalIcons );
  }
  else
  {
    Konqueror::KfmIconView_var iv = Konqueror::KfmIconView::_narrow( m_currentView->view() );
    iv->setViewMode( Konqueror::SmallVerticalIcons );
  }    

  m_vMenuView->setItemChecked( MVIEW_LARGEICONS_ID, false );
  m_vMenuView->setItemChecked( MVIEW_SMALLICONS_ID, false );
  m_vMenuView->setItemChecked( MVIEW_SMALLVICONS_ID, true );
  m_vMenuView->setItemChecked( MVIEW_TREEVIEW_ID, false );
}

void KonqMainView::slotTreeView()
{
  if ( !m_currentView->view()->supportsInterface( "IDL:Konqueror/KfmTreeView:1.0" ) )
  {
    m_currentView->lockHistory();
    m_currentView->changeViewMode( "inode/directory", QString::null, false,
                                   Konqueror::TreeView );
  }
  else return;

  m_vMenuView->setItemChecked( MVIEW_LARGEICONS_ID, false );
  m_vMenuView->setItemChecked( MVIEW_SMALLICONS_ID, false );
  m_vMenuView->setItemChecked( MVIEW_SMALLVICONS_ID, false );
  m_vMenuView->setItemChecked( MVIEW_TREEVIEW_ID, true );
}

void KonqMainView::slotReload()
{
  m_currentView->reload();
}

void KonqMainView::slotStop()
{
  if ( m_currentView && m_currentView->isLoading() )
  {
    m_currentView->stop();
    if ( m_currentView->kfmRun() )
      delete m_currentView->kfmRun();
    m_currentView->setKfmRun( 0L );
    slotStopAnimation();
    if ( m_pProgressBar ) 
      m_pProgressBar->setValue( -1 );
      
    if ( !CORBA::is_nil( m_vStatusBar ) )
    {
      m_vStatusBar->changeItem( 0L, STATUSBAR_SPEED_ID );
      m_vStatusBar->changeItem( 0L, STATUSBAR_MSG_ID );
    }      
  }    
}

void KonqMainView::slotStop2()
{
  slotStop();
}

void KonqMainView::slotUp()
{
  kdebug(0, 1202, "KonqMainView::slotUp()");
  QString url = m_currentView->url();
  KURL u( url );
  u.cd(".."); // KURL does it for us
  
  openURL( u.url() ); // not m_currentView->openURL since the view mode might be different
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
  openURL("~"); // might need a view-mode change...
}

void KonqMainView::slotShowCache()
{
  QString file = KIOCache::storeIndex();
  if ( file.isEmpty() )
  {
    KMessageBox::sorry( 0L, i18n( "Could not write index file" ));
    return;
  }

  QString f = file;
  KURL::encode( f );
  f.prepend( "file:" );
  openURL( f );
}

void KonqMainView::slotShowHistory()
{
  // TODO
}

void KonqMainView::slotEditMimeTypes()
{
    openURL( KGlobal::dirs()->getSaveLocation("mime").prepend( "file:" ) );
}

void KonqMainView::slotEditApplications()
{
    openURL( KGlobal::dirs()->getSaveLocation("apps").prepend( "file:" ) );
}

void KonqMainView::slotShowMenubar()
{
  m_vMenuBar->enable( OpenPartsUI::Toggle );
  m_vMenuOptions->setItemChecked( MOPTIONS_SHOWMENUBAR_ID, m_vMenuBar->isVisible() );
}

void KonqMainView::slotShowStatusbar()
{
  if ( CORBA::is_nil( m_vStatusBar ) )
    return;

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
  if (fork() == 0) {
    // execute 'kcmkonq' 
    execl(locate("exe", "/kcmkonq"), 0);
    warning("Error launching kcmkonq !");
    exit(1);
  }             
}

void KonqMainView::slotConfigureBrowser()
{
  if (fork() == 0) {
    // execute 'kcmkio' 
    execl(locate("exe", "kcmkio"), 0);
    warning("Error launching kcmkio !");
    exit(1);
  }                          
}

void KonqMainView::slotConfigureKeys()
{
  KKeyDialog::configureKeys( m_pAccel );
}

void KonqMainView::slotReloadPlugins()
{
  KonqPlugins::reload();

  KOM::Base::EventFilterSeq_var filters = describeEventFilters();

  for ( CORBA::ULong k = 0; k < filters->length(); ++k )
    uninstallFilter( filters[ k ].receiver );

  KonqPlugins::installKOMPlugins( this );

  Browser::View_ptr pView;
  
  MapViews::ConstIterator it = m_mapViews.begin();
  MapViews::ConstIterator end = m_mapViews.end();
  for (; it != end; ++it )
  {
    pView = (*it)->view();

    filters = pView->describeEventFilters();
    for ( CORBA::ULong k = 0; k < filters->length(); ++k )
      pView->uninstallFilter( filters[ k ].receiver );

    KonqPlugins::installKOMPlugins( pView );
  }
}

void KonqMainView::slotConfigurePlugins()
{
  KonqPlugins::configure( this );
  
  QListIterator<KonqMainView> it( *s_lstWindows );
  for (; it.current(); ++it )
    it.current()->slotReloadPlugins();
}

void KonqMainView::slotSaveDefaultProfile()
{
  KConfig *config = kapp->getConfig();
  config->setGroup( "Default View Profile" );
  m_pViewManager->saveViewProfile( *config );
}

void KonqMainView::slotProfileDlg()
{
  KonqProfileDlg dlg( m_pViewManager, this );
  dlg.exec();
  fillProfileMenu();
/*
  KLineEditDlg *dlg = new KLineEditDlg( i18n( "Enter Name for Profile" ),
                                        QString::null, this, false );
  
  dlg->setFocusPolicy( QWidget::StrongFocus );
  
  if ( dlg->exec() && !dlg->text().isEmpty() )
  {
    QString fileName = locateLocal( "data", 
                       QString::fromLatin1( "konqueror/profiles/" ) + 
		       dlg->text() );
    
    if ( QFile::exists( fileName ) )
    {
      QFile f( fileName );
      f.remove();
    }
    
    KConfig cfg( fileName );
    cfg.setGroup( "Profile" );
    m_pViewManager->saveViewProfile( cfg );
    
    if ( !CORBA::is_nil( m_vMenuOptionsProfiles ) )
      fillProfileMenu();
  }
*/  
}

void KonqMainView::slotViewProfileActivated( CORBA::Long id )
{
  CORBA::WString_var text = m_vMenuWindowProfiles->text( id );
  QString name = QString::fromLatin1( "konqueror/profiles/" ) + C2Q( text );
  
  QString fileName = locate( "data", name );
  
  m_vMainWindow->setActivePart( this->id() );
  
  KConfig cfg( fileName, true );
  cfg.setGroup( "Profile" );
  m_pViewManager->loadViewProfile( cfg );
}

void KonqMainView::slotHelpContents()
{
  kapp->invokeHTMLHelp( "konqueror/index.html", "" );
}

void KonqMainView::slotHelpAbout()
{
  KMessageBox::about( 0, i18n(
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

void KonqMainView::slotURLEntered( const CORBA::WChar *_url )
{
  EMIT_EVENT_WSTRING( this, Konqueror::eventURLEntered, _url );
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

void KonqMainView::slotMenuEditAboutToShow()
{
  createEditMenu();
}

void KonqMainView::slotMenuViewAboutToShow()
{
  createViewMenu();
}

void KonqMainView::fillHistoryPopup( OpenPartsUI::Menu_ptr menu, const QStringList &urls )
{
  menu->clear();
  
  if ( !m_currentView )
    return;

  CORBA::WString_var text;
  OpenPartsUI::Pixmap_var pix;
  QStringList::ConstIterator it = urls.begin();
  QStringList::ConstIterator end = urls.end();

  for (; it != end; ++it )
  {
    KURL u( *it ) ;
    menu->insertItem11( ( pix = OPUIUtils::convertPixmap( *KPixmapCache::pixmapForURL( u, 0, u.isLocalFile(), true ) ) ), 
                        ( text = Q2C( *it ) ), -1, -1 );
  }			
}

void KonqMainView::slotHistoryBackwardAboutToShow()
{
  fillHistoryPopup( m_vHistoryBackPopupMenu, m_currentView->backHistoryURLs() );
}

void KonqMainView::slotHistoryForwardAboutToShow()
{
  fillHistoryPopup( m_vHistoryForwardPopupMenu, m_currentView->forwardHistoryURLs() );
}

void KonqMainView::slotHistoryBackwardActivated( CORBA::Long id )
{
  if ( m_currentView )
    m_currentView->goBack( m_vHistoryBackPopupMenu->indexOf( id ) + 1 );
}

void KonqMainView::slotHistoryForwardActivated( CORBA::Long id )
{
  if ( m_currentView )
    m_currentView->goForward( m_vHistoryForwardPopupMenu->indexOf( id ) + 1 );
}

void KonqMainView::slotBookmarkSelected( CORBA::Long id )
{
  if ( m_pBookmarkMenu )
    m_pBookmarkMenu->slotBookmarkSelected( id );
}

void KonqMainView::slotBookmarkHighlighted( CORBA::Long id )
{
  if ( m_pBookmarkMenu && !CORBA::is_nil( m_vStatusBar ) )
  {
    CORBA::WString_var wurl;
    KBookmark *bm = KBookmarkManager::self()->findBookmark( (int)id );
    if ( bm )
      wurl = Q2C( bm->url() );

    m_vStatusBar->changeItem( wurl, STATUSBAR_MSG_ID );
  }
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
  
  (*it)->setLoading( true );
  
  if ( id == m_currentId )
    slotStartAnimation();

  (*it)->makeHistory( true );
  (*it)->setProgress( -1 );
  
  if ( id == m_currentId )
  {
    setUpEnabled( url, id );
    setItemEnabled( m_vMenuGo, MGO_BACK_ID, m_currentView->canGoBack() );
    setItemEnabled( m_vMenuGo, MGO_FORWARD_ID, m_currentView->canGoForward() );
    if ( m_pProgressBar )
      m_pProgressBar->setValue( -1 );

    if ( !CORBA::is_nil( m_vStatusBar ) )
      m_vStatusBar->changeItem( 0L, STATUSBAR_SPEED_ID );
  }
}

void KonqMainView::slotURLCompleted( OpenParts::Id id )
{
  kdebug(0, 1202, "void KonqMainView::slotURLCompleted( OpenParts::Id id )");

  MapViews::Iterator it = m_mapViews.find( id );
  
  assert( it != m_mapViews.end() );

  (*it)->setLoading( false );

  if ( id == m_currentId )
    slotStopAnimation();

  (*it)->setProgress( -1 );

  if ( id == m_currentId )
  {
    setItemEnabled( m_vMenuGo, MGO_BACK_ID, m_currentView->canGoBack() );
    setItemEnabled( m_vMenuGo, MGO_FORWARD_ID, m_currentView->canGoForward() );
    
    if ( m_pProgressBar ) 
      m_pProgressBar->setValue( -1 );
      
    if ( !CORBA::is_nil( m_vStatusBar ) )
      m_vStatusBar->changeItem( 0L, STATUSBAR_SPEED_ID );
  }
}

void KonqMainView::slotUpAboutToShow()
{
  m_vUpPopupMenu->clear();

  CORBA::WString_var text;
  
  KURL u( m_currentView->url() );
  u.cd( ".." );
  while ( u.hasPath() )
  {
    m_vUpPopupMenu->insertItem7( ( text = Q2C( u.decodedURL() ) ), -1, -1 );
    
    if ( u.path() == "/" )
      break;
      
    u.cd( ".." );
  }
}

void KonqMainView::slotUpActivated( CORBA::Long id )
{
  CORBA::WString_var text = m_vUpPopupMenu->text( id );
  QString url = C2Q( text );
  openURL( url.ascii() );
}

void KonqMainView::slotLoadingProgress( OpenParts::Id id, CORBA::Long percent )
{
  if ( id == m_currentId && m_pProgressBar && m_currentView->isLoading() )
    m_pProgressBar->setValue( (int)percent );
  
  MapViews::Iterator it = m_mapViews.find( id );
  
  (*it)->setProgress( (int)percent );
}

void KonqMainView::slotSpeedProgress( OpenParts::Id id, CORBA::Long bytesPerSecond )
{
  if ( id != m_currentId || CORBA::is_nil( m_vStatusBar ) || !m_currentView->isLoading() )
    return;

  QString sizeStr;
  
  if ( bytesPerSecond > 0 )
    sizeStr = KIOJob::convertSize( (int)bytesPerSecond ) + QString::fromLatin1( "/s" );
  else
    sizeStr = i18n( "stalled" );
    
  CORBA::WString_var wSizeStr = Q2C( sizeStr );

  m_vStatusBar->changeItem( wSizeStr, STATUSBAR_SPEED_ID );
}

void KonqMainView::slotSelectionChanged()
{
  checkEditExtension();
}

void KonqMainView::slotItemHighlighted( CORBA::Long id )
{
  if ( CORBA::is_nil( m_vStatusBar ) )
    return;

  QString msg;
  switch ( id )
  {
    case MGO_BACK_ID: msg = i18n( "Return to the previous page in History list" ); break;
    case MGO_FORWARD_ID: msg = i18n( "Go to the next page in History list" ); break;
    case MVIEW_RELOAD_ID: msg = i18n( "Reload the current page" ); break;
    case MGO_HOME_ID: msg = i18n( "Go to the home directory" ); break;
    case MFILE_PRINT_ID: msg = i18n( "Print the current page" ); break;
    case MVIEW_STOP_ID: msg = i18n( "Stop loading the current page" ); break;
    case MEDIT_PASTE_ID: msg = i18n( "Paste the clipboard content to the current view" ); break;
    case MEDIT_COPY_ID: msg = i18n( "Copy to selected content to the clipboard" ); break;
    case MEDIT_DELETE_ID: msg = i18n( "Delete the selected content" ); break;
    case MEDIT_TRASH_ID: msg = i18n( "Move the selected content to the system trash" ); break;
    case MHELP_CONTENTS_ID: msg = i18n( "Display help on using Konqueror" ); break;
    default: msg = QString::null; break;
  }
  
  CORBA::WString_var wmsg = Q2C( msg );
  m_vStatusBar->changeItem( wmsg, STATUSBAR_MSG_ID );
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
  if ( !CORBA::is_nil( m_vStatusBar ) )
    m_vStatusBar->changeItem( msg, STATUSBAR_MSG_ID );
}

void KonqMainView::slotIdChanged( KonqChildView * childView, OpenParts::Id oldId, OpenParts::Id newId )
{
  m_mapViews.remove( oldId );
  m_mapViews.insert( newId, childView );
  if ( oldId == m_currentId )
    m_currentId = newId;
}

void KonqMainView::checkEditExtension()
{
  CORBA::Boolean bCopy = false;
  CORBA::Boolean bPaste = false;
  CORBA::Boolean bMove = false;
  
  if ( m_currentView &&
       m_currentView->view()->supportsInterface( "IDL:Browser/EditExtension:1.0" ) )
  {
    CORBA::Object_var obj = m_currentView->view()->getInterface( "IDL:Browser/EditExtension:1.0" );
    Browser::EditExtension_var editExtension = Browser::EditExtension::_narrow( obj );
    editExtension->can( bCopy, bPaste, bMove );
  }
  
  setItemEnabled( m_vMenuEdit, MEDIT_COPY_ID, bCopy );
  setItemEnabled( m_vMenuEdit, MEDIT_PASTE_ID, bPaste );
  setItemEnabled( m_vMenuEdit, MEDIT_TRASH_ID, bMove );
  setItemEnabled( m_vMenuEdit, MEDIT_DELETE_ID, bMove );
}

void KonqMainView::resizeEvent( QResizeEvent * )
{
  if ( m_pViewManager )
    m_pViewManager->doGeometry( width(), height() );
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
//  else
//    this->resize(m_Props->m_width,m_Props->m_height);
    
  KConfig *config = kapp->getConfig();
  config->setGroup( "Settings" );
  m_lstLocationBarCombo = config->readListEntry( "ToolBarCombo" );
}

void KonqMainView::initGui()
{
  KConfig *config = kapp->getConfig();
  if ( config->hasGroup( "Default View Profile" ) && m_sInitialURL.isEmpty() )
  {
    config->setGroup( "Default View Profile" );
    m_pViewManager->loadViewProfile( *config );
  }
  else
  {
    //dummy default view
    QStringList serviceTypes;
    //this *may* not fail
    Browser::View_var vView = KonqFactory::createView( "text/plain", serviceTypes, this );
    m_pViewManager->splitView( Qt::Horizontal, vView, serviceTypes );
    
    m_vMainWindow->setActivePart( vView->id() );
    
    if ( m_sInitialURL.isEmpty() )
      m_sInitialURL = QDir::homeDirPath().prepend( "file:" );
      
    openURL( m_sInitialURL );
  }

  if ( s_lstAnimatedLogo->count() == 0 )
  {
    s_lstAnimatedLogo->setAutoDelete( true );
    for ( int i = 1; i < 9; i++ )
    {
      QString n;
      n.sprintf( "kde%i.png", i );
      s_lstAnimatedLogo->append( OPUIUtils::convertPixmap( *KPixmapCache::toolbarPixmap( n ) ) );
    }
  }			

  m_animatedLogoCounter = 0;
  QObject::connect( &m_animatedLogoTimer, SIGNAL( timeout() ), this, SLOT( slotAnimatedLogoTimeout() ) );
}

void KonqMainView::checkPrintingExtension()
{
  if ( m_currentView )
    setItemEnabled( m_vMenuFile, MFILE_PRINT_ID, m_currentView->view()->supportsInterface( "IDL:Browser/PrintingExtension:1.0" ) );
}

void KonqMainView::setItemEnabled( OpenPartsUI::Menu_ptr menu, int id, bool enable )
{
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

void KonqMainView::clearViewGUIElements( KonqChildView *childView )
{
  Browser::View_ptr view = childView->view();
  EMIT_EVENT( view, Browser::View::eventFillMenuEdit, 0L );
  EMIT_EVENT( view, Browser::View::eventFillMenuView, 0L );
    
  Browser::View::EventFillToolBar ev;
  ev.create = (CORBA::Boolean)false;
  ev.toolBar = OpenPartsUI::ToolBar::_duplicate( m_vToolBar );
  EMIT_EVENT( view, Browser::View::eventFillToolBar, ev );
}

void KonqMainView::createViewToolBar( KonqChildView *childView )
{
  if ( CORBA::is_nil( m_vToolBar ) )
    return;

  Browser::View::EventFillToolBar ev;
  ev.create = (CORBA::Boolean)true;
  ev.startIndex = m_lToolBarViewStartIndex;
  ev.toolBar = OpenPartsUI::ToolBar::_duplicate( m_vToolBar );
  EMIT_EVENT( childView->view(), Browser::View::eventFillToolBar, ev );
}

void KonqMainView::createViewMenu()
{
  if ( !CORBA::is_nil( m_vMenuView ) && m_bViewMenuDirty )
  {
    m_vMenuView->clear();
  
    CORBA::WString_var text;
    OpenPartsUI::Pixmap_var pix;
    
    m_vMenuView->setCheckable( true );
    
    text = Q2C( i18n("&Large Icons") );
    m_vMenuView->insertItem4( text, this, "slotLargeIcons" , 0, MVIEW_LARGEICONS_ID, -1 );
    text = Q2C( i18n("&Small Icons") );
    m_vMenuView->insertItem4( text, this, "slotSmallIcons" , 0, MVIEW_SMALLICONS_ID, -1 );
    text = Q2C( i18n("Small Vertical Icons" ) );
    m_vMenuView->insertItem4( text, this, "slotSmallVerticalIcons", 0, MVIEW_SMALLVICONS_ID, -1 );
    text = Q2C( i18n("&Tree View") );
    m_vMenuView->insertItem4( text, this, "slotTreeView" , 0, MVIEW_TREEVIEW_ID, -1 );

    m_vMenuView->insertSeparator( -1 );

    text = Q2C( i18n("&Use HTML") );
    m_vMenuView->insertItem4( text, this, "slotShowHTML" , 0, MVIEW_SHOWHTML_ID, -1 );
    
    m_vMenuView->insertSeparator( -1 );

    if ( m_currentView )
    {
      Browser::View_ptr pView = m_currentView->view();
      if ( pView->supportsInterface( "IDL:Konqueror/KfmIconView:1.0" ) )
      {
        Konqueror::KfmIconView_var iconView = Konqueror::KfmIconView::_narrow( pView );
	Konqueror::DirectoryDisplayMode dirMode = iconView->viewMode();
	
	switch ( dirMode )
	{
	  case Konqueror::LargeIcons:
	    m_vMenuView->setItemChecked( MVIEW_LARGEICONS_ID, true );
	    break;
	  case Konqueror::SmallIcons:
	    m_vMenuView->setItemChecked( MVIEW_SMALLICONS_ID, true );
	    break;
	  case Konqueror::SmallVerticalIcons:
	    m_vMenuView->setItemChecked( MVIEW_SMALLVICONS_ID, true );
	    break;
	}
      }
      else if ( pView->supportsInterface( "IDL:Konqueror/KfmTreeView:1.0" ) )
        m_vMenuView->setItemChecked( MVIEW_TREEVIEW_ID, true );
    }

    pix = OPUIUtils::convertPixmap( *KPixmapCache::toolbarPixmap( "reload.png" ) );
    text = Q2C( i18n("&Reload Document") );
    m_vMenuView->insertItem6( pix, text, this, "slotReload" , Key_F5, MVIEW_RELOAD_ID, -1 );
    text = Q2C( i18n("Sto&p loading") );
    m_vMenuView->insertItem4( text, this, "slotStop" , 0, MVIEW_STOP_ID, -1 );

    setItemEnabled( m_vMenuWindow, MWINDOW_REMOVEVIEW_ID, 
	(m_mapViews.count() > 1) );

    if ( m_currentView )	
      setItemEnabled( m_vMenuView, MVIEW_STOP_ID, m_currentView->isLoading() );
    else
      setItemEnabled( m_vMenuView, MVIEW_STOP_ID, false );

    if ( m_currentView )
    {
      m_vMenuView->setItemChecked( MVIEW_SHOWHTML_ID, (m_currentView->allowHTML() ) );
      
      EMIT_EVENT( m_currentView->view(), Browser::View::eventFillMenuView, m_vMenuView );
    }

    m_bViewMenuDirty = false;
  }
}

void KonqMainView::createEditMenu()
{
  if ( !CORBA::is_nil( m_vMenuEdit ) && m_bEditMenuDirty )
  {
    m_vMenuEdit->clear();

    KStdAccel stdAccel; // Get standard accelarators (copy, paste, cut)

    OpenPartsUI::Pixmap_var pix = OPUIUtils::convertPixmap( *KPixmapCache::toolbarPixmap( "editcopy.png" ) );
    CORBA::WString_var text = Q2C( i18n("&Copy") );
    m_vMenuEdit->insertItem6( pix, text, this, "slotCopy", stdAccel.copy(), MEDIT_COPY_ID, -1 );
    pix = OPUIUtils::convertPixmap( *KPixmapCache::toolbarPixmap( "editpaste.png" ) );
    text = Q2C( i18n("&Paste") );
    m_vMenuEdit->insertItem6( pix, text, this, "slotPaste", stdAccel.paste(), MEDIT_PASTE_ID, -1 );
    pix = OPUIUtils::convertPixmap( *KPixmapCache::pixmap( "kfm_trash.png", true ) );
    text = Q2C( i18n("&Move to Trash") );
    m_vMenuEdit->insertItem6( pix, text, this, "slotTrash", stdAccel.cut(), MEDIT_TRASH_ID, -1 );
    text = Q2C( i18n("&Delete") );
    m_vMenuEdit->insertItem4( text, this, "slotDelete", CTRL+Key_Delete, MEDIT_DELETE_ID, -1 );
    m_vMenuEdit->insertSeparator( -1 );

    checkEditExtension();

    if ( m_currentView )
      EMIT_EVENT( m_currentView->view(), Browser::View::eventFillMenuEdit, m_vMenuEdit );

    m_bEditMenuDirty = false;
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

void KonqMainView::fillProfileMenu()
{
  m_vMenuWindowProfiles->clear();
  
  QStringList dirs = KGlobal::dirs()->findDirs( "data", "konqueror/profiles/" );
  QStringList::ConstIterator dIt = dirs.begin();
  QStringList::ConstIterator dEnd = dirs.end();
  
  for (; dIt != dEnd; ++dIt )
  {
    QDir dir( *dIt );
    QStringList entries = dir.entryList( QDir::Files );
    
    QStringList::ConstIterator eIt = entries.begin();
    QStringList::ConstIterator eEnd = entries.end();
    
    CORBA::WString_var text;
    
    for (; eIt != eEnd; ++eIt )
      m_vMenuWindowProfiles->insertItem7( ( text = Q2C( *eIt ) ), -1, -1 );
  }
}

QStringList KonqMainView::locationBarCombo()
{
  QStringList entryList;
  
  CORBA::WString_var item;
  CORBA::Long count = m_vLocationBar->comboItemCount( TOOLBAR_URL_ID );
  for ( CORBA::ULong i = 0; i < (CORBA::ULong)count; i++ )
    entryList.append( C2Q( item = m_vLocationBar->comboItem( TOOLBAR_URL_ID, i ) ) );

  return entryList;
}

void KonqMainView::setLocationBarCombo( const QStringList &entryList )
{
  m_vToolBar->clearCombo( TOOLBAR_URL_ID );

  CORBA::WString_var item;
  QStringList::ConstIterator it = entryList.begin();
  QStringList::ConstIterator end = entryList.end();

  for (; it != end; ++it )
    m_vLocationBar->insertComboItem( TOOLBAR_URL_ID, ( item = Q2C( *it ) ), -1 );
    
  if ( entryList.count() == 0 )
    m_vLocationBar->insertComboItem( TOOLBAR_URL_ID, 0L, -1 );
}

#include "konq_mainview.moc"
