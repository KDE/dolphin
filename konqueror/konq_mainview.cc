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

#include "konq_mainview.h"
#include "userpaths.h"
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
#include "kfmguiprops.h"
#include "kfmpopup.h"
#include "kfmrun.h"

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
#include <kclipboard.h>
#include <kconfig.h>
#include <kio_cache.h>
#include <kio_openwith.h>
#include <kio_openwith.h>
#include <kio_paste.h>
#include <kio_propsdlg.h>
#include <kkeydialog.h>
#include <klineeditdlg.h>
#include <kpixmapcache.h>
#include <kprotocolmanager.h>
#include <kservices.h>
#include <kstdaccel.h>
#include <kuserprofile.h>

#include <iostream>
#include <assert.h>

enum _ids {
/////  toolbar gear and lineedit /////
    TOOLBAR_GEAR_ID, TOOLBAR_URL_ID,

////// menu items //////
    MFILE_NEW_ID, MFILE_NEWWINDOW_ID, MFILE_RUN_ID, MFILE_OPENTERMINAL_ID,
    MFILE_OPENLOCATION_ID, MFILE_FIND_ID, MFILE_PRINT_ID, MFILE_CLOSE_ID,

    MEDIT_COPY_ID, MEDIT_PASTE_ID, MEDIT_TRASH_ID, MEDIT_DELETE_ID, MEDIT_SELECT_ID,
    MEDIT_SELECTALL_ID, // MEDIT_FINDINPAGE_ID, MEDIT_FINDNEXT_ID,
    MEDIT_MIMETYPES_ID, MEDIT_APPLICATIONS_ID, // later, add global mimetypes and apps here
    MEDIT_SAVEGEOMETRY_ID,

    MVIEW_SPLITWINDOW_ID, MVIEW_ROWABOVE_ID, MVIEW_ROWBELOW_ID, MVIEW_REMOVEVIEW_ID, 
    MVIEW_SHOWDOT_ID, MVIEW_SHOWHTML_ID,
    MVIEW_LARGEICONS_ID, MVIEW_SMALLICONS_ID, MVIEW_TREEVIEW_ID, 
    MVIEW_RELOAD_ID, MVIEW_STOP_ID,
    // + view frame source, view document source, document encoding

    MGO_UP_ID, MGO_BACK_ID, MGO_FORWARD_ID, MGO_HOME_ID,
    MGO_CACHE_ID, MGO_HISTORY_ID, MGO_MIMETYPES_ID, MGO_APPLICATIONS_ID,

    // clear cache is needed somewhere
    // MOPTIONS_...

    MHELP_HELP_ID
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

  m_vStatusBar = 0L;

  m_pRun = 0L;

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

  m_vStatusBar->insertItem( i18n("Konqueror :-)"), 1 );

  m_vStatusBar->enable( OpenPartsUI::Show );
  if ( !m_Props->m_bShowStatusBar )
    m_vStatusBar->enable( OpenPartsUI::Hide );
    
  initGui();

//  KonqPlugins::installKOMPlugins( this ); <<--- enable this only if you want to fire up mainview plugins
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

  if ( m_pRun )
    delete m_pRun;
  
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
  if (!KfmGuiProps::m_pDefaultProps)
  {
    kdebug(0,1202,"Reading global config");
    KConfig *config = kapp->getConfig();
    config->setGroup( "Settings" );
    KfmGuiProps::m_pDefaultProps = new KfmGuiProps(config);
  }

  // For the moment, no local properties
  // Copy the default properties
  m_Props = new KfmGuiProps( *KfmGuiProps::m_pDefaultProps );

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
  initView();

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
  //row->setOpaqueResize( TRUE );
  if ( append )
    m_lstRows.append( row );
  else
  {
    m_lstRows.insert( 0, row );
    m_pMainSplitter->moveToFirst( row );
  }
  row->show();
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
  m_pMainSplitter->show();
}

void KonqMainView::initView()
{
  Konqueror::View_var vView1 = Konqueror::View::_duplicate( new KonqKfmIconView );
  insertView( vView1, Konqueror::left );

  MapViews::Iterator it = m_mapViews.find( vView1->id() );
  it.data()->lockHistory(); // first URL won't go into history
  it.data()->openURL( m_sInitialURL );

  setActiveView( vView1->id() );
}

bool KonqMainView::event( const char* event, const CORBA::Any& value )
{
  EVENT_MAPPER( event, value );

  MAPPING( OpenPartsUI::eventCreateMenuBar, OpenPartsUI::typeCreateMenuBar_var, mappingCreateMenubar );
  MAPPING( OpenPartsUI::eventCreateToolBar, OpenPartsUI::typeCreateToolBar_var, mappingCreateToolbar );
  MAPPING( OpenParts::eventChildGotFocus, OpenParts::Part_var, mappingChildGotFocus );
  MAPPING( OpenParts::eventParentGotFocus, OpenParts::Part_var, mappingParentGotFocus );
  MAPPING( Konqueror::eventOpenURL, Konqueror::EventOpenURL, mappingOpenURL );

  END_EVENT_MAPPER;

  return false;
}

