/* -*- c-basic-offset:2 -*-
   This file is part of the KDE project
   Copyright (C) 1998, 1999 Simon Hausmann <hausmann@kde.org>
   Copyright (C) 2000-2004 David Faure <faure@kde.org>

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
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef __konq_mainwindow_h__
#define __konq_mainwindow_h__

#include <qmap.h>
#include <qpoint.h>
#include <qtimer.h>
#include <qpointer.h>
//Added by qt3to4:
#include <QPixmap>
#include <Q3CString>
#include <QCloseEvent>
#include <Q3PtrList>
#include <QEvent>
#include <QLabel>
#include <QCustomEvent>

#include <kfileitem.h>
#include "konq_openurlrequest.h"

#include <kparts/mainwindow.h>
#include <kbookmarkmanager.h>
#include <kcompletion.h>
#include <kurlcompletion.h>
#include <kglobalsettings.h>
#include <dcopobject.h>
#include <kxmlguifactory.h>
#include <kxmlguiclient.h>
#include <ktrader.h>
#include "konq_combo.h"
#include "konq_frame.h"

class QFile;
class KAction;
class KActionCollection;
class KActionMenu;
class KBookmarkMenu;
class KCMultiDialog;
class KHistoryCombo;
class KNewMenu;
class KProgress;
class KSelectAction;
class KToggleAction;
class KonqBidiHistoryAction;
class KBookmarkBar;
class KonqView;
class KonqComboAction;
class KonqFrame;
class KonqFrameBase;
class KonqFrameContainerBase;
class KonqFrameContainer;
class KToolBarPopupAction;
class KonqLogoAction;
class KonqViewModeAction;
class KonqPart;
class KonqViewManager;
class OpenWithGUIClient;
class ToggleViewGUIClient;
class ViewModeGUIClient;
class KonqMainWindowIface;
class KonqDirPart;
class KonqRun;
class KURLRequester;
class KZip;
struct HistoryEntry;

namespace KParts {
 class BrowserExtension;
 class BrowserHostExtension;
 class ReadOnlyPart;
 struct URLArgs;
}

class KonqExtendedBookmarkOwner;

class KonqMainWindow : public KParts::MainWindow, public KonqFrameContainerBase
{
  Q_OBJECT
  Q_PROPERTY( int viewCount READ viewCount )
  Q_PROPERTY( int activeViewsCount READ activeViewsCount )
  Q_PROPERTY( int linkableViewsCount READ linkableViewsCount )
  Q_PROPERTY( QString locationBarURL READ locationBarURL )
  Q_PROPERTY( bool fullScreenMode READ fullScreenMode )
  Q_PROPERTY( QString currentTitle READ currentTitle )
  Q_PROPERTY( QString currentURL READ currentURL )
  Q_PROPERTY( bool isHTMLAllowed READ isHTMLAllowed )
  Q_PROPERTY( QString currentProfile READ currentProfile )
public:
  enum ComboAction { ComboClear, ComboAdd, ComboRemove };
  enum PageSecurity { NotCrypted, Encrypted, Mixed };

  KonqMainWindow( const KURL &initialURL = KURL(), bool openInitialURL = true, const char *name = 0, const QString& xmluiFile="konqueror.rc");
  ~KonqMainWindow();


  /**
   * Filters the URL and calls the main openURL method.
   */
  void openFilteredURL( const QString & _url, KonqOpenURLRequest& _req);

  /**
   * Filters the URL and calls the main openURL method.
   */
  void openFilteredURL( const QString &_url, bool inNewTab = false, bool tempFile = false );

  /**
   * The main openURL method.
   */
  void openURL( KonqView * view, const KURL & url,
                const QString &serviceType = QString::null,
                KonqOpenURLRequest & req = KonqOpenURLRequest::null, bool trustedSource = false );

  /**
   * Called by openURL when it knows the service type (either directly,
   * or using KonqRun)
   */
  bool openView( QString serviceType, const KURL &_url, KonqView *childView,
                 KonqOpenURLRequest & req = KonqOpenURLRequest::null );


  void abortLoading();

    void openMultiURL( KURL::List url );

  KonqViewManager *viewManager() const { return m_pViewManager; }

  // Central widget of the mainwindow, never 0L
  QWidget *mainWidget() const;

  virtual QWidget *createContainer( QWidget *parent, int index, const QDomElement &element, int &id );
  virtual void removeContainer( QWidget *container, QWidget *parent, QDomElement &element, int id );

  virtual void saveProperties( KConfig *config );
  virtual void readProperties( KConfig *config );

  void setInitialFrameName( const QString &name );

  KonqMainWindowIface * dcopObject();

  void reparseConfiguration();

  void insertChildView( KonqView *childView );
  void removeChildView( KonqView *childView );
  KonqView *childView( KParts::ReadOnlyPart *view );
  KonqView *childView( KParts::ReadOnlyPart *callingPart, const QString &name, KParts::BrowserHostExtension **hostExtension, KParts::ReadOnlyPart **part );

  // dcop idl bug! it can't handle KonqMainWindow *&mainWindow
  static KonqView *findChildView( KParts::ReadOnlyPart *callingPart, const QString &name, KonqMainWindow **mainWindow, KParts::BrowserHostExtension **hostExtension, KParts::ReadOnlyPart **part );

  // Total number of views
  int viewCount() const { return m_mapViews.count(); }

  // Number of views not in "passive" mode
  int activeViewsCount() const;

  // Number of views that can be linked, i.e. not with "follow active view" behavior
  int linkableViewsCount() const;

  // Number of main views (non-toggle non-passive views)
  int mainViewsCount() const;

  typedef QMap<KParts::ReadOnlyPart *, KonqView *> MapViews;

  const MapViews & viewMap() const { return m_mapViews; }

  KonqView *currentView() const { return m_currentView; }

  KParts::ReadOnlyPart *currentPart() const;

  /** URL of current part, or URLs of selected items for directory views */
  KURL::List currentURLs() const;

  // Only valid if there are one or two views
  KonqView * otherView( KonqView * view ) const;

  virtual void customEvent( QCustomEvent *event );

  /// Overloaded of KMainWindow
  virtual void setCaption( const QString &caption );

  /**
   * Reimplemented for internal reasons. The API is not affected.
   */
  virtual void show();

  /**
   * Change URL displayed in the location bar
   */
  void setLocationBarURL( const QString &url );
  /**
   * Overload for convenience
   */
  void setLocationBarURL( const KURL &url );
  /**
   * Return URL displayed in the location bar - for KonqViewManager
   */
  QString locationBarURL() const;
  void focusLocationBar();

  /**
   * Set page security related to current view
   */
  void setPageSecurity( PageSecurity );

  void enableAllActions( bool enable );

  void disableActionsNoView();

  void updateToolBarActions( bool pendingActions = false );
  void updateOpenWithActions();
  void updateViewModeActions();
  void updateViewActions();

  bool sidebarVisible() const;

  void setShowHTML( bool b );

    void showHTML( KonqView * view, bool b, bool _activateView );

  bool fullScreenMode() const { return m_ptaFullScreen->isChecked(); }

  /**
   * @return the "link view" action, for checking/unchecking from KonqView
   */
  KToggleAction * linkViewAction()const { return m_paLinkView; }

  void enableAction( const char * name, bool enabled );
  void setActionText( const char * name, const QString& text );

  /**
   * The default settings "allow HTML" - the one used when creating a new view
   * Might not match the current view !
   */
  bool isHTMLAllowed() const { return m_bHTMLAllowed; }

  bool saveViewPropertiesLocally() const { return m_bSaveViewPropertiesLocally; }

  static Q3PtrList<KonqMainWindow> *mainWindowList() { return s_lstViews; }

  // public for konq_guiclients
  void viewCountChanged();

  // for the view manager
  void currentProfileChanged();

  // operates on all combos of all mainwindows of this instance
  // up to now adds an entry or clears all entries
  static void comboAction( int action, const QString& url,
			   const Q3CString& objId );

