/* This file is part of the KDE project
   Copyright (C) 1998, 1999 Simon Hausmann <hausmann@kde.org>

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

#ifndef __konq_mainwindow_h__
#define __konq_mainwindow_h__

#include <qmap.h>
#include <qpoint.h>
#include <qtimer.h>
#include <qguardedptr.h>

#include <konqfileitem.h>
#include <konq_openurlrequest.h>

#include <kparts/mainwindow.h>
#include <kbookmark.h>
#include <dcopobject.h>
#include <kxmlgui.h>
#include <kxmlguiclient.h>
#include <ktrader.h>

class KAction;
class KActionMenu;
class KBookmarkMenu;
class KHistoryCombo;
class KNewMenu;
class KProgress;
class KSelectAction;
class KToggleAction;
class KonqBidiHistoryAction;
class KonqBookmarkBar;
class KonqView;
class KonqComboAction;
class KonqFrame;
class KonqFrameBase;
class KonqFrameContainer;
class KonqHistoryAction;
class KonqLogoAction;
class KonqPart;
class KonqViewManager;
class OpenWithGUIClient;
class ToggleViewGUIClient;
class ViewModeGUIClient;
class KonqMainWindowIface;
struct HistoryEntry;

namespace KParts {
 class BrowserExtension;
 class BrowserHostExtension;
 class ReadOnlyPart;
 struct URLArgs;
};

class KonqMainWindow : public KParts::MainWindow,
                     virtual public KBookmarkOwner
{
  Q_OBJECT
public:
  KonqMainWindow( const KURL &initialURL = KURL(), bool openInitialURL = true, const char *name = 0 );
  ~KonqMainWindow();

  /**
   * Filters the URL and calls the main openURL method.
   */
  void openFilteredURL( const QString &_url );

  /**
   * The main openURL method.
   */
  void openURL( KonqView * view, const KURL & url,
                const QString &serviceType = QString::null,
                const KonqOpenURLRequest & req = KonqOpenURLRequest() );

  /**
   * Called by openURL when it knows the service type (either directly,
   * or using KonqRun)
   * @param req is not passed by reference because the calling KonqRun
   * that holds it might get destroyed (by childView->stop())
   */
  bool openView( QString serviceType, const KURL &_url, KonqView *childView,
                 KonqOpenURLRequest req = KonqOpenURLRequest() );


  void abortLoading();

  KonqViewManager *viewManager() const { return m_pViewManager; }

  virtual QWidget *createContainer( QWidget *parent, int index, const QDomElement &element, const QByteArray &containerStateBuffer, int &id );

  void attachToolbars( KonqFrame *frame );

  virtual void saveProperties( KConfig *config );
  virtual void readProperties( KConfig *config );

  void setInitialFrameName( const QString &name );

  KonqMainWindowIface * dcopObject();

  void reparseConfiguration();

  void insertChildView( KonqView *childView );
  void removeChildView( KonqView *childView );
  KonqView *childView( KParts::ReadOnlyPart *view );
  KonqView *childView( const QString &name, KParts::BrowserHostExtension **hostExtension );

  // dcop idl bug! it can't handle KonqMainWindow *&mainWindow
  static KonqView *findChildView( const QString &name, KonqMainWindow **mainWindow, KParts::BrowserHostExtension **hostExtension );

  int viewCount() const { return m_mapViews.count(); }
  int activeViewsCount() const;

  typedef QMap<KParts::ReadOnlyPart *, KonqView *> MapViews;

  const MapViews & viewMap() const { return m_mapViews; }

  KonqView *currentView() const { return m_currentView; }
  KParts::ReadOnlyPart *currentPart();

  // Only valid if there are one or two views
  KonqView * otherView( KonqView * view ) const;

  virtual void customEvent( QCustomEvent *event );

  /// Overloaded of KTMainWindow
  virtual void setCaption( const QString &caption );

  /// Overloaded functions of KBookmarkOwner
  virtual void openBookmarkURL( const QString & _url );
  virtual QString currentTitle() const { return m_title; }
  virtual QString currentURL() const;

  /**
   * Change URL displayed in the location bar
   */
  void setLocationBarURL( const QString &url );

  void enableAllActions( bool enable );

  void updateToolBarActions();

  bool fullScreenMode() const { return m_ptaFullScreen->isChecked(); }

  /**
   * @return the "link view" action, for checking/unchecking from KonqView
   */
  KToggleAction * linkViewAction() { return m_paLinkView; }

  /**
   * The default settings "allow HTML" - the one used when creating a new view
   * Might not match the current view !
   */
  bool isHTMLAllowed() const { return m_bHTMLAllowed; }

  static QList<KonqMainWindow> *mainWindowList() { return s_lstViews; }