bool KonqMainView::mappingCreateMenubar( OpenPartsUI::MenuBar_ptr menuBar )
{

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
    createViewMenu();
    m_vMenuGo = 0L;
    m_vMenuBookmarks = 0L;
    m_vMenuOptions = 0L;

    return true;
  }

  KStdAccel stdAccel;

  CORBA::Long m_idMenuFile = menuBar->insertMenu( i18n("&File"), m_vMenuFile, -1, -1 );

  m_vMenuFile->insertItem8( i18n("&New"), m_vMenuFileNew, MFILE_NEW_ID, -1 );

  m_vMenuFileNew->connect("activated", this, "slotFileNewActivated");
  m_vMenuFileNew->connect("aboutToShow", this, "slotFileNewAboutToShow");
  
  m_pMenuNew = new KNewMenu( m_vMenuFileNew );

  m_vMenuFile->insertItem4( i18n("New &Window"), this, "slotNewWindow", stdAccel.openNew(), MFILE_NEWWINDOW_ID, -1 );
  m_vMenuFile->insertSeparator( -1 );
  m_vMenuFile->insertItem4( i18n("&Run..."), this, "slotRun", 0, MFILE_RUN_ID, -1  );
  m_vMenuFile->insertItem4( i18n("Open &Terminal"), this, "slotTerminal", CTRL+Key_T, MFILE_OPENTERMINAL_ID, -1 );
  m_vMenuFile->insertSeparator( -1 );

  m_vMenuFile->insertItem4( i18n("&Open Location..."), this, "slotOpenLocation", stdAccel.open(), MFILE_OPENLOCATION_ID, -1 );
  m_vMenuFile->insertItem4( i18n("&Find"), this, "slotToolFind", stdAccel.find(), MFILE_FIND_ID, -1 );
  m_vMenuFile->insertSeparator( -1 );
  m_vMenuFile->insertItem4( i18n("&Print..."), this, "slotPrint", stdAccel.print(), MFILE_PRINT_ID, -1 );
  m_vMenuFile->insertSeparator( -1 );
  m_vMenuFile->insertItem4( i18n("&Close"), this, "slotClose", stdAccel.close(), MFILE_CLOSE_ID, -1 );

  menuBar->setFileMenu( m_idMenuFile );

  menuBar->insertMenu( i18n("&Edit"), m_vMenuEdit, -1, -1 );

  m_vMenuEdit->insertItem4( i18n("&Copy"), this, "slotCopy", stdAccel.copy(), MEDIT_COPY_ID, -1 );
  m_vMenuEdit->insertItem4( i18n("&Paste"), this, "slotPaste", stdAccel.paste(), MEDIT_PASTE_ID, -1 );
  m_vMenuEdit->insertItem4( i18n("&Move to Trash"), this, "slotTrash", stdAccel.cut(), MEDIT_TRASH_ID, -1 );
  m_vMenuEdit->insertItem4( i18n("&Delete"), this, "slotDelete", CTRL+Key_Delete, MEDIT_DELETE_ID, -1 );
  m_vMenuEdit->insertSeparator( -1 );
  m_vMenuEdit->insertItem4( i18n("&Select"), this, "slotSelect", CTRL+Key_S, MEDIT_SELECT_ID, -1 );
  m_vMenuEdit->insertItem4( i18n("Select &all"), this, "slotSelectAll", CTRL+Key_A, MEDIT_SELECTALL_ID, -1 );
  m_vMenuEdit->insertSeparator( -1 );
  m_vMenuEdit->insertItem4( i18n("Save &Geometry"), this, "slotSaveGeometry", 0, MEDIT_SAVEGEOMETRY_ID, -1 );

  menuBar->insertMenu( i18n("&View"), m_vMenuView, -1, -1 );  
  
  createViewMenu();
  
  menuBar->insertMenu( i18n("&Go"), m_vMenuGo, -1, -1 );

  m_vMenuGo->insertItem4( i18n("&Up"), this, "slotUp", 0, MGO_UP_ID, -1 );
  m_vMenuGo->insertItem4( i18n("&Back"), this, "slotBack", 0, MGO_BACK_ID, -1 );
  m_vMenuGo->insertItem4( i18n("&Forward"), this, "slotForward", 0, MGO_FORWARD_ID, -1 );
  m_vMenuGo->insertItem4( i18n("&Home"), this, "slotHome", 0, MGO_HOME_ID, -1 );
  m_vMenuGo->insertSeparator( -1 );
  m_vMenuGo->insertItem4( i18n("&Cache"), this, "slotShowCache", 0, MGO_CACHE_ID, -1 );
  m_vMenuGo->insertItem4( i18n("&History"), this, "slotShowHistory", 0, MGO_HISTORY_ID, -1 );
  m_vMenuGo->insertItem4( i18n("Mime &Types"), this, "slotEditMimeTypes", 0, MGO_MIMETYPES_ID, -1 );
  m_vMenuGo->insertItem4( i18n("App&lications"), this, "slotEditApplications", 0, MGO_APPLICATIONS_ID, -1 );
  //TODO: Global mime types and applications for root

  menuBar->insertMenu( i18n("&Bookmarks"), m_vMenuBookmarks, -1, -1 );
  m_pBookmarkMenu = new KBookmarkMenu( this, m_vMenuBookmarks, this, true );

  menuBar->insertMenu( i18n("&Options"), m_vMenuOptions, -1, -1 );

  m_vMenuOptions->insertItem( i18n("Configure &keys"), this, "slotConfigureKeys", 0 );

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

  m_vToolBar->setFullWidth( false );

  pix = OPUIUtils::convertPixmap( *KPixmapCache::toolbarPixmap( "up.xpm" ) );
  m_vToolBar->insertButton2( pix, MGO_UP_ID, SIGNAL(clicked()),
                             this, "slotUp", false, i18n("Up"), -1);

  pix = OPUIUtils::convertPixmap( *KPixmapCache::toolbarPixmap( "back.xpm" ) );
  m_vToolBar->insertButton2( pix, MGO_BACK_ID, SIGNAL(clicked()),
                             this, "slotBack", false, i18n("Back"), -1);

  pix = OPUIUtils::convertPixmap( *KPixmapCache::toolbarPixmap( "forward.xpm" ) );
  m_vToolBar->insertButton2( pix, MGO_FORWARD_ID, SIGNAL(clicked()),
                             this, "slotForward", false, i18n("Forward"), -1);

  pix = OPUIUtils::convertPixmap( *KPixmapCache::toolbarPixmap( "home.xpm" ) );
  m_vToolBar->insertButton2( pix, MGO_HOME_ID, SIGNAL(clicked()),
                             this, "slotHome", true, i18n("Home"), -1);

  m_vToolBar->insertSeparator( -1 );

  pix = OPUIUtils::convertPixmap( *KPixmapCache::toolbarPixmap( "reload.xpm" ) );
  m_vToolBar->insertButton2( pix, MVIEW_RELOAD_ID, SIGNAL(clicked()),
                             this, "slotReload", true, i18n("Reload"), -1);

  m_vToolBar->insertSeparator( -1 );

  pix = OPUIUtils::convertPixmap( *KPixmapCache::toolbarPixmap( "editcopy.xpm" ) );
  m_vToolBar->insertButton2( pix, MEDIT_COPY_ID, SIGNAL(clicked()),
                             this, "slotCopy", true, i18n("Copy"), -1);

  pix = OPUIUtils::convertPixmap( *KPixmapCache::toolbarPixmap( "editpaste.xpm" ) );
  m_vToolBar->insertButton2( pix, MEDIT_PASTE_ID, SIGNAL(clicked()),
                             this, "slotPaste", true, i18n("Paste"), -1);
 				
  m_vToolBar->insertSeparator( -1 );				

  pix = OPUIUtils::convertPixmap( *KPixmapCache::toolbarPixmap( "help.xpm" ) );
  m_vToolBar->insertButton2( pix, MHELP_HELP_ID, SIGNAL(clicked()),
                             this, "slotHelp", true, i18n("Stop"), -1);
				
  m_vToolBar->insertSeparator( -1 );				

  pix = OPUIUtils::convertPixmap( *KPixmapCache::toolbarPixmap( "stop.xpm" ) );
  m_vToolBar->insertButton2( pix, MVIEW_STOP_ID, SIGNAL(clicked()),
                             this, "slotStop", false, i18n("Stop"), -1);

  pix = OPUIUtils::convertPixmap( *KPixmapCache::toolbarPixmap( "kde1.xpm" ) );
  m_vToolBar->insertButton2( pix, TOOLBAR_GEAR_ID, SIGNAL(clicked()),
                             this, "slotNewWindow", true, "", -1 );
  m_vToolBar->alignItemRight( TOOLBAR_GEAR_ID, true );

  m_vToolBar->enable( OpenPartsUI::Show );
  if ( !m_Props->m_bShowToolBar )
    m_vToolBar->enable( OpenPartsUI::Hide );

  //TODO: m_vToolBar->setBarPos( convert_to_openpartsui_bar_pos( m_Props->m_toolBarPos ) ) );

  m_vLocationBar = factory->create( OpenPartsUI::ToolBarFactory::Transient );

  m_vLocationBar->setFullWidth( true );

  m_vLocationBar->insertTextLabel( i18n("Location : "), -1, -1 );

  m_vLocationBar->insertLined("", TOOLBAR_URL_ID, SIGNAL(returnPressed()), this, "slotURLEntered", true, i18n("Current Location"), 70, -1 );
  m_vLocationBar->setItemAutoSized( TOOLBAR_URL_ID, true );
  if ( m_currentView )
    m_vLocationBar->setLinedText( TOOLBAR_URL_ID, m_currentView->locationBarURL().ascii() );

  //TODO: support completion in opToolBar->insertLined....
  //TODO: m_vLocationBar->setBarPos( convert_to_openpartsui_bar_pos( m_Props->m_locationBarPos ) ) );

  m_vLocationBar->enable( OpenPartsUI::Show );
  if ( !m_Props->m_bShowLocationBar )
    m_vLocationBar->enable( OpenPartsUI::Hide );

  kdebug(0,1202,"KonqMainView::mappingCreateToolbar : done !");
  return true;
}