#ifndef NDEBUG
  void dumpViewList();
#endif

  // KonqFrameContainerBase implementation BEGIN

  /**
   * Call this after inserting a new frame into the splitter.
   */
  void insertChildFrame( KonqFrameBase * frame, int index = -1 );
  /**
   * Call this before deleting one of our children.
   */
  void removeChildFrame( KonqFrameBase * frame );

  void saveConfig( KConfig* config, const QString &prefix, bool saveURLs, KonqFrameBase* docContainer, int id = 0, int depth = 0 );

  void copyHistory( KonqFrameBase *other );

  void printFrameInfo( const QString &spaces );

  void reparentFrame( QWidget* parent,
                              const QPoint & p, bool showIt=FALSE );

  KonqFrameContainerBase* parentContainer()const;
  void setParentContainer(KonqFrameContainerBase* parent);

  void setTitle( const QString &title , QWidget* sender);
  void setTabIcon( const QString &url, QWidget* sender );

  QWidget* widget();

  void listViews( ChildViewList *viewList );
  Q3CString frameType();

  KonqFrameBase* childFrame()const;

  void setActiveChild( KonqFrameBase* activeChild );

  // KonqFrameContainerBase implementation END

  KonqFrameBase* workingTab()const { return m_pWorkingTab; }
  void setWorkingTab( KonqFrameBase* tab ) { m_pWorkingTab = tab; }

  static bool isMimeTypeAssociatedWithSelf( const QString &mimeType );
  static bool isMimeTypeAssociatedWithSelf( const QString &mimeType, const KService::Ptr &offer );

  void resetWindow();

  static void setPreloadedFlag( bool preloaded );
  static bool isPreloaded() { return s_preloaded; }
  static void setPreloadedWindow( KonqMainWindow* );
  static KonqMainWindow* preloadedWindow() { return s_preloadedWindow; }

  QString currentTitle() const;
  QString currentURL() const;
  QString currentProfile() const;

  QStringList configModules() const;