signals:
  void viewAdded( KonqView *view );
  void viewRemoved( KonqView *view );

public slots:
  void slotToggleFullScreen( bool );
  void slotFullScreenStart();
  void slotFullScreenStop();

  void slotPopupMenu( const QPoint &_global, const KFileItemList &_items );
  void slotPopupMenu( const QPoint &_global, const KURL &_url, const QString &_mimeType, mode_t mode );
  void slotPopupMenu( KXMLGUIClient *client, const QPoint &_global, const KFileItemList &_items );
  void slotPopupMenu( KXMLGUIClient *client, const QPoint &_global, const KURL &_url, const QString &_mimeType, mode_t mode );

  /**
   * __NEEEEVER__ call this method directly. It relies on sender() (the part)
   */
  void slotOpenURLRequest( const KURL &url, const KParts::URLArgs &args );

  void openURL( KonqView *childView, const KURL &url, const KParts::URLArgs &args );

  void slotCreateNewWindow( const KURL &url, const KParts::URLArgs &args );

  void slotNewWindow();
  void slotDuplicateWindow();
  void slotRun();
  void slotCopyFiles();
  void slotMoveFiles();
  void slotOpenTerminal();
  void slotOpenLocation();
  void slotToolFind();

  // View menu
  void slotViewModeToggle( bool toggle );
  void slotShowHTML();
  void slotLockView();
  void slotUnlockViews();
  void slotLinkView();
  void slotReload();
  void slotStop();

  // Go menu
  void slotUp();
  void slotBack();
  void slotForward();
  void slotHome();
  void slotGoApplications();
  void slotGoDirTree();
  void slotGoTrash();
  void slotGoTemplates();
  void slotGoAutostart();

  void slotConfigureFileManager();
  void slotConfigureFileTypes();
  void slotConfigureBrowser();
  void slotConfigureEBrowsing();
  void slotConfigureCookies();
  void slotConfigureProxies();
  void slotConfigureKeys();
  void slotConfigureToolbars();

  void slotUndoAvailable( bool avail );

  void slotPartChanged( KonqView *childView, KParts::ReadOnlyPart *oldPart, KParts::ReadOnlyPart *newPart );

  void slotRunFinished();
  void slotClearLocationBar();

  // reimplement from KParts::MainWindow
  virtual void slotSetStatusBarText( const QString &text );

protected slots:
  void slotPartActivated( KParts::Part *part );
  void slotViewCompleted( KonqView * view );
  void slotEnableAction( const char * name, bool enabled );

  void slotURLEntered( const QString &text );

  void slotFileNewAboutToShow();

  void slotSplitViewHorizontal();
  void slotSplitViewVertical();
  void slotSplitWindowHorizontal();
  void slotSplitWindowVertical();
  void slotRemoveView();

  void slotSaveViewPropertiesLocally();
  void slotRemoveLocalProperties();

  void slotOpenEmbedded();
  void slotOpenEmbeddedDoIt();

  // Connected to KSycoca
  void slotDatabaseChanged();

  // Connected to KApp
  void slotReconfigure();

  void slotOpenWith();

  //void slotAbout();

  void slotGoMenuAboutToShow();
  void slotUpAboutToShow();
  void slotBackAboutToShow();
  void slotForwardAboutToShow();

  void slotGoHistoryActivated( int steps );
  void slotUpActivated( int id );
  void slotBackActivated( int id );
  void slotForwardActivated( int id );

  void slotComboPlugged();

  void slotShowMenuBar();
  void slotShowToolBar();
  void slotShowLocationBar();
  void slotShowBookmarkBar();

  void slotActionHighlighted( KAction *action );