bool KonqMainView::mappingChildGotFocus( OpenParts::Part_ptr child )
{
  kdebug(0, 1202, "bool KonqMainView::mappingChildGotFocus( OpenParts::Part_ptr child )");
  setActiveView( child->id() );
  return true;
}

bool KonqMainView::mappingParentGotFocus( OpenParts::Part_ptr child )
{
  kdebug(0, 1202, "bool KonqMainView::mappingParentGotFocus( OpenParts::Part_ptr child )");
  // removing view-specific menu entries (view will probably be destroyed !)
  if (m_currentView)
  {
    m_currentView->emitEventViewMenu( m_vMenuView, false );
    m_currentView->repaint();
  }
  m_currentView = 0L;

  // no more active view (even temporarily)
  slotSetUpEnabled( "/", 0 );
  setItemEnabled( m_vMenuGo, MGO_BACK_ID, false );
  setItemEnabled( m_vMenuGo, MGO_FORWARD_ID, false );

  return true;
}

bool KonqMainView::mappingOpenURL( Konqueror::EventOpenURL eventURL )
{
  openURL( eventURL.url, eventURL.reload );
  return true;
}

void KonqMainView::insertView( Konqueror::View_ptr view,
                               Konqueror::NewViewPosition newViewPosition )
{
  Row * currentRow;
  if ( m_currentView )
    currentRow = m_currentView->row();
  else // complete beginning, we don't even have a view
    currentRow = m_lstRows.first();

  if (newViewPosition == Konqueror::above || 
      newViewPosition == Konqueror::below)
  {
    kdebug(0,1202,"Creating a new row");
    currentRow = newRow( (newViewPosition == Konqueror::below) ); // append if below
    // Now insert a view, say on the right (doesn't matter)
    newViewPosition = Konqueror::right;
  }

  KonqChildView *v = new KonqChildView( view, currentRow, newViewPosition,
                                        this, m_vMainWindow );
  QObject::connect( v, SIGNAL(sigIdChanged( KonqChildView *, OpenParts::Id, OpenParts::Id )), 
                    this, SLOT(slotIdChanged( KonqChildView * , OpenParts::Id, OpenParts::Id ) ));
  QObject::connect( v, SIGNAL(sigSetUpEnabled( QString, OpenParts::Id )),
                    this, SLOT(slotSetUpEnabled( QString, OpenParts::Id )) );

  m_mapViews.insert( view->id(), v );

  setItemEnabled( m_vMenuView, MVIEW_REMOVEVIEW_ID, true );
}

