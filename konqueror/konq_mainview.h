/*  This file is part of the KDE project
    Copyright (C) 1998, 1999 Torben Weis <weis@kde.org>
 
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
 
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
 
    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/     

#ifndef __konq_mainview_h__
#define __konq_mainview_h__

#include "konqueror.h"

#include <opPart.h>

#include <qmap.h>
#include <qstring.h>
#include <qtimer.h>
#include <qwidget.h>

#include <kmimetypes.h>
#include <kbookmarkmenu.h>

class KAccel;
class KNewMenu;
class KService;
class KURLCompletion;
class KonqPropsMainView;
class KfmRun;
class KonqChildView;
class QSplitter;
class KonqViewManager;

class KonqMainView : public QWidget,
                     virtual public OPPartIf,
		     virtual public Konqueror::MainView_skel,
                     virtual public KBookmarkOwner
{
  Q_OBJECT
public:
  //C++
  KonqMainView( const char *url = 0L, QWidget *parent = 0L );
  ~KonqMainView();

  //inherited
  virtual void init();
  virtual void cleanUp();

  virtual bool event( const char* event, const CORBA::Any& value );
  bool mappingCreateMenubar( OpenPartsUI::MenuBar_ptr menuBar );
  bool mappingCreateToolbar( OpenPartsUI::ToolBarFactory_ptr factory );
  bool mappingChildGotFocus( OpenParts::Part_ptr child );
  bool mappingParentGotFocus( OpenParts::Part_ptr child );
  bool mappingOpenURL( Browser::EventOpenURL eventURL );
  bool mappingNewTransfer( Browser::EventNewTransfer transfer );

  //IDL
  virtual void setActiveView( OpenParts::Id id );
  virtual Browser::View_ptr activeView();
  virtual OpenParts::Id activeViewId();
  virtual Browser::ViewList *viewList();

  virtual void openURL( const Browser::URLRequest &_urlreq );
  virtual void openURL( const char * _url, bool _reload = false, int xOffset = 0,
                        int yOffset = 0, KonqChildView *_view = 0L );
  
  virtual void setStatusBarText( const CORBA::WChar *_text );
  virtual void setLocationBarURL( OpenParts::Id id, const char *_url );
  
  virtual void createNewWindow( const char *url );

  void popupMenu( const QPoint &_global, const QStringList &_urls, mode_t _mode );
  
  bool openView( const QString &serviceType, const QString &url, KonqChildView *childView );

  KonqChildView *childView( OpenParts::Id id );
  KonqChildView *currentChildView() { return m_currentView; }
  void insertChildView( KonqChildView *view );
  void removeChildView( OpenParts::Id id );

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

  void saveProperties( KConfig *config );
  void readProperties( KConfig *config );  

public slots:  // IDL
  // File menu
  virtual void slotNewWindow();
  virtual void slotRun();
  virtual void slotTerminal();
  virtual void slotOpenLocation();
  virtual void slotToolFind();
  virtual void slotPrint();

  // Edit menu
  virtual void slotCopy();
  virtual void slotPaste();
  virtual void slotTrash();
  virtual void slotDelete();

  // View menu
  virtual void slotSplitViewHorizontal();
  virtual void slotSplitViewVertical();
  virtual void slotRemoveView();
  virtual void slotShowHTML();
  virtual void slotLargeIcons();
  virtual void slotSmallIcons();
  virtual void slotTreeView();
  virtual void slotReload();
  virtual void slotStop();
  
  void slotStop2();

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
  virtual void slotShowMenubar();
  virtual void slotShowStatusbar();
  virtual void slotShowToolbar();
  virtual void slotShowLocationbar();
  virtual void slotSaveSettings();
  virtual void slotSaveLocalSettings();
  // TODO : cache submenu
  virtual void slotConfigureFileManager();
  virtual void slotConfigureBrowser();
  virtual void slotConfigureKeys();
  virtual void slotReloadPlugins();
  virtual void slotSaveDefaultProfile();
  virtual void slotSaveViewProfile();
  virtual void slotViewProfileActivated( CORBA::Long id );

  // Help menu
  virtual void slotHelpContents();    
  virtual void slotHelpAbout();

  /////////////////////////
  // Location Bar
  /////////////////////////
  virtual void slotURLEntered( const CORBA::WChar *_url );
  
  /////////////////////////
  
  virtual void slotFileNewActivated( CORBA::Long id );
  virtual void slotFileNewAboutToShow();
  
  virtual void slotMenuEditAboutToShow();
  virtual void slotMenuViewAboutToShow();

  void fillHistoryPopup( OpenPartsUI::Menu_ptr menu, const QStringList &urls );

  virtual void slotHistoryBackwardAboutToShow();
  virtual void slotHistoryForwardAboutToShow();

  virtual void slotHistoryBackwardActivated( CORBA::Long id );
  virtual void slotHistoryForwardActivated( CORBA::Long id );
  
  virtual void slotBookmarkSelected( CORBA::Long id );
  virtual void slotBookmarkHighlighted( CORBA::Long id );
  virtual void slotEditBookmarks();  
  
  virtual void slotURLStarted( OpenParts::Id id, const char *url );
  virtual void slotURLCompleted( OpenParts::Id id );
  
  virtual void slotUpAboutToShow();
  virtual void slotUpActivated( CORBA::Long id );
  
  /////////////////////////
  // Animated Logo
  /////////////////////////
  void slotAnimatedLogoTimeout();
  void slotStartAnimation();
  void slotStopAnimation();

  //
  // internal ;-)
  //  
  void slotIdChanged( KonqChildView * childView, OpenParts::Id oldId, OpenParts::Id newId );

  void slotSelectView1();
  void slotSelectView2();
  void slotSelectView3();
  void slotSelectView4();
  void slotSelectView5();
  void slotSelectView6();
  void slotSelectView7();
  void slotSelectView8();
  void slotSelectView9();
  void slotSelectView10();

protected:

  virtual void resizeEvent( QResizeEvent *e );

private:

  void initConfig();
  void initGui();

  void checkPrintingExtension();

  /**
   * Splits the view, depending on orientation, either horizontally or 
   * vertically. The first of the resulting views will contain the initial 
   * view, the other will be a new one with the same URL and the same view type
   */
  void splitView( Orientation orientation );

  /**
   * Enable menu item and related toolbar button if present
   * This will allow configurable toolbar buttons later
   * @param menu the menu hosting the menu item
   * @param id the item id (both for menu and toolbar)
   * @param enable whether to enable or disable it
   */
  void setItemEnabled( OpenPartsUI::Menu_ptr menu, int id, bool enable );

  /**
   * Enable or disable the "up" button (and the menu item)
   * @param _url the URL shown in the view - the button state depends on it
   * @param _id the view id - if not currentId, the call has no effect
   */
  void setUpEnabled( QString _url, OpenParts::Id _id );

  void clearViewGUIElements( KonqChildView *childView );

  void createViewToolBar( KonqChildView *childView );

  /**
   * Fills the view menu with the entries of the mainview, merged with the
   * ones of the current view.
   */
  void createViewMenu();

  /**
   * Fills the edit menu with the entries of the mainview, merged with the
   * ones of the current view.
   */
  void createEditMenu();

  /**
   * Tries to find a index.html (.kde.html) file in the specified directory
   */
  QString findIndexFile( const QString &directory );

  void fillProfileMenu();
  
  QStringList locationBarCombo();
  void setLocationBarCombo( const QStringList &lst );
  
  OpenPartsUI::Menu_var m_vMenuFile;
  OpenPartsUI::Menu_var m_vMenuFileNew;
  OpenPartsUI::Menu_var m_vMenuEdit;
  OpenPartsUI::Menu_var m_vMenuView;
  OpenPartsUI::Menu_var m_vMenuGo;
  OpenPartsUI::Menu_var m_vMenuBookmarks;
  OpenPartsUI::Menu_var m_vMenuOptions;
  OpenPartsUI::Menu_var m_vMenuOptionsProfiles;
  OpenPartsUI::Menu_var m_vMenuHelp;

  OpenPartsUI::ToolBar_var m_vToolBar;
  OpenPartsUI::ToolBar_var m_vLocationBar;
  OpenPartsUI::MenuBar_var m_vMenuBar;
  OpenPartsUI::StatusBar_var m_vStatusBar;

  OpenPartsUI::Menu_var m_vHistoryBackPopupMenu;
  OpenPartsUI::Menu_var m_vHistoryForwardPopupMenu;

  OpenPartsUI::Menu_var m_vUpPopupMenu;

  KBookmarkMenu* m_pBookmarkMenu;

  //////// View storage //////////////

  /* Storage of View * instances : mapped by Id */
  typedef QMap<OpenParts::Id,KonqChildView *> MapViews;
  MapViews m_mapViews;

  KonqChildView *m_currentView;
  OpenParts::Id m_currentId;

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

  KonqPropsMainView *m_Props;

  KfmRun *m_pRun;

  QString m_sInitialURL;
  
  CORBA::Long m_lToolBarViewStartIndex;

  bool m_bEditMenuDirty;
  bool m_bViewMenuDirty;

  KonqViewManager *m_pViewManager;

  QStringList m_lstLocationBarCombo;

  static QList<OpenPartsUI::Pixmap>* s_lstAnimatedLogo;
  static QList<KonqMainView>* s_lstWindows;
};

#endif