signals:
  void viewAdded( KonqView *view );
  void viewRemoved( KonqView *view );

public slots:
  void slotCtrlTabPressed();

  // for KBookmarkMenu and KBookmarkBar
  void slotFillContextMenu( const KBookmark &, Q3PopupMenu * );
  void slotOpenBookmarkURL( const QString & url, Qt::ButtonState state );

  void slotPopupMenu( const QPoint &_global, const KURL &_url, const QString &_mimeType, mode_t mode );
  void slotPopupMenu( KXMLGUIClient *client, const QPoint &_global, const KURL &_url, const QString &_mimeType, mode_t mode );
  void slotPopupMenu( KXMLGUIClient *client, const QPoint &_global, const KURL &_url, const KParts::URLArgs &_args, KParts::BrowserExtension::PopupFlags f, mode_t mode );

  void slotPopupMenu( const QPoint &_global, const KFileItemList &_items );
  void slotPopupMenu( KXMLGUIClient *client, const QPoint &_global, const KFileItemList &_items );
  void slotPopupMenu( KXMLGUIClient *client, const QPoint &_global, const KFileItemList &_items, const KParts::URLArgs &_args, KParts::BrowserExtension::PopupFlags _flags );


  void slotPopupMenu( KXMLGUIClient *client, const QPoint &_global, const KFileItemList &_items, const KParts::URLArgs &_args, KParts::BrowserExtension::PopupFlags f, bool showProperties );

  /**
   * __NEEEEVER__ call this method directly. It relies on sender() (the part)
   */
  void slotOpenURLRequest( const KURL &url, const KParts::URLArgs &args );

  void openURL( KonqView *childView, const KURL &url, const KParts::URLArgs &args );

  void slotCreateNewWindow( const KURL &url, const KParts::URLArgs &args );
  void slotCreateNewWindow( const KURL &url, const KParts::URLArgs &args,
                            const KParts::WindowArgs &windowArgs, KParts::ReadOnlyPart *&part );

  void slotNewWindow();
  void slotDuplicateWindow();
  void slotSendURL();
  void slotSendFile();
  void slotCopyFiles();
  void slotMoveFiles();
  void slotNewDir();
  void slotOpenTerminal();
  void slotOpenLocation();
  void slotToolFind();

  // View menu
  void slotViewModeToggle( bool toggle );
  void slotShowHTML();
  void slotLockView();
  void slotLinkView();
  void slotReload( KonqView* view = 0L );
  void slotStop();

  // Go menu
  void slotUp();
  void slotUp(KAction::ActivationReason, Qt::ButtonState state);
  void slotUpDelayed();
  void slotBack();
  void slotBack(KAction::ActivationReason, Qt::ButtonState state);
  void slotForward();
  void slotForward(KAction::ActivationReason, Qt::ButtonState state);
  void slotHome();
  void slotHome(KAction::ActivationReason, Qt::ButtonState state);
  void slotGoSystem();
  void slotGoApplications();
  void slotGoMedia();
  void slotGoNetworkFolders();
  void slotGoSettings();
  void slotGoDirTree();
  void slotGoTrash();
  void slotGoAutostart();
  void slotGoHistory();

  void slotConfigure();
  void slotConfigureToolbars();
  void slotConfigureExtensions();
  void slotConfigureSpellChecking();
  void slotNewToolbarConfig();

  void slotUndoAvailable( bool avail );

  void slotPartChanged( KonqView *childView, KParts::ReadOnlyPart *oldPart, KParts::ReadOnlyPart *newPart );

  void slotRunFinished();
  void slotClearLocationBar( KAction::ActivationReason reason, Qt::ButtonState state );

  // reimplement from KParts::MainWindow
  virtual void slotSetStatusBarText( const QString &text );

  // public for KonqViewManager
  void slotPartActivated( KParts::Part *part );

  virtual void setIcon( const QPixmap& );
  void slotGoHistoryActivated( int steps );
  void slotGoHistoryActivated( int steps, Qt::ButtonState state );

  void slotAddTab();