void KonqMainView::setActiveView( OpenParts::Id id )
{
  KonqChildView* previousView = m_currentView;
  // clean view-specific part of the view menu
  if ( previousView != 0L )
    previousView->emitEventViewMenu( m_vMenuView, false );
  
  MapViews::Iterator it = m_mapViews.find( id );
  assert( it != m_mapViews.end() );
  
  m_currentView = it.data();
  assert( m_currentView );
  m_currentId = id;

  slotSetUpEnabled( m_currentView->url(), id );
  setItemEnabled( m_vMenuGo, MGO_BACK_ID, m_currentView->canGoBack() );
  setItemEnabled( m_vMenuGo, MGO_FORWARD_ID, m_currentView->canGoForward() );

  if ( !CORBA::is_nil( m_vLocationBar ) )
    m_vLocationBar->setLinedText( TOOLBAR_URL_ID, m_currentView->locationBarURL().ascii() );

  m_currentView->emitEventViewMenu( m_vMenuView, true );
  if (previousView != 0L) // might be 0L e.g. if we just removed the current view
    previousView->repaint();
  m_currentView->repaint();
}

Konqueror::View_ptr KonqMainView::activeView()
{
  if ( m_currentView )
    return Konqueror::View::_duplicate( m_currentView->view() );
  else
    return Konqueror::View::_nil();
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
    (*seq)[ i ] = it.data()->view(); // no duplicate here ?
  }

  return seq;
}

void KonqMainView::removeView( OpenParts::Id id )
{
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
      
    delete it.data();
    m_mapViews.remove( it );
  }
  if ( m_mapViews.count() == 1 )
    setItemEnabled( m_vMenuView, MVIEW_REMOVEVIEW_ID, false );
}

void KonqMainView::slotIdChanged( KonqChildView * childView, OpenParts::Id oldId, OpenParts::Id newId )
{
  m_mapViews.remove( oldId );
  m_mapViews.insert( newId, childView );
  if ( oldId == m_currentId)
    m_currentId = newId;
}

void KonqMainView::openURL( const Konqueror::URLRequest &url )
{
  CORBA::String_var u = url.url;
  openURL( u.in(), url.reload );
}

void KonqMainView::openURL( const char * _url, CORBA::Boolean _reload )
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
    string tmp = i18n("Malformed URL\n").ascii();
    tmp += _url;
    QMessageBox::critical( (QWidget*)0L, i18n( "Error" ), tmp.c_str(), i18n( "OK" ) );
    return;
  }

  //FIXME... this is far for being complete (Simon)
  //(for example: obey the reload flag...)
  
  slotStop(); //hm....
    
  m_pRun = new KfmRun( this, url, 0, false, false );
}

void KonqMainView::setStatusBarText( const char *_text )
{
  if ( !CORBA::is_nil( m_vStatusBar ) )
    m_vStatusBar->changeItem( _text, 1 );
}

void KonqMainView::setLocationBarURL( OpenParts::Id id, const char *_url )
{
  MapViews::Iterator it = m_mapViews.find( id );
  assert( it != m_mapViews.end() );
  
  it.data()->setLocationBarURL( _url );
  
  if ( ( id == m_currentId ) && (!CORBA::is_nil( m_vLocationBar ) ) )
    m_vLocationBar->setLinedText( TOOLBAR_URL_ID, _url );

  // hmm, not the right URL it seems. slotSetUpEnabled( _url, id );
}

void KonqMainView::setItemEnabled( OpenPartsUI::Menu_ptr menu, int id, bool enable )
{
  // enable menu item, and if present in toolbar, related button
  // this will allow configurable toolbar buttons later
  if ( !CORBA::is_nil( menu ) ) 
    menu->setItemEnabled( id, enable );
 
  if ( !CORBA::is_nil( m_vToolBar ) ) // TODO : and if such ID exists in toolbar
    m_vToolBar->setItemEnabled( id, enable );
} 

void KonqMainView::slotSetUpEnabled( QString _url, OpenParts::Id _id )
{
  if ( _id != m_currentId )
    return;

  KURL u;
  bool bHasUpURL = false;
  
  if ( !_url.isNull() )
  {
    u = _url;
    if ( u.hasPath() )
    {
      kdebug(0,1202,"u.path() = '%s'",u.path().data());
      bHasUpURL = ( strcmp( u.path(), "/") != 0 );
    }
  }
  
  setItemEnabled( m_vMenuGo, MGO_UP_ID, bHasUpURL );
}

void KonqMainView::createNewWindow( const char *url )
{
  KonqMainWindow *m_pShell = new KonqMainWindow( url );
  m_pShell->show();
}