protected:

  QString detectNameFilter( QString & url );

  void toggleBar( const char *name, const char *className );

  void fillHistoryPopup( QPopupMenu *menu, const QList<HistoryEntry> &history );

  void makeViewsFollow( const KURL & url, KonqView * senderView );

  void applyMainWindowSettings();

  void viewCountChanged();
  void viewsChanged();

private:
  /**
   * takes care of hiding the bookmarkbar and calling setChecked( false ) on the
   * corresponding action
   */
  void updateBookmarkBar();

  void startAnimation();
  void stopAnimation();

  void setUpEnabled( const KURL &url );

  void initActions();

  /**
   * Tries to find a index.html (.kde.html) file in the specified directory
   */
  QString findIndexFile( const QString &directory );

  void connectExtension( KParts::BrowserExtension *ext );
  void disconnectExtension( KParts::BrowserExtension *ext );

  void updateOpenWithActions( const KTrader::OfferList &services );
  void updateViewModeActions( const KTrader::OfferList &services );
  void plugViewModeActions();
  void unplugViewModeActions();

  KNewMenu * m_pMenuNew;

  KAction *m_paFileType;
  KAction *m_paProperties;

  KAction *m_paPrint;
  KAction *m_paShellClose;

  KActionMenu *m_pamBookmarks;

  KonqHistoryAction *m_paUp;
  KonqHistoryAction *m_paBack;
  KonqHistoryAction *m_paForward;
  KAction *m_paHome;

  KonqBidiHistoryAction *m_paHistory;

  KToggleAction *m_paSaveViewPropertiesLocally;
  KAction *m_paRemoveLocalProperties;

  KAction *m_paSplitViewHor;
  KAction *m_paSplitViewVer;
  KAction *m_paSplitWindowHor;
  KAction *m_paSplitWindowVer;
  KAction *m_paRemoveView;

  KAction *m_paSaveDefaultProfile;

  KAction *m_paSaveRemoveViewProfile;
  KActionMenu *m_pamLoadViewProfile;

  KAction *m_paLockView;
  KAction *m_paUnlockAll;
  KToggleAction *m_paLinkView;
  KAction *m_paReload;
  KAction *m_paUndo;
  KAction *m_paCut;
  KAction *m_paCopy;
  KAction *m_paPaste;
  KAction *m_paStop;

  KAction *m_paTrash;
  KAction *m_paDelete;
  KAction *m_paShred;

  KAction *m_paCopyFiles;
  KAction *m_paMoveFiles;

  KonqLogoAction *m_paAnimatedLogo;

  KonqComboAction *m_paURLCombo;
  KonqBookmarkBar *m_paBookmarkBar;

  KToggleAction *m_ptaUseHTML;

  KToggleAction *m_paShowMenuBar;
  KToggleAction *m_paShowStatusBar;
  KToggleAction *m_paShowToolBar;
  KToggleAction *m_paShowLocationBar;
  KToggleAction *m_paShowBookmarkBar;

  KToggleAction *m_ptaFullScreen;

  KonqFrameContainer *m_tempContainer;
  QWidget::FocusPolicy m_tempFocusPolicy;

  MapViews m_mapViews;

  KonqView *m_currentView;

  KBookmarkMenu* m_pBookmarkMenu;

  KonqViewManager *m_pViewManager;

  QString m_title;

  bool m_bURLEnterLock;

  QGuardedPtr<KHistoryCombo> m_combo;

  ToggleViewGUIClient *m_toggleViewGUIClient;

  KTrader::OfferList m_popupEmbeddingServices;
  QString m_popupService;
  QString m_popupServiceType;
  KURL m_popupURL;

  QString m_initialFrameName;

  // Set in constructor, used in slotRunFinished
  bool m_bNeedApplyMainWindowSettings;
  // Global settings
  bool m_bSaveViewPropertiesLocally;
  bool m_bHTMLAllowed;
  QString m_sViewModeForDirectory; // is actually the name of the service

  QList<KAction> m_openWithActions;
  KActionMenu *m_viewModeMenu;
  QList<KAction> m_viewModeActions;
  bool m_bViewModeToggled;

  KonqMainWindowIface * m_dcopObject;

  static QStringList *s_plstAnimatedLogo;

  static QList<KonqMainWindow> *s_lstViews;

  typedef QMap<QCString,QCString> ActionSlotMap;
  static ActionSlotMap *s_actionSlotMap;

};

#endif