protected slots:
  void slotViewCompleted( KonqView * view );

  void slotURLEntered( const QString &text, int );

  void slotFileNewAboutToShow();
  void slotLocationLabelActivated();

  void slotSplitViewHorizontal();
  void slotSplitViewVertical();
  void slotDuplicateTab();
  void slotDuplicateTabPopup();

  void slotBreakOffTab();
  void slotBreakOffTabPopup();
  void slotBreakOffTabPopupDelayed();

  void slotPopupNewWindow();
  void slotPopupThisWindow();
  void slotPopupNewTab();
  void slotPopupNewTabRight();
  void slotPopupPasteTo();
  void slotRemoveView();

  void slotRemoveOtherTabsPopup();
  void slotRemoveOtherTabsPopupDelayed();

  void slotReloadPopup();
  void slotReloadAllTabs();
  void slotRemoveTab();
  void slotRemoveTabPopup();
  void slotRemoveTabPopupDelayed();

  void slotActivateNextTab();
  void slotActivatePrevTab();
  void slotActivateTab();

  void slotDumpDebugInfo();

  void slotSaveViewProfile();
  void slotSaveViewPropertiesLocally();
  void slotRemoveLocalProperties();

  void slotOpenEmbedded();
  void slotOpenEmbeddedDoIt();

  // Connected to KSycoca
  void slotDatabaseChanged();

  // Connected to KApp
  void slotReconfigure();

  void slotForceSaveMainWindowSettings();

  void slotOpenWith();

  void slotGoMenuAboutToShow();
  void slotUpAboutToShow();
  void slotBackAboutToShow();
  void slotForwardAboutToShow();

  void slotUpActivated( int id );
  void slotBackActivated( int id );
  void slotForwardActivated( int id );
  void slotGoHistoryDelayed();

  void slotCompletionModeChanged( KGlobalSettings::Completion );
  void slotMakeCompletion( const QString& );
  void slotSubstringcompletion( const QString& );
  void slotRotation( KCompletionBase::KeyBindingType );
  void slotMatch( const QString& );
  void slotClearHistory();
  void slotClearComboHistory();

  void slotClipboardDataChanged();
  void slotCheckComboSelection();

  void slotShowMenuBar();

  void slotOpenURL( const KURL& );

  void slotActionStatusText( const QString &text );
  void slotClearStatusText();

  void slotFindOpen( KonqDirPart * dirPart );
  void slotFindClosed( KonqDirPart * dirPart );

  void slotIconsChanged();

  virtual bool event( QEvent* );

  void slotMoveTabLeft();
  void slotMoveTabRight();

  void slotAddWebSideBar(const KURL& url, const QString& name);

  void slotUpdateFullScreen( bool set ); // do not call directly