void KonqMainView::popupMenu( const Konqueror::View::MenuPopupRequest &popup )
{
  assert( popup.urls.length() >= 1 );
  m_popupMode = popup.mode;

  OPMenu *m_popupMenu = new OPMenu;
  bool bHttp          = true;
  bool isTrash        = true;
  bool currentDir     = false;
  bool isCurrentTrash = false;
  bool sReading       = true;
  bool sWriting       = true;
  bool sDeleting      = true;
  bool sMoving        = true;
  int id;
  KNewMenu *m_menuNew = 0L;

  KProtocolManager pManager = KProtocolManager::self();
  
  KURL url;
  CORBA::ULong i;
  // Check whether all URLs are correct
  for ( i = 0; i < popup.urls.length(); i++ )
  {
    url = KURL( popup.urls[i].in() );
    const char* protocol = url.protocol();

    if ( url.isMalformed() )
    {
//FIXME?
//      emit error( ERR_MALFORMED_URL, s );
      return;
    }
    if ( url.protocol() != "http" ) bHttp = false; // not HTTP

    // check if all urls are in the trash
    if ( isTrash )
    {
      QString path = url.path();
      if ( path.right(1) != "/" )
	path += "/";
    
      if ( strcmp( protocol, "file" ) != 0L ||
	   path != UserPaths::trashPath() )
	isTrash = false;
    }

    if ( sReading )
      sReading = pManager.supportsReading( protocol );

    if ( sWriting )
      sWriting = pManager.supportsWriting( protocol );
    
    if ( sDeleting )
      sDeleting = pManager.supportsDeleting( protocol );

    if ( sMoving )
      sMoving = pManager.supportsMoving( protocol );
  }

  QString viewURL = m_currentView->url();
  
  //check if current url is trash
  url = KURL( viewURL );
  QString path = url.path();
  if ( path.right(1) != "/" )
    path += "/";
    
  if ( strcmp( url.protocol(), "file" ) == 0L &&
       path == UserPaths::trashPath() )
    isTrash = true;

  //check if url is current directory
  if ( popup.urls.length() == 1 )
    if ( strcmp( viewURL, ((popup.urls)[0]) ) == 0 )
      currentDir = true;

  QObject::disconnect( m_popupMenu, SIGNAL( activated( int ) ), this, SLOT( slotPopup( int ) ) );

  m_popupMenu->clear();

//   //---------- Sven --------------
//   // check if menubar is hidden and if yes add "Show Menubar"
//   if (view->getGUI()->isMenuBarHidden())
//   {
    
//     popupMenu->insertItem(klocale->getAlias(ID_STRING_SHOW_MENUBAR),
// 			  view->getGUI(), SLOT(slotShowMenubar()));
//     popupMenu->insertSeparator();
//   }
//   //------------------------------

  if ( isTrash )
  {
    /* Commented out. Left click does it. Why have it on right click menu ?. David.
       id = popupMenu->insertItem( klocale->getAlias(ID_STRING_CD), 
       view, SLOT( slotPopupCd() ) );
    */
    id = m_popupMenu->insertItem( i18n( "New view" ), 
				  this, SLOT( slotPopupNewView() ) );
    m_popupMenu->insertSeparator();    
    id = m_popupMenu->insertItem( i18n( "Empty Trash Bin" ), 
				  this, SLOT( slotPopupEmptyTrashBin() ) );
  } 
  else if ( S_ISDIR( (mode_t)popup.mode ) )
  {
    //we don't want to use OpenParts here, because of "missing" interface 
    //methods for the popup menu (wouldn't make much sense imho) (Simon)    
    m_menuNew = new KNewMenu(); 
    id = m_popupMenu->insertItem( i18n("&New"), m_menuNew->popupMenu() );
    m_popupMenu->insertSeparator();

    id = m_popupMenu->insertItem( *KPixmapCache::toolbarPixmap( "up.xpm" ), i18n( "Up" ), this, SLOT( slotUp() ), 100 );
    // TODO !
    // if ( !m_currentView->m_strUpURL.isEmpty() )
    //   m_popupMenu->setItemEnabled( id, false );

    id = m_popupMenu->insertItem( *KPixmapCache::toolbarPixmap( "back.xpm" ), i18n( "Back" ), this, SLOT( slotBack() ), 101 );
    if ( m_currentView->canGoBack() )
      m_popupMenu->setItemEnabled( id, false );

    id = m_popupMenu->insertItem( *KPixmapCache::toolbarPixmap( "forward.xpm" ), i18n( "Forward" ), this, SLOT( slotForward() ), 102 );
    if ( m_currentView->canGoForward() )
      m_popupMenu->setItemEnabled( id, false );

    m_popupMenu->insertSeparator();  

    id = m_popupMenu->insertItem( i18n( "New View"), this, SLOT( slotPopupNewView() ) );
    m_popupMenu->insertSeparator();    

    if ( sReading )
      id = m_popupMenu->insertItem( *KPixmapCache::toolbarPixmap( "editcopy.xpm" ), i18n( "Copy" ), this, SLOT( slotPopupCopy() ) );
    if ( sWriting )
      id = m_popupMenu->insertItem( *KPixmapCache::toolbarPixmap( "editpaste.xpm" ), i18n( "Paste" ), this, SLOT( slotPopupPaste() ) );
    if ( isClipboardEmpty() )
      m_popupMenu->setItemEnabled( id, false );
    if ( sMoving && !isCurrentTrash && !currentDir )
      id = m_popupMenu->insertItem( *KPixmapCache::pixmap( "kfm_trash.xpm", true ), i18n( "Move to trash" ), this, SLOT( slotPopupTrash() ) );
    if ( sDeleting && !currentDir )
      id = m_popupMenu->insertItem( i18n( "Delete" ), this, SLOT( slotPopupDelete() ) );
  }
  else
  {
    if ( bHttp )
    {
      /* Should be for http URLs (HTML pages) only ... */
      id = m_popupMenu->insertItem( i18n( "New View"), this, SLOT( slotPopupNewView() ) );
    }
    id = m_popupMenu->insertItem( i18n( "Open with" ), this, SLOT( slotPopupOpenWith() ) );
    m_popupMenu->insertSeparator();

    if ( sReading )
      id = m_popupMenu->insertItem( *KPixmapCache::toolbarPixmap( "editcopy.xpm" ), i18n( "Copy" ), this, SLOT( slotPopupCopy() ) );
    if ( sMoving && !isCurrentTrash && !currentDir )
      id = m_popupMenu->insertItem( *KPixmapCache::pixmap( "kfm_trash.xpm", true ), i18n( "Move to trash" ), this, SLOT( slotPopupTrash() ) );
    if ( sDeleting /* && !_current_dir */)
      id = m_popupMenu->insertItem( i18n( "Delete" ), this, SLOT( slotPopupDelete() ) );
  }

  id = m_popupMenu->insertItem( i18n( "Add To Bookmarks" ), this, SLOT( slotPopupBookmarks() ) );

  m_lstPopupURLs.clear();
  for ( i = 0; i < popup.urls.length(); i++ )
    m_lstPopupURLs.append( (popup.urls)[i].in() );

  if ( m_menuNew ) m_menuNew->setPopupFiles( m_lstPopupURLs );

  // Do all URLs have the same mimetype ?
  url = KURL( m_lstPopupURLs.getFirst() );


  KMimeType* mime = KMimeType::findByURL( url, (mode_t)popup.mode, (bool)popup.isLocalFile );
  ASSERT( mime );
  QStringList::Iterator it = m_lstPopupURLs.begin();
  for( ; it != m_lstPopupURLs.end(); ++it )
  {
    KURL u( *it );  
    KMimeType* m = KMimeType::findByURL( u, (mode_t)popup.mode, (bool)popup.isLocalFile );
    if ( m != mime )
      mime = 0L;
  }
  
  if ( mime )
  {
    KServiceTypeProfile::OfferList offers = KServiceTypeProfile::offers( mime->name() );

    QValueList<KDELnkMimeType::Service> builtin;
    QValueList<KDELnkMimeType::Service> user;
    if ( mime->name() == "application/x-kdelnk" )
    {
      builtin = KDELnkMimeType::builtinServices( url );
      user = KDELnkMimeType::userDefinedServices( url );
    }
  
    if ( !offers.isEmpty() || !user.isEmpty() || !builtin.isEmpty() )
      QObject::connect( m_popupMenu, SIGNAL( activated( int ) ), this, SLOT( slotPopup( int ) ) );

    if ( !offers.isEmpty() || !user.isEmpty() )
      m_popupMenu->insertSeparator();
  
    m_mapPopup.clear();
    m_mapPopup2.clear();
  
    KServiceTypeProfile::OfferList::Iterator it = offers.begin();
    for( ; it != offers.end(); it++ )
    {    
      id = m_popupMenu->insertItem( *(KPixmapCache::pixmap( it->service().icon(), true ) ),
				    it->service().name() );
      m_mapPopup[ id ] = &(it->service());
    }
    
    QValueList<KDELnkMimeType::Service>::Iterator it2 = user.begin();
    for( ; it2 != user.end(); ++it2 )
    {
      if ( !it2->m_strIcon.isEmpty() )
	id = m_popupMenu->insertItem( *(KPixmapCache::pixmap( it2->m_strIcon, true ) ), it2->m_strName );
      else
	id = m_popupMenu->insertItem( it2->m_strName );
      m_mapPopup2[ id ] = *it2;
    }
    
    if ( builtin.count() > 0 )
      m_popupMenu->insertSeparator();

    it2 = builtin.begin();
    for( ; it2 != builtin.end(); ++it2 )
    {
      if ( !it2->m_strIcon.isEmpty() )
	id = m_popupMenu->insertItem( *(KPixmapCache::pixmap( it2->m_strIcon, true ) ), it2->m_strName );
      else
	id = m_popupMenu->insertItem( it2->m_strName );
      m_mapPopup2[ id ] = *it2;
    }
  }
  
  if ( m_lstPopupURLs.count() == 1 )
  {
    m_popupMenu->insertSeparator();
    m_popupMenu->insertItem( i18n("Properties"), this, SLOT( slotPopupProperties() ) );
  }

  // m_popupMenu->insertSeparator();
  // Konqueror::View::EventCreateViewMenu viewMenu;
  // viewMenu.menu = OpenPartsUI::Menu::_duplicate( m_popupMenu->interface() );
  // EMIT_EVENT( m_currentView->m_vView, Konqueror::View::eventCreateViewMenu, viewMenu );
    
  m_popupMenu->exec( QPoint( popup.x, popup.y ) );

  //  viewMenu.menu = OpenPartsUI::Menu::_nil();
  // EMIT_EVENT( m_currentView->m_vView, Konqueror::View::eventCreateViewMenu, viewMenu );
    
  delete m_popupMenu;
  if ( m_menuNew ) delete m_menuNew;
}

