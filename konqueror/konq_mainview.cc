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
#include "kfmguiprops.h"
#include "kfmpaths.h"
#include "kfmviewprops.h"
#include "kbookmark.h"
#include "konq_defaults.h"
#include "konq_mainwindow.h"
#include "konq_iconview.h"
#include "konq_htmlview.h"
#include "konq_treeview.h"

#include <opUIUtils.h>

#include <qmsgbox.h>
#include <qstring.h>
#include <qkeycode.h>

#include <kapp.h>
#include <kiconloader.h>
#include <k2url.h>
#include <kconfig.h>
#include <kmimemagic.h>
#include <kuserprofile.h>
#include <kregistry.h>
#include <kio_cache.h>
#include <kio_manager.h>
#include <kio_paste.h>
#include <kurlcompletion.h>
#include <kpixmapcache.h>
#include <kio_linedit_dlg.h>
#include <kio_openwith.h>
#include <kkeydialog.h>
#include <kstdaccel.h>
#include <kclipboard.h>

#include <iostream>
#include <assert.h>

enum _ids {
////// toolbar buttons //////
    TOOLBAR_UP_ID, TOOLBAR_BACK_ID, TOOLBAR_FORWARD_ID,
    TOOLBAR_HOME_ID, TOOLBAR_RELOAD_ID, TOOLBAR_COPY_ID, TOOLBAR_PASTE_ID,
    TOOLBAR_HELP_ID, TOOLBAR_STOP_ID, TOOLBAR_GEAR_ID,
/////  toolbar lineedit /////
    TOOLBAR_URL_ID,

////// menu items //////
    MFILE_NEW_ID, MFILE_NEWWINDOW_ID, MFILE_RUN_ID, MFILE_OPENTERMINAL_ID,
    MFILE_GOTO_ID, MFILE_CACHE_ID, MFILE_HISTORY_ID, MFILE_OPENLOCATION_ID,
    MFILE_FIND_ID, MFILE_PRINT_ID, MFILE_CLOSE_ID,

    MEDIT_COPY_ID, MEDIT_PASTE_ID, MEDIT_TRASH_ID, MEDIT_DELETE_ID, MEDIT_SELECT_ID,
    MEDIT_SELECTALL_ID, // MEDIT_FINDINPAGE_ID, MEDIT_FINDNEXT_ID,
    MEDIT_MIMETYPES_ID, MEDIT_APPLICATIONS_ID, // later, add global mimetypes and apps here
    MEDIT_SAVEGEOMETRY_ID,

    MVIEW_SPLITWINDOW_ID, MVIEW_SHOWDOT_ID, MVIEW_IMAGEPREVIEW_ID, MVIEW_LARGEICONS_ID,
    MVIEW_SMALLICONS_ID, MVIEW_TREEVIEW_ID, MVIEW_HTMLVIEW_ID, MVIEW_RELOADTREE_ID,
    MVIEW_RELOAD_ID // + view frame source, view document source, document encoding

    // clear cache is needed somewhere
    // MOPTIONS_...
};

QList<KonqMainView>* KonqMainView::s_lstWindows = 0L;
QList<OpenPartsUI::Pixmap>* KonqMainView::s_lstAnimatedLogo = 0L;