protected:
  virtual bool eventFilter(QObject*obj,QEvent *ev);

  void fillHistoryPopup( Q3PopupMenu *menu, const Q3PtrList<HistoryEntry> &history );

  bool makeViewsFollow( const KURL & url, const KParts::URLArgs &args, const QString & serviceType,
                        KonqView * senderView );

  void applyKonqMainWindowSettings();

  void saveToolBarServicesMap();

  void viewsChanged();

  void updateLocalPropsActions();

  virtual void closeEvent( QCloseEvent * );
  virtual bool queryExit();

  bool askForTarget(const QString& text, KURL& url);

private slots:
  void slotRequesterClicked( KURLRequester * );
  void slotIntro();
  /**
   * Loads the url displayed currently in the lineedit of the locationbar, by
   * emulating a enter key press event.
   */
  void goURL();

  void bookmarksIntoCompletion();

  void initBookmarkBar();
  void slotTrashActivated( KAction::ActivationReason reason, Qt::ButtonState state );

  void showPageSecurity();

private:
  /**
   * takes care of hiding the bookmarkbar and calling setChecked( false ) on the
   * corresponding action
   */
  void updateBookmarkBar();

  /**
   * Adds all children of @p group to the static completion object
   */
  static void bookmarksIntoCompletion( const KBookmarkGroup& group );

  /**
   * Returns all matches of the url-history for @p s. If there are no direct
   * matches, it will try completing with http:// prepended, and if there's
   * still no match, then http://www. Due to that, this is only usable for
   * popupcompletion and not for manual or auto-completion.
   */
  static QStringList historyPopupCompletionItems( const QString& s = QString::null);

  void startAnimation();
  void stopAnimation();

  void setUpEnabled( const KURL &url );

  void initCombo();
  void initActions();

  void popupNewTab(bool infront, bool openAfterCurrentPage);

  /**
   * Tries to find a index.html (.kde.html) file in the specified directory
   */
  static QString findIndexFile( const QString &directory );

  void connectExtension( KParts::BrowserExtension *ext );
  void disconnectExtension( KParts::BrowserExtension *ext );

  void plugViewModeActions();
  void unplugViewModeActions();
  static QString viewModeActionKey( KService::Ptr service );

  void connectActionCollection( KActionCollection *coll );
  void disconnectActionCollection( KActionCollection *coll );

  bool stayPreloaded();
  bool checkPreloadResourceUsage();

  QObject* lastFrame( KonqView *view );

  KNewMenu * m_pMenuNew;

  KAction *m_paPrint;

  KActionMenu *m_pamBookmarks;

  KToolBarPopupAction *m_paUp;
  KToolBarPopupAction *m_paBack;
  KToolBarPopupAction *m_paForward;
  KAction *m_paHome;

  KonqBidiHistoryAction *m_paHistory;

  KAction *m_paSaveViewProfile;
  KToggleAction *m_paSaveViewPropertiesLocally;
  KAction *m_paRemoveLocalProperties;

  KAction *m_paSplitViewHor;
  KAction *m_paSplitViewVer;
  KAction *m_paAddTab;
  KAction *m_paDuplicateTab;
  KAction *m_paBreakOffTab;
  KAction *m_paRemoveView;
  KAction *m_paRemoveTab;
  KAction *m_paRemoveOtherTabs;
  KAction *m_paActivateNextTab;
  KAction *m_paActivatePrevTab;

  KAction *m_paSaveRemoveViewProfile;
  KActionMenu *m_pamLoadViewProfile;

  KToggleAction *m_paLockView;
  KToggleAction *m_paLinkView;
  KAction *m_paReload;
  KAction *m_paReloadAllTabs;
  KAction *m_paUndo;
  KAction *m_paCut;
  KAction *m_paCopy;
  KAction *m_paPaste;
  KAction *m_paStop;
  KAction *m_paRename;

  KAction *m_paTrash;
  KAction *m_paDelete;

  KAction *m_paCopyFiles;
  KAction *m_paMoveFiles;
  KAction *m_paNewDir;

  KAction *m_paMoveTabLeft;
  KAction *m_paMoveTabRight;

  KAction *m_paConfigureExtensions;
  KAction *m_paConfigureSpellChecking;

  KonqLogoAction *m_paAnimatedLogo;

  KBookmarkBar *m_paBookmarkBar;

  KToggleAction * m_paFindFiles;
  KToggleAction *m_ptaUseHTML;

  KToggleAction *m_paShowMenuBar;
  KToggleAction *m_paShowStatusBar;

  KToggleFullScreenAction *m_ptaFullScreen;

  uint m_bLocationBarConnected:1;
  uint m_bURLEnterLock:1;
  // Global settings
  uint m_bSaveViewPropertiesLocally:1;
  uint m_bHTMLAllowed:1;
  // Set in constructor, used in slotRunFinished
  uint m_bNeedApplyKonqMainWindowSettings:1;
  uint m_bViewModeToggled:1;

  int m_goBuffer;
  Qt::ButtonState m_goState;

  MapViews m_mapViews;

  QPointer<KonqView> m_currentView;

  KBookmarkMenu* m_pBookmarkMenu;
  KonqExtendedBookmarkOwner *m_pBookmarksOwner;
  KActionCollection* m_bookmarksActionCollection;
  KActionCollection* m_bookmarkBarActionCollection;

  KonqViewManager *m_pViewManager;
  KonqFrameBase* m_pChildFrame;

  KonqFrameBase* m_pWorkingTab;

  KFileItemList popupItems;
  KParts::URLArgs popupUrlArgs;

  KonqRun *m_initialKonqRun;

  QString m_title;

  /**
   * @since 3.4
   */
  KCMultiDialog* m_configureDialog;

  /**
   * A list of the modules to be shown in
   * the configure dialog.
   * @since 3.4
   */
  QStringList m_configureModules;

  QLabel* m_locationLabel;
  QPointer<KonqCombo> m_combo;
  static KConfig *s_comboConfig;
  KURLCompletion *m_pURLCompletion;
  // just a reference to KonqHistoryManager's completionObject
  static KCompletion *s_pCompletion;

  ToggleViewGUIClient *m_toggleViewGUIClient;

  KTrader::OfferList m_popupEmbeddingServices;
  QString m_popupService;
  QString m_popupServiceType;
  KURL m_popupURL;

  QString m_initialFrameName;

  Q3PtrList<KAction> m_openWithActions;
  KActionMenu *m_viewModeMenu;
  Q3PtrList<KAction> m_toolBarViewModeActions; // basically holds two KonqViewActions, one of
                                              // iconview and one for listview
  Q3PtrList<KRadioAction> m_viewModeActions;
  QMap<QString,KService::Ptr> m_viewModeToolBarServices; // similar to m_toolBarViewModeActions
  // it holds a map library name (libkonqiconview/libkonqlistview) ==> service (service for
  // iconview, multicolumnview, treeview, etc .)

  KonqMainWindowIface * m_dcopObject;

  static QStringList *s_plstAnimatedLogo;

  static Q3PtrList<KonqMainWindow> *s_lstViews;

  QString m_currentDir; // stores current dir for relative URLs whenever applicable

  bool m_urlCompletionStarted;

  static bool s_preloaded;
  static KonqMainWindow* s_preloadedWindow;
  static int s_initialMemoryUsage;
  static time_t s_startupTime;
  static int s_preloadUsageCount;

public:

  static QFile *s_crashlog_file;
};

#endif