void KonqMainView::openDirectory( const char *url )
{
  m_pRun = 0L;

  m_currentView->changeViewMode( "KonquerorKfmIconView" );  

  //TODO: check for html index file and stuff (Simon)
    
  // Do we perhaps want to display a html index file ? => Save the path of the URL
  //QString tmppath;
  //if ( lst.size() == 1 && lst.front().isLocalFile() /*&& isHTMLAllowed()*/ )
  //tmppath = lst.front().path();

  m_currentView->openURL( url );
}

void KonqMainView::openHTML( const char *url )
{
  m_pRun = 0L;
  
  m_currentView->changeViewMode( "KonquerorHTMLView" );
  m_currentView->openURL( url );
}

void KonqMainView::openPluginView( const char *url, Konqueror::View_ptr view )
{
  m_pRun = 0L;
  Konqueror::View_var vView = Konqueror::View::_duplicate( view );

  m_currentView->switchView( vView );
  m_currentView->openURL( url );

  slotSetUpEnabled( QString::null, m_currentId ); // HACK.
     // How can we really know if a plugin supports 'up' ?
}

void KonqMainView::openText( const char *url )
{
  m_pRun = 0L;
  
  m_currentView->changeViewMode( "KonquerorTxtView" );
  m_currentView->openURL( url );
}