KonqMainView::KonqMainView( QWidget *_parent = 0L ) : QWidget( _parent )
{
  setWidget( this );

  OPPartIf::setFocusPolicy( OpenParts::Part::ClickFocus );

  m_vMenuFile = 0L;
  m_vMenuFileNew = 0L;
  m_vMenuEdit = 0L;
  m_vMenuView = 0L;
  m_vMenuBookmarks = 0L;
  m_vMenuOptions = 0L;

  m_vToolBar = 0L;
  m_vLocationBar = 0L;

  m_vStatusBar = 0L;

  m_pRun = 0L;
  
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
    s_lstAnimatedLogo = new QList<OpenPartsUI::Pixmap>;
  if ( !s_lstWindows )
    s_lstWindows = new QList<KonqMainView>;

  s_lstWindows->setAutoDelete( false );
  s_lstWindows->append( this );

//  m_pCompletion = 0L;

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

  m_bInit = false;
/*
  m_views.at(0)->m_pView->fetchFocus();
  m_views.at(0)->m_pView->openURL( m_strTmpURL.data() );

  if ( m_Props->isSplitView() )
  {
    m_views.at(1)->m_pView->openURL( m_strTmpURL.data() );
  }
*/
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
  
  map<OpenParts::Id,View*>::iterator it = m_mapViews.begin();
  for (; it != m_mapViews.end(); it++ )
      {
        it->second->m_vView->disconnectObject( this );
	it->second->m_pFrame->detach();
	it->second->m_vView = 0L;
        delete it->second->m_pFrame;
	delete it->second;
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

  OpenPartsUI::Menu_var m_vMenuFileGoto;

  m_vMenuFile->insertItem8( i18n("&Goto"), m_vMenuFileGoto, MFILE_GOTO_ID, -1 );

  m_vMenuFileGoto->insertItem4( i18n("&Cache"), this, "slotShowCache", 0, MFILE_CACHE_ID, -1 );
  m_vMenuFileGoto->insertItem4( i18n("&History"), this, "slotShowHistory", 0, MFILE_HISTORY_ID, -1 );

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
  // m_vMenuEdit->insertItem4( i18n("&Find in page..."), this, "slotFindInPage", Key_F2, MEDIT_FINDINPAGE_ID, -1 );
  // m_vMenuEdit->insertItem4( i18n("Find &next"), this, "slotFindNext", Key_F3, MEDIT_FINDNEXT_ID, -1 );
  // m_vMenuEdit->insertSeparator( -1 );
  m_vMenuEdit->insertItem4( i18n("Mime &Types"), this, "slotEditMimeTypes", 0, MEDIT_MIMETYPES_ID, -1 );
  m_vMenuEdit->insertItem4( i18n("App&lications"), this, "slotEditApplications", 0, MEDIT_APPLICATIONS_ID, -1 );
  //TODO: Global mime types and applications for root
  m_vMenuEdit->insertSeparator( -1 );
  m_vMenuEdit->insertItem4( i18n("Save &Geometry"), this, "slotSaveGeometry", 0, MEDIT_SAVEGEOMETRY_ID, -1 );

  menuBar->insertMenu( i18n("&View"), m_vMenuView, -1, -1 );  
  
  m_vMenuView->setCheckable( true );
  //  m_vMenuView->insertItem4( i18n("Show Directory Tr&ee"), this, "slotShowTree" , 0 );
  m_vMenuView->insertItem4( i18n("Split &window"), this, "slotSplitView" , 0, MVIEW_SPLITWINDOW_ID, -1 );
  m_vMenuView->insertItem4( i18n("Show &Dot Files"), this, "slotShowDot" , 0, MVIEW_SHOWDOT_ID, -1 );
  m_vMenuView->insertItem4( i18n("Image &Preview"), this, "slotShowSchnauzer" , 0, MVIEW_IMAGEPREVIEW_ID, -1 );
  //m_vMenuView->insertItem4( i18n("&Always Show index.html"), this, "slotShowHTML" , 0 );
  m_vMenuView->insertSeparator( -1 );
  m_vMenuView->insertItem4( i18n("&Large Icons"), this, "slotLargeIcons" , 0, MVIEW_LARGEICONS_ID, -1 );
  m_vMenuView->insertItem4( i18n("&Small Icons"), this, "slotSmallIcons" , 0, MVIEW_SMALLICONS_ID, -1 );
  m_vMenuView->insertItem4( i18n("&Tree View"), this, "slotTreeView" , 0, MVIEW_TREEVIEW_ID, -1 );
  m_vMenuView->insertSeparator( -1 );
  m_vMenuView->insertItem4( i18n("&Use HTML"), this, "slotHTMLView" , 0, MVIEW_HTMLVIEW_ID, -1 );
  m_vMenuView->insertSeparator( -1 );
  m_vMenuView->insertItem4( i18n("Rel&oad Tree"), this, "slotReloadTree" , 0, MVIEW_RELOADTREE_ID, -1 );
  m_vMenuView->insertItem4( i18n("&Reload Document"), this, "slotReload" , Key_F5, MVIEW_RELOAD_ID, -1 );
  //TODO: view frame source, view document source, document encoding

  menuBar->insertMenu( i18n("&Bookmarks"), m_vMenuBookmarks, -1, -1 );

//  m_pBookmarkMenu = new KBookmarkMenu( m_currentView->m_pView, m_vMenuBookmarks, this, true );

  menuBar->insertMenu( i18n("&Options"), m_vMenuOptions, -1, -1 );

  m_vMenuOptions->insertItem( i18n("Configure &keys"), this, "slotConfigureKeys", 0 );

  // disable menu items that need to be
//  slotGotFocus( m_currentView->m_pView );

  return true;
}

bool KonqMainView::mappingCreateToolbar( OpenPartsUI::ToolBarFactory_ptr factory )
{
  debug("KonqMainView::mappingCreateToolbar");
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
  m_vToolBar->insertButton2( pix, TOOLBAR_UP_ID, SIGNAL(clicked()),
                             this, "slotUp", false, i18n("Up"), -1);

  pix = OPUIUtils::convertPixmap( *KPixmapCache::toolbarPixmap( "back.xpm" ) );
  m_vToolBar->insertButton2( pix, TOOLBAR_BACK_ID, SIGNAL(clicked()),
                             this, "slotBack", false, i18n("Back"), -1);

  pix = OPUIUtils::convertPixmap( *KPixmapCache::toolbarPixmap( "forward.xpm" ) );
  m_vToolBar->insertButton2( pix, TOOLBAR_FORWARD_ID, SIGNAL(clicked()),
                             this, "slotForward", false, i18n("Forward"), -1);

  pix = OPUIUtils::convertPixmap( *KPixmapCache::toolbarPixmap( "home.xpm" ) );
  m_vToolBar->insertButton2( pix, TOOLBAR_HOME_ID, SIGNAL(clicked()),
                             this, "slotHome", true, i18n("Home"), -1);

  m_vToolBar->insertSeparator( -1 );

  pix = OPUIUtils::convertPixmap( *KPixmapCache::toolbarPixmap( "reload.xpm" ) );
  m_vToolBar->insertButton2( pix, TOOLBAR_RELOAD_ID, SIGNAL(clicked()),
                             this, "slotReload", true, i18n("Reload"), -1);

  m_vToolBar->insertSeparator( -1 );

  pix = OPUIUtils::convertPixmap( *KPixmapCache::toolbarPixmap( "editcopy.xpm" ) );
  m_vToolBar->insertButton2( pix, TOOLBAR_COPY_ID, SIGNAL(clicked()),
                             this, "slotCopy", true, i18n("Copy"), -1);

  pix = OPUIUtils::convertPixmap( *KPixmapCache::toolbarPixmap( "editpaste.xpm" ) );
  m_vToolBar->insertButton2( pix, TOOLBAR_PASTE_ID, SIGNAL(clicked()),
                             this, "slotPaste", true, i18n("Paste"), -1);
 				
  m_vToolBar->insertSeparator( -1 );				

  pix = OPUIUtils::convertPixmap( *KPixmapCache::toolbarPixmap( "help.xpm" ) );
  m_vToolBar->insertButton2( pix, TOOLBAR_HELP_ID, SIGNAL(clicked()),
                             this, "slotHelp", true, i18n("Stop"), -1);
				
  m_vToolBar->insertSeparator( -1 );				

  pix = OPUIUtils::convertPixmap( *KPixmapCache::toolbarPixmap( "stop.xpm" ) );
  m_vToolBar->insertButton2( pix, TOOLBAR_STOP_ID, SIGNAL(clicked()),
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

  //TODO: support completion in opToolBar->insertLined....
  //TODO: m_vLocationBar->setBarPos( convert_to_openpartsui_bar_pos( m_Props->m_locationBarPos ) ) );

  m_vLocationBar->enable( OpenPartsUI::Show );
  if ( !m_Props->m_bShowLocationBar )
    m_vLocationBar->enable( OpenPartsUI::Hide );

  debug("KonqMainView::mappingCreateToolbar : done !");
  return true;
}

bool KonqMainView::mappingChildGotFocus( OpenParts::Part_ptr child )
{
  cerr << "bool KonqMainView::mappingChildGotFocus( OpenParts::Part_ptr child )" << endl;
  
  setActiveView( child->id() );

  cerr << "current view is a " << m_currentView->m_vView->viewName() << endl;
  
  assert( m_currentView );

  if ( !CORBA::is_nil( m_vToolBar ) )
  {
//    m_vToolBar->setItemEnabled( TOOLBAR_UP_ID, hasUpURL() );
//    m_vToolBar->setItemEnabled( TOOLBAR_BACK_ID, hasBackHistory() );
//    m_vToolBar->setItemEnabled( TOOLBAR_FORWARD_ID, hasForwardHistory() );
  }

//  KfmViewProps * vProps = m_currentView->m_pView->props();
//  setViewModeMenu( vProps->viewMode() );
  setLocationBarURL( m_currentView->m_vView->url() );

  // m_vMenuView->setItemChecked( ..., vProps->isShowingDirTree() );
//  m_vMenuView->setItemChecked( MVIEW_SHOWDOT_ID, vProps->isShowingDotFiles() );
//  m_vMenuView->setItemChecked( MVIEW_IMAGEPREVIEW_ID, vProps->isShowingImagePreview() );
//  m_vMenuView->setItemChecked( MVIEW_HTMLVIEW_ID, vProps->isHTMLAllowed() );

  return true;
}

bool KonqMainView::mappingParentGotFocus( OpenParts::Part_ptr child )
{
  //hum....
  cerr << "void KonqMainView::mappingParentGotFocus( OpenParts::Part_ptr child )" << endl;
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
  Konqueror::View_var m_vView = Konqueror::View::_duplicate( view );

  m_vView->setMainWindow( m_vMainWindow );
  m_vView->setParent( this );

  View *v = new View;

  m_mapViews[ view->id() ] = v;
  v->m_vView = Konqueror::View::_duplicate( m_vView );

  if (newViewPosition == Konqueror::above || 
      newViewPosition == Konqueror::below)
  {
    fatal("TODO : create a new row and put the view in it");
  }
  else // left or right, in the current row
  {
    assert(m_pCurrentRow);
    int n = m_pCurrentRow->lstViews.count();
    debug("insertView : I already have %d views",n);

    v->m_pFrame = new OPFrame( m_pCurrentRow->pRowSplitter );

    if ((n>0) && newViewPosition == Konqueror::left) {
      m_pCurrentRow->lstViews.insert( 0, v );
      m_pCurrentRow->pRowSplitter->moveToFirst( v->m_pFrame );
    }
    else {
      m_pCurrentRow->lstViews.append( v );
    }
  }
  
  v->m_pFrame->attach( m_vView );
  v->m_pFrame->show();

  try
  {
    m_vView->connect("openURL", this, "openURL");
  }
  catch ( ... )
  {
    cerr << "WARNING: view does not know signal ""openURL"" " << endl;
  }
  try
  {
    m_vView->connect("started", this, "slotStartAnimation");
  }
  catch ( ... )
  {
    cerr << "WARNING: view does not know signal ""started"" " << endl;
  }
  try
  {
    m_vView->connect("completed", this, "slotStopAnimation");
  }
  catch ( ... )
  {
    cerr << "WARNING: view does not know signal ""completed"" " << endl;
  }
  try
  {
    m_vView->connect("setStatusBarText", this, "setStatusBarText");
  }
  catch ( ... )
  {
    cerr << "WARNING: view does not know signal ""setStatusBarText"" " << endl;
  }
  try
  {
    m_vView->connect("setLocationBarURL", this, "setLocationBarURL");
  }
  catch ( ... )
  {
    cerr << "WARNING: view does not know signal ""setLocationBarURL"" " << endl;
  }
  try
  {
    m_vView->connect("createNewWindow", this, "createNewWindow");
  }
  catch ( ... )
  {
    cerr << "WARNING: view does not know signal ""createNewWindow"" " << endl;
  }
  try
  {
    m_vView->connect("popupMenu", this, "popupMenu");
  }
  catch ( ... )
  {
    cerr << "WARNING: view does not know signal ""popupMenu"" " << endl;
  }
  try
  {
    m_vView->connect("addHistory", this, "addHistory");
  }
  catch ( ... )
  {
    cerr << "WARNING: view does not know signal ""addHistory"" " << endl;
  }

//  m_views.append( new View );

  // HACK : for the moment, use m_pPanner as parent for views. Will have to be extended
//  if (m_views.count() == 1) m_views.last()->m_pPannerChild = m_pPanner->child1();
//  if (m_views.count() == 2) m_views.last()->m_pPannerChild = m_pPanner->child0();
/*
  KfmView * v = new KfmView( this, m_views.last()->m_pPannerChild );
  m_views.last()->m_pView = v;
  v->show();
  v->fetchFocus();

  m_views.last()->m_pPannerChildGM = new QGridLayout( m_views.last()->m_pPannerChild, 1, 1 );
  m_views.last()->m_pPannerChildGM->addWidget( v, 0, 0 );

  QObject::connect( v, SIGNAL( canceled() ) ,
	   this, SLOT( slotStopAnimation() ) );
  QObject::connect( v, SIGNAL( completed() ) ,
	   this, SLOT( slotStopAnimation() ) );
  QObject::connect( v, SIGNAL( started() ) ,
	   this, SLOT( slotStartAnimation() ) );
  QObject::connect( v, SIGNAL( upURL() ),
	   this, SLOT( slotUp() ) );
  QObject::connect( v, SIGNAL( backHistory() ),
	   this, SLOT( slotBack() ) );
  QObject::connect( v, SIGNAL( forwardHistory() ),
	   this, SLOT( slotForward() ) );
  QObject::connect( v, SIGNAL( gotFocus( KfmView* ) ),
	   this, SLOT( slotGotFocus( KfmView* ) ) );
*/	
}

void KonqMainView::setActiveView( OpenParts::Id id )
{
  map<OpenParts::Id,View*>::iterator it = m_mapViews.find( id );

  m_currentView = it->second;
  // TODO update m_pCurrentRow here
}

Konqueror::View_ptr KonqMainView::activeView()
{
  if ( m_currentView )
    return Konqueror::View::_duplicate( m_currentView->m_vView );
  else
    return 0L;
}

Konqueror::ViewList *KonqMainView::viewList()
{
  Konqueror::ViewList *seq = new Konqueror::ViewList;
  int i = 0;
  seq->length( i );

  map<OpenParts::Id,View*>::iterator it = m_mapViews.begin();

  for (; it != m_mapViews.end(); it++ )
  {
    seq->length( i++ );
    (*seq)[ i ] = it->second->m_vView;
  }

  return seq;
}

void KonqMainView::removeView( OpenParts::Id id )
{
  map<OpenParts::Id,View*>::iterator it = m_mapViews.find( id );
  if ( it != m_mapViews.end() )
  {
    it->second->m_vView->disconnectObject( this );
    it->second->m_pFrame->detach();
    it->second->m_vView = 0L;
    delete it->second->m_pFrame;
    m_mapViews.erase( it );
  }
}

void KonqMainView::openURL( const Konqueror::URLRequest &url )
{
  openURL( url.url, url.reload );
}

void KonqMainView::openURL( const char * _url, CORBA::Boolean _reload )
{
  /////////// First, modify the URL if necessary (adding protocol, ...) //////
  string url = _url;

  // Root directory?
  if ( url[0] == '/' )
  {
    K2URL::encode( url );
  }
  // Home directory?
  else if ( url[0] == '~' )
  {
    QString tmp( QDir::homeDirPath() );
    tmp += _url + 1;
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
    string tmp = i18n("Malformed URL\n").ascii();
    tmp += _url;
    QMessageBox::critical( (QWidget*)0L, i18n( "KFM Error" ), tmp.c_str(), i18n( "OK" ) );
    return;
  }

  //FIXME... this is far for being complete (Simon)
  //(for example: obey the reload flag...)
  
  slotStop(); //hm....
    
  m_pRun = new KfmRun( this, url.c_str(), 0, false, false );
}

void KonqMainView::setStatusBarText( const char *_text )
{
  if ( !CORBA::is_nil( m_vStatusBar ) )
    m_vStatusBar->changeItem( _text, 1 );
}

void KonqMainView::setLocationBarURL( const char *_url )
{
  if ( !CORBA::is_nil( m_vLocationBar ) )
    m_vLocationBar->setLinedText( TOOLBAR_URL_ID, _url );
}

void KonqMainView::setUpURL( const char *_url )
{
  if ( _url == 0 )
    m_currentView->m_strUpURL = "";
  else
    m_currentView->m_strUpURL = _url;

  if ( CORBA::is_nil( m_vToolBar ) )
    return;

  if ( m_currentView->m_strUpURL.isEmpty() )
    m_vToolBar->setItemEnabled( TOOLBAR_UP_ID, false );
  else
    m_vToolBar->setItemEnabled( TOOLBAR_UP_ID, true );
}

void KonqMainView::addHistory( const char *_url, CORBA::Long _xoffset, CORBA::Long _yoffset )
{
  History h;
  h.m_strURL = _url;
  h.m_iXOffset = _xoffset;
  h.m_iYOffset = _yoffset;

  if ( m_bBack )
  {
    m_bBack = false;

    m_currentView->m_lstForward.push_front( h );
    if ( !CORBA::is_nil( m_vToolBar ) )
      m_vToolBar->setItemEnabled( TOOLBAR_FORWARD_ID, true );

    return;
  }

  if ( m_bForward )
  {
    m_bForward = false;

    m_currentView->m_lstBack.push_back( h );
    if ( !CORBA::is_nil( m_vToolBar ) )
      m_vToolBar->setItemEnabled( TOOLBAR_BACK_ID, true );

    return;
  }

  m_currentView->m_lstForward.clear();
  m_currentView->m_lstBack.push_back( h );

  if ( !CORBA::is_nil( m_vToolBar ) )
  {
    m_vToolBar->setItemEnabled( TOOLBAR_FORWARD_ID, false );
    m_vToolBar->setItemEnabled( TOOLBAR_BACK_ID, true );
  }
}

void KonqMainView::createNewWindow( const char *url )
{
  KonqMainWindow *m_pShell = new KonqMainWindow( url );
  m_pShell->show();
}

void KonqMainView::popupMenu( const Konqueror::View::MenuPopupRequest &popup )
{
  assert( popup.urls.length() >= 1 );

  QPopupMenu *m_popupMenu = new QPopupMenu;  
  bool bHttp          = true;
  bool isTrash        = true;
  bool currentDir     = false;
  bool isCurrentTrash = false;
  bool sReading       = true;
  bool sWriting       = true;
  bool sDeleting      = true;
  bool sMoving        = true;
  int id;
  KNewMenu *m_menuNew = new KNewMenu;

  ProtocolManager* pManager = ProtocolManager::self();
  
  K2URL url;
  CORBA::ULong i;
  // Check wether all URLs are correct
  for ( i = 0; i < popup.urls.length(); i++ )
  {
    url = K2URL( popup.urls[i] );
    const char* protocol = url.protocol();

    if ( url.isMalformed() )
    {
//FIXME?
//      emit error( ERR_MALFORMED_URL, s );
      return;
    }
    if (strcmp( protocol, "http" )) bHttp = false; // not HTTP

    // check if all urls are in the trash
    if ( isTrash )
    {
      QString path = url.path();
      if ( path.right(1) != "/" )
	path += "/";
    
      if ( strcmp( protocol, "file" ) != 0L ||
	   path != KfmPaths::trashPath() )
	isTrash = false;
    }

    if ( sReading )
      sReading = pManager->supportsReading( protocol );

    if ( sWriting )
      sWriting = pManager->supportsWriting( protocol );
    
    if ( sDeleting )
      sDeleting = pManager->supportsDeleting( protocol );

    if ( sMoving )
      sMoving = pManager->supportsMoving( protocol );
  }

  //check if current url is trash
  url = K2URL( m_currentView->m_vView->url() );
  QString path = url.path();
  if ( path.right(1) != "/" )
    path += "/";
    
  if ( strcmp( url.protocol(), "file" ) == 0L &&
       path == KfmPaths::trashPath() )
    isTrash = true;

  //check if url is current directory
  if ( popup.urls.length() == 1 )
    if ( strcmp( m_currentView->m_vView->url(), ((popup.urls)[0]) ) == 0 )
      currentDir = true;

  m_lstPopupURLs.setAutoDelete( true );
  m_lstPopupURLs.clear();
  for ( i = 0; i < popup.urls.length(); i++ )
    m_lstPopupURLs.append( (popup.urls)[i] );
      
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
    id = m_popupMenu->insertItem( i18n("&New"), m_menuNew->popupMenu() );
    m_popupMenu->insertSeparator();

    id = m_popupMenu->insertItem( *KPixmapCache::toolbarPixmap( "up.xpm" ), i18n( "Up" ), this, SLOT( slotUp() ), 100 );
    if ( !m_currentView->m_strUpURL.isEmpty() )
      m_popupMenu->setItemEnabled( id, false );

    id = m_popupMenu->insertItem( *KPixmapCache::toolbarPixmap( "back.xpm" ), i18n( "Back" ), this, SLOT( slotBack() ), 101 );
    if ( m_currentView->m_lstBack.size() == 0 )
      m_popupMenu->setItemEnabled( id, false );

    id = m_popupMenu->insertItem( *KPixmapCache::toolbarPixmap( "forward.xpm" ), i18n( "Forward" ), this, SLOT( slotForward() ), 102 );
    if ( m_currentView->m_lstForward.size() == 0 )
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

  m_menuNew->setPopupFiles( m_lstPopupURLs );

  // Do all URLs have the same mimetype ?
  url = K2URL( m_lstPopupURLs.first() );

  KMimeType* mime = KMimeType::findByURL( url, (mode_t)popup.mode, (bool)popup.isLocalFile );
  assert( mime );
  const char *s;
  for( s = m_lstPopupURLs.next(); mime != 0L && s != 0L; s = m_lstPopupURLs.next() )
  {
    K2URL u( s );  
    KMimeType* m = KMimeType::findByURL( u, (mode_t)popup.mode, (bool)popup.isLocalFile );
    if ( m != mime )
      mime = 0L;
  }
  
  if ( mime )
  {
    // Get all services for this mime type
    list<KService::Offer> offers;
    KService::findServiceByServiceType( mime->mimeType(), offers );

    list<KDELnkMimeType::Service> builtin;
    list<KDELnkMimeType::Service> user;
    if ( strcmp( mime->mimeType(), "application/x-kdelnk" ) == 0 )
    {
      KDELnkMimeType::builtinServices( url, builtin );
      KDELnkMimeType::userDefinedServices( url, user );
    }
  
    if ( !offers.empty() || !user.empty() || !builtin.empty() )
      QObject::connect( m_popupMenu, SIGNAL( activated( int ) ), this, SLOT( slotPopup( int ) ) );

    if ( !offers.empty() || !user.empty() )
      m_popupMenu->insertSeparator();
  
    m_mapPopup.clear();
    m_mapPopup2.clear();
  
    list<KService::Offer>::iterator it = offers.begin();
    for( ; it != offers.end(); it++ )
    {    
      id = m_popupMenu->insertItem( *(KPixmapCache::pixmap( it->m_pService->icon(), true ) ), it->m_pService->name() );
      m_mapPopup[ id ] = it->m_pService;
    }
    
    list<KDELnkMimeType::Service>::iterator it2 = user.begin();
    for( ; it2 != user.end(); ++it2 )
    {
      if ( !it2->m_strIcon.isEmpty() )
	id = m_popupMenu->insertItem( *(KPixmapCache::pixmap( it2->m_strIcon, true ) ), it2->m_strName );
      else
	id = m_popupMenu->insertItem( it2->m_strName );
      m_mapPopup2[ id ] = *it2;
    }
    
    if ( builtin.size() > 0 )
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
  
  m_popupMenu->exec( QPoint( popup.x, popup.y ) );
  
  delete m_popupMenu;
  delete m_menuNew;
}

void KonqMainView::openDirectory( const char *url )
{
  m_pRun = 0L;
  
  if ( strcmp( m_currentView->m_vView->viewName(), "KonquerorKfmIconView" ) != 0L )
  {
    Konqueror::View_var vView = Konqueror::View::_duplicate( new KonqKfmIconView );
    
    vView->setMainWindow( m_vMainWindow );
    vView->setParent( this );
    
    m_mapViews.erase( m_currentView->m_vView->id() );
    
    m_currentView->m_vView->disconnectObject( this );
    m_currentView->m_vView = Konqueror::View::_duplicate( vView );
    m_currentView->m_pFrame->attach( vView );
    m_currentView->m_pFrame->show();
    
  try
  {
    vView->connect("openURL", this, "openURL");
  }
  catch ( ... )
  {
    cerr << "WARNING: view does not know signal ""openURL"" " << endl;
  }
  try
  {
    vView->connect("started", this, "slotStartAnimation");
  }
  catch ( ... )
  {
    cerr << "WARNING: view does not know signal ""started"" " << endl;
  }
  try
  {
    vView->connect("completed", this, "slotStopAnimation");
  }
  catch ( ... )
  {
    cerr << "WARNING: view does not know signal ""completed"" " << endl;
  }
  try
  {
    vView->connect("setStatusBarText", this, "setStatusBarText");
  }
  catch ( ... )
  {
    cerr << "WARNING: view does not know signal ""setStatusBarText"" " << endl;
  }
  try
  {
    vView->connect("setLocationBarURL", this, "setLocationBarURL");
  }
  catch ( ... )
  {
    cerr << "WARNING: view does not know signal ""setLocationBarURL"" " << endl;
  }
  try
  {
    vView->connect("createNewWindow", this, "createNewWindow");
  }
  catch ( ... )
  {
    cerr << "WARNING: view does not know signal ""createNewWindow"" " << endl;
  }
  try
  {
    vView->connect("popupMenu", this, "popupMenu");
  }
  catch ( ... )
  {
    cerr << "WARNING: view does not know signal ""popupMenu"" " << endl;
  }
  try
  {
    vView->connect("addHistory", this, "addHistory");
  }
  catch ( ... )
  {
    cerr << "WARNING: view does not know signal ""addHistory"" " << endl;
  }
    m_mapViews[ vView->id() ] = m_currentView;
  }
  
  Konqueror::EventOpenURL eventURL;
  eventURL.url = CORBA::string_dup( url );
  eventURL.reload = (CORBA::Boolean)false;
  eventURL.xOffset = 0;
  eventURL.yOffset = 0;
  EMIT_EVENT( m_currentView->m_vView, Konqueror::eventOpenURL, eventURL );
}

void KonqMainView::openHTML( const char *url )
{
  m_pRun = 0L;
  
  if ( strcmp( m_currentView->m_vView->viewName(), "KonquerorHTMLView" ) != 0L )
  {
    Konqueror::View_var vView = Konqueror::View::_duplicate( new KonqHTMLView );

    vView->setMainWindow( m_vMainWindow );
    vView->setParent( this );
    
    m_mapViews.erase( m_currentView->m_vView->id() );

    m_currentView->m_vView->disconnectObject( this );
    m_currentView->m_vView = Konqueror::View::_duplicate( vView );
    m_currentView->m_pFrame->attach( vView );
    m_currentView->m_pFrame->show();

  try
  {
    vView->connect("openURL", this, "openURL");
  }
  catch ( ... )
  {
    cerr << "WARNING: view does not know signal ""openURL"" " << endl;
  }
  try
  {
    vView->connect("started", this, "slotStartAnimation");
  }
  catch ( ... )
  {
    cerr << "WARNING: view does not know signal ""started"" " << endl;
  }
  try
  {
    vView->connect("completed", this, "slotStopAnimation");
  }
  catch ( ... )
  {
    cerr << "WARNING: view does not know signal ""completed"" " << endl;
  }
  try
  {
    vView->connect("setStatusBarText", this, "setStatusBarText");
  }
  catch ( ... )
  {
    cerr << "WARNING: view does not know signal ""setStatusBarText"" " << endl;
  }
  try
  {
    vView->connect("setLocationBarURL", this, "setLocationBarURL");
  }
  catch ( ... )
  {
    cerr << "WARNING: view does not know signal ""setLocationBarURL"" " << endl;
  }
  try
  {
    vView->connect("createNewWindow", this, "createNewWindow");
  }
  catch ( ... )
  {
    cerr << "WARNING: view does not know signal ""createNewWindow"" " << endl;
  }
  try
  {
    vView->connect("popupMenu", this, "popupMenu");
  }
  catch ( ... )
  {
    cerr << "WARNING: view does not know signal ""popupMenu"" " << endl;
  }
  try
  {
    vView->connect("addHistory", this, "addHistory");
  }
  catch ( ... )
  {
    cerr << "WARNING: view does not know signal ""addHistory"" " << endl;
  }
        
    m_mapViews[ vView->id() ] = m_currentView;
  }

  Konqueror::EventOpenURL eventURL;
  eventURL.url = CORBA::string_dup( url );
  eventURL.reload = (CORBA::Boolean)false;
  eventURL.xOffset = 0;
  eventURL.yOffset = 0;
  EMIT_EVENT( m_currentView->m_vView, Konqueror::eventOpenURL, eventURL );
}

void KonqMainView::slotSplitView()
{
  // JUST FOR TESTING
  insertView( new KonqKfmIconView, Konqueror::right );
/*
  if ( m_Props->m_bSplitView )
  {
    m_currentView->m_pView->fetchFocus();

    m_Props->m_bSplitView = false;

    m_pPanner->setSeparator( 0 );

    View * todel = m_views.at(1);
    delete todel->m_pPannerChildGM;
    cerr << "!!!!!!!!! Layout done !!!!!!!!!" << endl;
    delete todel->m_pView;
    todel->m_lstBack.clear();
    todel->m_lstForward.clear();
    m_views.remove(1);
  } else
  {
    m_Props->m_bSplitView = true;

    QString url = m_views.at(0)->m_pView->currentURL();

    m_views.at(0)->m_pView->clearFocus();

    createView( );

    m_pPanner->setSeparator( 50 );

    cerr << "########## Opening " << url << endl;

    m_views.at(1)->m_pView->openURL( url );

  }
  if ( !CORBA::is_nil( m_vMenuView ) )
    m_vMenuView->setItemChecked( MVIEW_SPLITWINDOW_ID, m_Props->m_bSplitView );
*/
}

void KonqMainView::slotShowDot()
{
/*
  KfmView * v = m_currentView->m_pView;
  v->setShowDot( ! v->props()->isShowingDotFiles() );
  debug("v->props()->isShowingDotFiles() : %s",
        v->props()->isShowingDotFiles() ? "TRUE" : "FALSE");
  m_vMenuView->setItemChecked( MVIEW_SHOWDOT_ID, v->props()->isShowingDotFiles() );
*/
}

void KonqMainView::slotLargeIcons()
{
/*
  m_currentView->m_pView->setViewMode( KfmView::HOR_ICONS );
  setViewModeMenu( KfmView::HOR_ICONS );
*/
}

void KonqMainView::slotSmallIcons()
{
/*
  m_currentView->m_pView->setViewMode( KfmView::VERT_ICONS );
  setViewModeMenu( KfmView::VERT_ICONS );
*/
}

void KonqMainView::slotTreeView()
{
/*
  m_currentView->m_pView->setViewMode( KfmView::FINDER );
  setViewModeMenu( KfmView::FINDER );
*/
}

void KonqMainView::slotHTMLView()
{
/*
  m_vMenuView->setItemChecked( MVIEW_LARGEICONS_ID, false );
  m_vMenuView->setItemChecked( MVIEW_SMALLICONS_ID, false );
  m_vMenuView->setItemChecked( MVIEW_TREEVIEW_ID, false );

  if ( !CORBA::is_nil( m_vMenuView ) )
    m_vMenuView->setItemChecked( MVIEW_HTMLVIEW_ID, !m_currentView->m_pView->isHTMLAllowed() );

  m_currentView->m_pView->setHTMLAllowed( !m_currentView->m_pView->isHTMLAllowed() );
*/
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

void KonqMainView::slotShowCache()
{
  QString file = KIOCache::storeIndex();
  if ( file.isEmpty() )
  {
    QMessageBox::critical( 0L, i18n("KFM Error"), i18n( "Could not write index file" ), i18n( "OK" ) );
    return;
  }

  string f = file.data();
  K2URL::encode( f );
  openURL( f.c_str(), (CORBA::Boolean)false );
}

void KonqMainView::slotShowHistory()
{
  // TODO
}

void KonqMainView::slotOpenLocation()
{
//  QString url = m_currentView->m_pView->currentURL();
  QString url = m_currentView->m_vView->url();

  KLineEditDlg l( i18n("Open Location:"), url, this, true );
  int x = l.exec();
  if ( x )
  {
    QString url = l.text();
    url = url.stripWhiteSpace();
    // Exit if the user did not enter an URL
    if ( url.data()[0] == 0 )
      return;
    openURL( url, (CORBA::Boolean)false );
  }
}

void KonqMainView::slotConfigureKeys()
{
  KKeyDialog::configureKeys( m_pAccel );
}

void KonqMainView::slotAboutApp()
{
  kapp->invokeHTMLHelp( "kfm3/about.html", "" );
}

void KonqMainView::slotURLEntered()
{
  string url = m_vLocationBar->linedText( TOOLBAR_URL_ID );

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
    QString tmp( QDir::homeDirPath() );
    tmp += m_vLocationBar->linedText( TOOLBAR_URL_ID ) + 1;
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
    string tmp = i18n("Malformed URL\n").ascii();
    tmp += m_vLocationBar->linedText( TOOLBAR_URL_ID );
    QMessageBox::critical( (QWidget*)0L, i18n( "KFM Error" ), tmp.c_str(), i18n( "OK" ) );
    return;
  }
	
  m_bBack = false;
  m_bForward = false;

  openURL( url.c_str(), (CORBA::Boolean)false );
}

void KonqMainView::slotStop()
{
  if ( m_pRun )
  {
    delete m_pRun;
    m_pRun = 0L;
  }
  
  m_currentView->m_vView->stop();
}

void KonqMainView::slotNewWindow()
{
  QString url = m_currentView->m_vView->url();
  KonqMainWindow *m_pShell = new KonqMainWindow( url );
  m_pShell->show();
}

void KonqMainView::slotUp()
{
  assert( !m_currentView->m_strUpURL.isEmpty() );
//  m_currentView->m_pView->openURL( m_currentView->m_strUpURL );
  Konqueror::EventOpenURL eventURL;
  eventURL.url = m_currentView->m_strUpURL.data();
  eventURL.reload = (CORBA::Boolean)false;
  EMIT_EVENT( m_currentView->m_vView, Konqueror::eventOpenURL, eventURL );
}

void KonqMainView::slotHome()
{
  QString tmp( QDir::homeDirPath() );
//  m_currentView->m_pView->openURL( tmp );
  tmp.prepend( "file:" );
  openURL(tmp,(CORBA::Boolean)false); // might need a view-mode change...
}

void KonqMainView::slotBack()
{
  assert( m_currentView->m_lstBack.size() != 0 );
  // m_lstForward.push_front( m_currentHistory );
  History h = m_currentView->m_lstBack.back();
  m_currentView->m_lstBack.pop_back();

  if( m_currentView->m_lstBack.size() == 0 && ( !CORBA::is_nil( m_vToolBar ) ) )
    m_vToolBar->setItemEnabled( TOOLBAR_BACK_ID, false );
  // if( m_lstForward.size() != 0 )
  // m_pToolbar->setItemEnabled( TOOLBAR_FORWARD_ID, true );

  m_bBack = true;

//  m_currentView->m_pView->openURL( h.m_strURL, 0, false, h.m_iXOffset, h.m_iYOffset );
  //TODO
}

void KonqMainView::slotForward()
{
  assert( m_currentView->m_lstForward.size() != 0 );
  // m_lstBack.push_back( m_currentHistory() );
  History h = m_currentView->m_lstForward.front();
  m_currentView->m_lstForward.pop_front();

  // if( m_lstBack.size() != 0 )
  // m_pToolbar->setItemEnabled( TOOLBAR_BACK_ID, true );
  if( m_currentView->m_lstForward.size() == 0 && ( !CORBA::is_nil( m_vToolBar ) ) )
    m_vToolBar->setItemEnabled( TOOLBAR_FORWARD_ID, false );

  m_bForward = true;

//  m_currentView->m_pView->openURL( h.m_strURL, 0, false, h.m_iXOffset, h.m_iYOffset );
  //TODO
  //(extend the EventOpenURL structure!?) (Simon)
  // or emit event, then set offsets... (David)
}

void KonqMainView::slotReload()
{
  m_bForward = false;
  m_bBack = false;

//  m_currentView->m_pView->reload();
// TODO (trivial)
}

void KonqMainView::slotFileNewActivated( CORBA::Long id )
{
  if ( m_pMenuNew )
     {
       QStrList urls;
//       urls.append( m_currentView->m_pView->currentURL() );
       urls.append( m_currentView->m_vView->url() );

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

void KonqMainView::slotFocusLeftView()
{
//  slotGotFocus( m_views.at(0)->m_pView );
}

void KonqMainView::slotFocusRightView()
{
//  slotGotFocus( m_views.at(1)->m_pView );
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

  if ( !CORBA::is_nil( m_vToolBar ) )
    m_vToolBar->setItemEnabled( TOOLBAR_STOP_ID, true );
}

void KonqMainView::slotStopAnimation()
{
  m_animatedLogoTimer.stop();

  if ( !CORBA::is_nil( m_vToolBar ) )
  {
    m_vToolBar->setButtonPixmap( TOOLBAR_GEAR_ID, *( s_lstAnimatedLogo->at( 0 ) ) );
    m_vToolBar->setItemEnabled( TOOLBAR_STOP_ID, false );
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
  KClipboard::self()->setURLList( m_lstPopupURLs );
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
  const char *s;
  for ( s = m_lstPopupURLs.first(); s != 0L; s = m_lstPopupURLs.next() )
    lst.push_back( s );
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
      KfmRun::run( service, m_lstPopupURLs );
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
  map<int,KService*>::iterator it = m_mapPopup.find( id );
  if ( it != m_mapPopup.end() )
  {
    KRun::run( it->second, m_lstPopupURLs );
    return;
  }
  
  // Is it a service specific to kdelnk files ?
  map<int,KDELnkMimeType::Service>::iterator it2 = m_mapPopup2.find( id );
  if ( it2 == m_mapPopup2.end() )
    return;
  
  const char *u;
  for( u = m_lstPopupURLs.first(); u != 0L; u = m_lstPopupURLs.next() )
    KDELnkMimeType::executeService( u, it2->second );

  return;
}

void KonqMainView::slotPopupProperties()
{
  //TODO
}

void KonqMainView::resizeEvent( QResizeEvent *e )
{
//  m_pPanner->setGeometry( 0, 0, width(), height() );
}

KonqMainView::View::View()
{
  m_vView = 0L;
  m_pFrame = 0L;
}

void KonqMainView::initConfig()
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
      s_lstAnimatedLogo->append( OPUIUtils::convertPixmap( ICON(n) ) );
    }
  }			

  m_animatedLogoCounter = 0;
  QObject::connect( &m_animatedLogoTimer, SIGNAL( timeout() ), this, SLOT( slotAnimatedLogoTimeout() ) );
}

KonqMainView::Row * KonqMainView::newRow()
{
  Row * row = new Row;
  row->pRowSplitter = new QSplitter ( QSplitter::Horizontal, m_pMainSplitter );
  //row->pRowSplitter->setOpaqueResize( TRUE );
  m_lstRows.insert( 0 /* HACK */, row );
  debug("newRow() done");
  return row;
}

void KonqMainView::initPanner()
{
/*  if ( m_Props->isShowingDirTree() )
    m_pPanner = new KPanner( this, "_panner", KPanner::O_VERTICAL, 30 );
  else if ( m_Props->isSplitView() )
    m_pPanner = new KPanner( this, "_panner", KPanner::O_VERTICAL, 50 );
  else
    m_pPanner = new KPanner( this, "_panner", KPanner::O_VERTICAL, 0 );

  QObject::connect( m_pPanner, SIGNAL( positionChanged() ), this, SLOT( slotPannerChanged() ) );
*/
//setView( m_pPanner );

  // Create the main splitter
  m_pMainSplitter = new QSplitter ( QSplitter::Vertical, this, "mainsplitter" );
  QGridLayout * GM = new QGridLayout( this, 1, 1 );
  GM->addWidget( m_pMainSplitter, 0, 0 );

  //m_pMainSplitter->setOpaqueResize( TRUE ); 
  // Create a row, and its splitter
  m_lstRows.clear();
  Row * row = newRow();
  m_pCurrentRow = row;
  m_pMainSplitter->show();
  debug("initpanner() done");
}

void KonqMainView::initView()
{
  Konqueror::View_var vView = Konqueror::View::_duplicate( new KonqKfmIconView );

  insertView( vView, Konqueror::left );
  setActiveView( vView->id() );


  vView = Konqueror::View::_duplicate( new KonqKfmTreeView );
  insertView( vView, Konqueror::left );

  //temporary...
  vView->show( true );
  Konqueror::EventOpenURL eventURL;
  eventURL.url = CORBA::string_dup( QDir::homeDirPath().data() );
  eventURL.reload = (CORBA::Boolean)false;
  eventURL.xOffset = 0;
  eventURL.yOffset = 0;

  EMIT_EVENT( vView, Konqueror::eventOpenURL, eventURL );
//  m_pPanner->setSeparator( 50 );

}
/*
void KonqMainView::setViewModeMenu( KfmView::ViewMode _viewMode )
{
  assert( !CORBA::is_nil( m_vMenuView ) );

  switch( _viewMode )
  {
  case KfmView::HOR_ICONS:
    m_vMenuView->setItemChecked( MVIEW_LARGEICONS_ID, true );
    m_vMenuView->setItemChecked( MVIEW_SMALLICONS_ID, false );
    m_vMenuView->setItemChecked( MVIEW_TREEVIEW_ID, false );
    break;
  case KfmView::VERT_ICONS:
    m_vMenuView->setItemChecked( MVIEW_LARGEICONS_ID, false );
    m_vMenuView->setItemChecked( MVIEW_SMALLICONS_ID, true );
    m_vMenuView->setItemChecked( MVIEW_TREEVIEW_ID, false );
    break;
  case KfmView::FINDER:
    m_vMenuView->setItemChecked( MVIEW_LARGEICONS_ID, false );
    m_vMenuView->setItemChecked( MVIEW_SMALLICONS_ID, false );
    m_vMenuView->setItemChecked( MVIEW_TREEVIEW_ID, true );
    break;
  default:
    assert( 0 );
  }
  // m_pViewMenu->setItemChecked( MVIEW_HTMLVIEW_ID, false );
}
*/
/******************************************************
 *
 * Global functions
 *
 ******************************************************/

void openFileManagerWindow( const char *_url )
{
  KonqMainWindow *m_pShell = new KonqMainWindow( _url );
  m_pShell->show();
}

#include "konq_mainview.moc"
