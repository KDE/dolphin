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

#ifndef __konq_mainview_h__
#define __konq_mainview_h__

#include "kbookmarkmenu.h"
#include "konqueror.h"

#include <opPart.h>
#include <openparts_ui.h>

#include <qstring.h>
#include <qtimer.h>
#include <qwidget.h>

#include <kmimetypes.h>

#include <map>

class KAccel;
class KBookmarkMenu;
class KNewMenu;
class KService;
class KURLCompletion;
class KfmGuiProps;
class KfmRun;
class KonqChildView;

class QSplitter;

typedef QSplitter Row;

class KonqMainView : public QWidget,
                     virtual public OPPartIf,
		     virtual public Konqueror::MainView_skel,
                     virtual public KBookmarkOwner
{
  Q_OBJECT
public:
  //C++
  KonqMainView( const char *url = 0L, QWidget *_parent = 0L );
  ~KonqMainView();

  //inherited
  virtual void init();
  virtual void cleanUp();

  virtual bool event( const char* event, const CORBA::Any& value );
  bool mappingCreateMenubar( OpenPartsUI::MenuBar_ptr menuBar );
  bool mappingCreateToolbar( OpenPartsUI::ToolBarFactory_ptr factory );
  bool mappingChildGotFocus( OpenParts::Part_ptr child );
  bool mappingParentGotFocus( OpenParts::Part_ptr child );
  bool mappingOpenURL( Konqueror::EventOpenURL eventURL );

  //IDL
  // Position is relative to activeView(); above and below create a new row
  virtual void insertView( Konqueror::View_ptr view, Konqueror::NewViewPosition newViewPosition);
  virtual void setActiveView( OpenParts::Id id );
  virtual Konqueror::View_ptr activeView();
  virtual Konqueror::ViewList *viewList();
  virtual void removeView( OpenParts::Id id );

  virtual void openURL( const Konqueror::URLRequest &url );
  virtual void openURL( const char * _url, CORBA::Boolean _reload );
  
  virtual void setStatusBarText( const char *_text );
  virtual void setLocationBarURL( OpenParts::Id id, const char *_url );
  
  virtual void createNewWindow( const char *url );
  virtual void popupMenu( const Konqueror::View::MenuPopupRequest &popup );

  void openDirectory( const char *url );
  void openHTML( const char *url );
  void openPluginView( const char *url, Konqueror::View_ptr view );
  void openText( const char *url );
  
  /**
   * Enable or disable the "up" button (and the menu item)
   * @param _url the URL shown in the view - the button state depends on it
   * @param _id the view id - if not currentId, the call has no effect
   */
  void setUpEnabled( const char * _url, OpenParts::Id _id );

  ////////////////////
  /// Overloaded functions of KBookmarkOwner
  ////////////////////
  /**
   * This function is called if the user selectes a bookmark. 
   */
  virtual void openBookmarkURL( const char *_url );
  /**
   * @return the title of the current page. This is called if the user wants
   *         to add the current page to the bookmarks.
   */
  virtual QString currentTitle();
  /**
   * @return the URL of the current page. This is called if the user wants
   *         to add the current page to the bookmarks.
   */
  virtual QString currentURL();
  

public slots:  
  // File menu
  virtual void slotNewWindow();
  virtual void slotRun();
  virtual void slotTerminal();
  virtual void slotOpenLocation();
  virtual void slotToolFind();
  virtual void slotPrint();
  virtual void slotClose();

  // Edit menu
  virtual void slotCopy();
  virtual void slotPaste();
  virtual void slotTrash();
  virtual void slotDelete();
  virtual void slotSelect();
  virtual void slotSelectAll();
  virtual void slotSaveGeometry();

  // View menu
  virtual void slotSplitView();
  virtual void slotRowAbove();
  virtual void slotRowBelow();
  virtual void slotRemoveView();
  virtual void slotShowHTML();
  virtual void slotLargeIcons();
  virtual void slotSmallIcons();
  virtual void slotTreeView();
  virtual void slotReload();
  virtual void slotStop();

  // Go menu
  virtual void slotUp();
  virtual void slotBack();
  virtual void slotForward();
  virtual void slotHome();
  virtual void slotShowCache();
  virtual void slotShowHistory();
  virtual void slotEditMimeTypes();
  virtual void slotEditApplications();

  // Options menu
  virtual void slotConfigureKeys();

  // Help menu
  virtual void slotAboutApp();    
  virtual void slotHelp();

  /////////////////////////
  // Location Bar
  /////////////////////////
  virtual void slotURLEntered();
  
  /////////////////////////
  
  virtual void slotFileNewActivated( CORBA::Long id );
  virtual void slotFileNewAboutToShow();
  
  virtual void slotBookmarkSelected( CORBA::Long id );
  virtual void slotEditBookmarks();  
  
  virtual void slotURLStarted( OpenParts::Id id, const char *url );
  virtual void slotURLCompleted( OpenParts::Id id );
  
  /////////////////////////
  // Animated Logo
  /////////////////////////
  void slotAnimatedLogoTimeout();
  void slotStartAnimation();
  void slotStopAnimation();
  
  // Popup
  void slotPopupNewView();
  void slotPopupEmptyTrashBin();
  void slotPopupCopy();
  void slotPopupPaste();
  void slotPopupTrash();
  void slotPopupDelete();
  void slotPopupOpenWith();
  void slotPopupBookmarks();
  void slotPopup( int id );
  void slotPopupProperties();

  void slotIdChanged( KonqChildView * childView, OpenParts::Id oldId, OpenParts::Id newId );
  void slotSetUpEnabled( QString url, OpenParts::Id id );
  
protected:

  ///////////// protected methods ///////////

  virtual void resizeEvent( QResizeEvent *e );

  void initConfig();
  void initGui();
  void initPanner();
  void initView();

  /**
   * Create a new view from the current view (same URL, same view type) 
   */
  void splitView ( Konqueror::NewViewPosition newViewPosition );

 void changeViewMode( const char *viewName ) ; 
  /**
   * Enable menu item and related toolbar button if present
   * This will allow configurable toolbar buttons later
   * @param menu the menu hosting the menu item
   * @param id the item id (both for menu and toolbar)
   * @param enable whether to enable or disable it
   */
  void setItemEnabled( OpenPartsUI::Menu_ptr menu, int id, bool enable );

  void makeHistory( KonqChildView *v );

  void createViewMenu();
  
  ///////// protected members //////////////

  OpenPartsUI::Menu_var m_vMenuFile;
  OpenPartsUI::Menu_var m_vMenuFileNew;
  OpenPartsUI::Menu_var m_vMenuEdit;
  OpenPartsUI::Menu_var m_vMenuView;
  OpenPartsUI::Menu_var m_vMenuGo;
  OpenPartsUI::Menu_var m_vMenuBookmarks;
  OpenPartsUI::Menu_var m_vMenuOptions;

  OpenPartsUI::ToolBar_var m_vToolBar;
  OpenPartsUI::ToolBar_var m_vLocationBar;
  
  OpenPartsUI::StatusBar_var m_vStatusBar;

  KBookmarkMenu* m_pBookmarkMenu;

  //////// View storage //////////////

  /* The list of rows */
  QList<Row> m_lstRows;
  /* The main, vertical, QSplitter, which holds the rows */
  QSplitter* m_pMainSplitter;

  /* Storage of View * instances : mapped by Id */
  map<OpenParts::Id,KonqChildView*> m_mapViews;

  KonqChildView *m_currentView;
  OpenParts::Id m_currentId;
  // current row is currentView->row, no need for a member

  Row * newRow( bool append );

  ////////////////////
    
  /**
   * The menu "New" in the "File" menu.
   * Since the items of this menu are not connected themselves
   * we need a pointer to this menu to get information about the
   * selected menu item.
   */
  KNewMenu *m_pMenuNew;

  /**
   * Set to true while the constructor is running.
   * @ref #initConfig needs to know about that.
   */
  bool m_bInit;

  unsigned int m_animatedLogoCounter;
  QTimer m_animatedLogoTimer;

  KAccel* m_pAccel;

  KfmGuiProps *m_Props;

  QStrList m_lstPopupURLs;
  CORBA::Long m_popupMode; // mode_t of first URL in m_lstPopupURLs
  map<int,KService*> m_mapPopup;
  map<int,KDELnkMimeType::Service> m_mapPopup2;

  KfmRun *m_pRun;

  QString m_sInitialURL;

  static QList<OpenPartsUI::Pixmap>* s_lstAnimatedLogo;
  static QList<KonqMainView>* s_lstWindows;
};

#endif