// protected
void KonqMainView::splitView ( Konqueror::NewViewPosition newViewPosition )
{
  QString url = m_currentView->url();
  QString viewName = m_currentView->viewName();

  Konqueror::View_var vView = m_currentView->createViewByName( viewName );
  insertView( vView, newViewPosition );
  MapViews::Iterator it = m_mapViews.find( vView->id() );
  it.data()->openURL( url );

  //hack to fix slotRowAbove()
  resize( width()+1, height()+1 );
  resize( width()-1, height()-1 );
}

void KonqMainView::createViewMenu()
{
  if ( !CORBA::is_nil( m_vMenuView ) )
  {
    m_vMenuView->clear();
  
    m_vMenuView->setCheckable( true );
    //  m_vMenuView->insertItem4( i18n("Show Directory Tr&ee"), this, "slotShowTree" , 0 );
    m_vMenuView->insertItem4( i18n("Split &window"), this, "slotSplitView" , 0, MVIEW_SPLITWINDOW_ID, -1 );
    m_vMenuView->insertItem4( i18n("Add row &above"), this, "slotRowAbove" , 0, MVIEW_ROWABOVE_ID, -1 );
    m_vMenuView->insertItem4( i18n("Add row &below"), this, "slotRowBelow" , 0, MVIEW_ROWBELOW_ID, -1 );
    m_vMenuView->insertItem4( i18n("Remove view"), this, "slotRemoveView" , 0, MVIEW_REMOVEVIEW_ID, -1 );
    m_vMenuView->insertSeparator( -1 );
    
    // Two namings for the same thing ! We have to decide ourselves. 
    // I prefer the second one, because of .kde.html
    //m_vMenuView->insertItem4( i18n("&Always Show index.html"), this, "slotShowHTML" , 0, MVIEW_SHOWHTML_ID, -1 );
    m_vMenuView->insertItem4( i18n("&Use HTML"), this, "slotShowHTML" , 0, MVIEW_SHOWHTML_ID, -1 );
    
    m_vMenuView->insertSeparator( -1 );

    m_vMenuView->insertItem4( i18n("&Large Icons"), this, "slotLargeIcons" , 0, MVIEW_LARGEICONS_ID, -1 );
    m_vMenuView->insertItem4( i18n("&Small Icons"), this, "slotSmallIcons" , 0, MVIEW_SMALLICONS_ID, -1 );
    m_vMenuView->insertItem4( i18n("&Tree View"), this, "slotTreeView" , 0, MVIEW_TREEVIEW_ID, -1 );
    m_vMenuView->insertSeparator( -1 );

    m_vMenuView->insertItem4( i18n("&Reload Document"), this, "slotReload" , Key_F5, MVIEW_RELOAD_ID, -1 );
    m_vMenuView->insertItem4( i18n("Sto&p loading"), this, "slotStop" , 0, MVIEW_STOP_ID, -1 );
    //TODO: view frame source, view document source, document encoding

  }
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
  // TODO
}

void KonqMainView::slotTerminal()
{
  // TODO
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
  // TODO
}

void KonqMainView::slotPrint()
{
  // TODO
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

void KonqMainView::slotSelect()
{
  // TODO
}

void KonqMainView::slotSelectAll()
{
  // TODO
}

void KonqMainView::slotSaveGeometry()
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

void KonqMainView::slotSplitView()
{
  // Create new view, same URL as current view, on its right.
  splitView( Konqueror::right );
}

void KonqMainView::slotRowAbove()
{
  // Create new row above, with a view, same URL as current view.
  splitView( Konqueror::above );
}

void KonqMainView::slotRowBelow()
{
  // Create new row below, with a view, same URL as current view.
  splitView( Konqueror::below );
}

void KonqMainView::slotRemoveView()
{
  removeView( m_currentId );
}

void KonqMainView::slotShowHTML()
{
  ///  m_currentView->changeViewMode( "KonquerorHTMLView" );
/*
  m_vMenuView->setItemChecked( MVIEW_LARGEICONS_ID, false );
  m_vMenuView->setItemChecked( MVIEW_SMALLICONS_ID, false );
  m_vMenuView->setItemChecked( MVIEW_TREEVIEW_ID, false );

  if ( !CORBA::is_nil( m_vMenuView ) )
    m_vMenuView->setItemChecked( MVIEW_HTMLVIEW_ID, !m_currentView->m_pView->isHTMLAllowed() );

  m_currentView->m_pView->setHTMLAllowed( !m_currentView->m_pView->isHTMLAllowed() );
*/
}

void KonqMainView::slotLargeIcons()
{
  m_currentView->changeViewMode( "KonquerorKfmIconView" );
  
  //this must never fail... 
  //(but it's quite sure that doesn't fail ;) 
  //(we could also ask via supportsInterface() ...anyway)
  Konqueror::KfmIconView_var iv = Konqueror::KfmIconView::_narrow( m_currentView->view() );
  
  iv->slotLargeIcons();
}

void KonqMainView::slotSmallIcons()
{
  m_currentView->changeViewMode( "KonquerorKfmIconView" );
  
  Konqueror::KfmIconView_var iv = Konqueror::KfmIconView::_narrow( m_currentView->view() );
  
  iv->slotSmallIcons();
}

void KonqMainView::slotTreeView()
{
  m_currentView->changeViewMode( "KonquerorKfmTreeView" );
}

void KonqMainView::slotReload()
{
  m_currentView->reload();
}

void KonqMainView::slotStop()
{
  if ( m_pRun )
  {
    delete m_pRun;
    m_pRun = 0L;
  }
  
  if ( m_currentView )
    m_currentView->stop();
}

void KonqMainView::slotUp()
{
  QString url = m_currentView->url();
  KURL u( url );
  u.cd(".."); // KURL does it for us
  
  m_currentView->openURL( u.url() );
}

void KonqMainView::slotBack()
{ 
  m_currentView->goBack();

  if( m_currentView->canGoBack() )
    setItemEnabled( m_vMenuGo, MGO_BACK_ID, false );
}

void KonqMainView::slotForward()
{
  m_currentView->goForward();
  if( m_currentView->canGoForward() )
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

  QString f = file.data();
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

void KonqMainView::slotConfigureKeys()
{
  KKeyDialog::configureKeys( m_pAccel );
}

void KonqMainView::slotAboutApp()
{
  kapp->invokeHTMLHelp( "kfm3/about.html", "" );
}

void KonqMainView::slotHelp()
{
}

void KonqMainView::slotURLEntered()
{
  CORBA::String_var _url = m_vLocationBar->linedText( TOOLBAR_URL_ID );
  QString url = _url.in();

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
    tmp += m_vLocationBar->linedText( TOOLBAR_URL_ID ) + 1;
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
    string tmp = i18n("Malformed URL\n").ascii();
    tmp += m_vLocationBar->linedText( TOOLBAR_URL_ID );
    QMessageBox::critical( (QWidget*)0L, i18n( "Error" ), tmp.c_str(), i18n( "OK" ) );
    return;
  }
	
  /*
    m_currentView->m_bBack = false;
    m_currentView->m_bForward = false;
    Why this ? Seems not necessary ... (David)
  */ 

  openURL( url, (CORBA::Boolean)false );
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
  if ( !url )
    return;

  MapViews::Iterator it = m_mapViews.find( id );
  
  assert( it != m_mapViews.end() );
  
  if ( id == m_currentId )
    slotStartAnimation();

  it.data()->makeHistory( false /* not completed */, url );
  if ( id == m_currentId )
  {
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

  setStatusBarText( i18n("Document: Done") );
}

void KonqMainView::slotPopupNewView()
{
  assert( m_lstPopupURLs.count() == 1 );
  KonqMainWindow *m_pShell = new KonqMainWindow( m_lstPopupURLs.getFirst() );
  m_pShell->show();
}

void KonqMainView::slotPopupEmptyTrashBin()
{
  //TODO
}

void KonqMainView::slotPopupCopy()
{
  // TODO (kclipboard.h will probably have to be ported to QStringList)
  //  KClipboard::self()->setURLList( m_lstPopupURLs ); 
}

void KonqMainView::slotPopupPaste()
{
  assert( m_lstPopupURLs.count() == 1 );
  pasteClipboard( m_lstPopupURLs.getFirst() );
}

void KonqMainView::slotPopupTrash()
{
  //TODO
}

void KonqMainView::slotPopupDelete()
{
  KIOJob *job = new KIOJob;
  list<string> lst;
  QStringList::Iterator it = m_lstPopupURLs.begin();
  for ( ; it != m_lstPopupURLs.end(); ++it )
    lst.push_back( it->data() );
  job->del( lst );
}

void KonqMainView::slotPopupOpenWith()
{
  OpenWithDlg l( i18n("Open With:"), "", this, true );
  if ( l.exec() )
  {
    KService *service = l.service();
    if ( service )
    {
      KfmRun::run( *service, m_lstPopupURLs );
      return;
    }
    else
    {
      QString exec = l.text();
      exec += " %f";
      KfmRun::runOldApplication( exec, m_lstPopupURLs, false );
    }
  }
}

void KonqMainView::slotPopupBookmarks()
{
  //TODO
}

void KonqMainView::slotPopup( int id )
{
  // Is it a usual service
  QMap<int,const KService *>::Iterator it = m_mapPopup.find( id );
  if ( it != m_mapPopup.end() )
  {
    KRun::run( *(it.data()), m_lstPopupURLs );
    return;
  }
  
  // Is it a service specific to kdelnk files ?
  QMap<int,KDELnkMimeType::Service>::Iterator it2 = m_mapPopup2.find( id );
  if ( it2 == m_mapPopup2.end() )
    return;

  QStringList::Iterator it3 = m_lstPopupURLs.begin();
  for( ; it3 != m_lstPopupURLs.end(); ++it3 )
    KDELnkMimeType::executeService( *it3, it2.data() );

  return;
}

void KonqMainView::slotPopupProperties()
{
  if ( m_lstPopupURLs.count() == 1 )
    (void) new PropertiesDialog( m_lstPopupURLs.getFirst(), m_popupMode );
  // else ERROR
}

void KonqMainView::resizeEvent( QResizeEvent *e )
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
  CORBA::String_var t = m_vMainWindow->partCaption( m_currentId );
  QString title = t.in();
  return title;
}
 
QString KonqMainView::currentURL()
{
  return m_currentView->url();
}

#include "konq_mainview.moc"
