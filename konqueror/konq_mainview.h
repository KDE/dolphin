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

#ifndef __konq_mainview_h__
#define __konq_mainview_h__

#include <qmap.h>
#include <qpoint.h>
#include <qtimer.h>
#include <qguardedptr.h>

#include <konqfileitem.h>

#include <kparts/mainwindow.h>
#include <kbookmark.h>
#include <dcopobject.h>
#include <kxmlgui.h>
#include <kxmlguiclient.h>
#include <ktrader.h>

class KAction;
class KAction;
class KActionMenu;
class KSelectAction;
class KToggleAction;
class KonqChildView;
class KonqViewManager;
class KNewMenu;
class KProgress;
class KonqPart;
class KonqLogoAction;
class KonqComboAction;
class KonqHistoryAction;
class KonqBookmarkBar;
struct HistoryEntry;
class KonqFrameBase;
class KonqFrameContainer;
class KonqFrame;
class KBookmarkMenu;
class ViewModeGUIClient;
class OpenWithGUIClient;
class ToggleViewGUIClient;

namespace KParts {
 class BrowserExtension;
 class BrowserHostExtension;
 class ReadOnlyPart;
 struct URLArgs;
};

class KonqMainView : public KParts::MainWindow,
                     virtual public KBookmarkOwner,
                     public DCOPObject
{
  Q_OBJECT
  K_DCOP
public:
  KonqMainView( const KURL &initialURL = KURL(), bool openInitialURL = true, const char *name = 0 );
  ~KonqMainView();

  void openFilteredURL( KonqChildView *_view, const QString &_url );

  void openURL( KonqChildView *_view, const KURL &_url, const QString &serviceType = QString::null );

  KonqViewManager *viewManager() const { return m_pViewManager; }

  virtual QWidget *createContainer( QWidget *parent, int index, const QDomElement &element, const QByteArray &containerStateBuffer, int &id );

  void attachToolbars( KonqFrame *frame );

  virtual void saveProperties( KConfig *config );
  virtual void readProperties( KConfig *config );

  void setInitialFrameName( const QString &name );

public slots:

  void slotPopupMenu( const QPoint &_global, const KFileItemList &_items );
  void slotPopupMenu( const QPoint &_global, const KURL &_url, const QString &_mimeType, mode_t mode );
  void slotPopupMenu( KXMLGUIClient *client, const QPoint &_global, const KFileItemList &_items );
  void slotPopupMenu( KXMLGUIClient *client, const QPoint &_global, const KURL &_url, const QString &_mimeType, mode_t mode );

  /**
   * __NEEEEVER__ call this method directly. It relies on sender()
   */
  void openURL( const KURL &url, const KParts::URLArgs &args );

  void openURL( KonqChildView *childView, const KURL &url, const KParts::URLArgs &args );

  void slotCreateNewWindow( const KURL &url, const KParts::URLArgs &args );

  void slotNewWindow();
  void slotRun();
  void slotOpenTerminal();
  void slotOpenLocation();
  void slotToolFind();

  // View menu
  void slotViewModeToggle( bool toggle );
  void slotShowHTML();
  void slotLockView();
  void slotUnlockViews();
  void slotReload();
  void slotStop();

  // Go menu
  void slotUp();
  void slotBack();
  void slotForward();
  void slotHome();
  void slotGoMimeTypes();
  void slotGoApplications();
  void slotGoDirTree();
  void slotGoTrash();

  void slotSaveSettings();
  void slotConfigureFileManager();
  void slotConfigureBrowser();
  void slotConfigureFileTypes();
  void slotConfigureNetwork();
  void slotConfigureKeys();
  void slotConfigureToolbars();

  void slotViewChanged( KParts::ReadOnlyPart *oldView, KParts::ReadOnlyPart *newView );

  void slotRunFinished();

  // reimplement from KParts::MainWindow
  virtual void slotSetStatusBarText( const QString &text );

signals:
  void viewAdded( KonqChildView *view );
  void viewRemoved( KonqChildView *view );

public:
k_dcop:

  virtual void reparseConfiguration();

public:
  bool openView( QString serviceType, const KURL &_url, KonqChildView *childView );

  void insertChildView( KonqChildView *childView );
  void removeChildView( KonqChildView *childView );
  KonqChildView *childView( KParts::ReadOnlyPart *view );
  KonqChildView *childView( const QString &name, KParts::BrowserHostExtension **hostExtension );

  // dcop idl bug! it can't handle KonqMainView *&mainView
  static KonqChildView *findChildView( const QString &name, KonqMainView **mainView, KParts::BrowserHostExtension **hostExtension );

  int viewCount() const { return m_mapViews.count(); }
  QValueList<KParts::ReadOnlyPart *> viewList();

  typedef QMap<KParts::ReadOnlyPart *, KonqChildView *> MapViews;

  // for KonqFrame::slotLinkedViewClicked only...
  const MapViews & viewMap() const { return m_mapViews; }

  KonqChildView *currentChildView() const { return m_currentView; }
  KParts::ReadOnlyPart *currentView();

  virtual void customEvent( QCustomEvent *event );

  /// Overloaded of KTMainWindow
  virtual void setCaption( const QString &caption );

  /// Overloaded functions of KBookmarkOwner
  virtual void openBookmarkURL( const QString & _url );
  virtual QString currentTitle() const { return m_title; }
  virtual QString currentURL();

  /**
   * Change URL displayed in the location bar
   */
  void setLocationBarURL( const QString &url );

  void enableAllActions( bool enable );

  static void setMoveSelection( bool b ) { s_bMoveSelection = b; }

  void updateStatusBar() {} // to remove if we confirm we don't want that anymore
  void updateToolBarActions();

  bool fullScreenMode() const { return m_bFullScreen; }

  static QList<KonqMainView> *mainViewList() { return s_lstViews; }

public slots:
  void slotFullScreenStart();
  void slotFullScreenStop();

protected slots:
  void slotPartActivated( KParts::Part *part );
  void slotEnableAction( const char * name, bool enabled );

  void slotURLEntered( const QString &text );

  void slotFileNewAboutToShow();

  void slotSplitViewHorizontal();
  void slotSplitViewVertical();
  void slotSplitWindowHorizontal();
  void slotSplitWindowVertical();
  void slotRemoveView();

  void slotSaveDefaultProfile();

  void slotOpenEmbedded();
  void slotOpenEmbeddedDoIt();

  // Connected to KSycoca
  void slotDatabaseChanged();

  // Connected to KApp
  void slotReconfigure();
  void slotCut();
  void slotCopy();
  void slotPaste();

  void slotOpenWith();

  void slotAbout();

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
  //void slotShowStatusBar();
  void slotShowToolBar();
  void slotShowLocationBar();
  void slotShowBookmarkBar();

protected:

  void toggleBar( const char *name, const char *className );

  void fillHistoryPopup( QPopupMenu *menu, const QList<HistoryEntry> &history );

  void callExtensionMethod( KonqChildView * childView, const char * methodName );

private:

  void startAnimation();
  void stopAnimation();

  void setUpEnabled( const KURL &url );

  void initActions();
  void initPlugins();

  QString konqFilteredURL( const QString &url );

  /**
   * Tries to find a index.html (.kde.html) file in the specified directory
   */
  QString findIndexFile( const QString &directory );

  void connectExtension( KParts::BrowserExtension *ext );
  void disconnectExtension( KParts::BrowserExtension *ext );

  KNewMenu * m_pMenuNew;
  KAction *m_paNewWindow;

  KAction *m_paFileType;
  KAction *m_paProperties;

  KAction *m_paPrint;
  KAction *m_paShellClose;

  KActionMenu *m_pamBookmarks;

  KonqHistoryAction *m_paUp;
  KonqHistoryAction *m_paBack;
  KonqHistoryAction *m_paForward;

  KAction *m_paSaveSettings;
  KAction *m_paSaveLocalProperties;

  KAction *m_paConfigureFileManager;
  KAction *m_paConfigureBrowser;
  KAction *m_paConfigureFileTypes;
  KAction *m_paConfigureNetwork;
  KAction *m_paConfigureKeys;
  KAction *m_paConfigureToolbars;

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
  KAction *m_paReload;
  KAction *m_paCut;
  KAction *m_paCopy;
  KAction *m_paPaste;
  KAction *m_paStop;

  KAction *m_paTrash;
  KAction *m_paDelete;
  KAction *m_paShred;

  KonqLogoAction *m_paAnimatedLogo;

  KonqComboAction *m_paURLCombo;
  KonqBookmarkBar *m_paBookmarkBar;

  KToggleAction *m_ptaUseHTML;

  KToggleAction *m_paShowMenuBar;
  KToggleAction *m_paShowStatusBar;
  KToggleAction *m_paShowToolBar;
  KToggleAction *m_paShowLocationBar;
  KToggleAction *m_paShowBookmarkBar;

  KAction *m_paFullScreenStart;
  KAction *m_paFullScreenStop;

  bool m_bFullScreen;
  KonqFrameContainer *m_tempContainer;
  QWidget::FocusPolicy m_tempFocusPolicy;

  MapViews m_mapViews;

  KonqChildView *m_currentView;

  KBookmarkMenu* m_pBookmarkMenu;

  KonqViewManager *m_pViewManager;

  //KStatusBar * m_statusBar;

  QString m_title;

  QGuardedPtr<KProgress> m_progressBar;

  bool m_bURLEnterLock;

  KonqChildView *m_oldView;

  QGuardedPtr<QComboBox> m_combo;

  ViewModeGUIClient *m_viewModeGUIClient;
  OpenWithGUIClient *m_openWithGUIClient;
  ToggleViewGUIClient *m_toggleViewGUIClient;

  KTrader::OfferList m_popupEmbeddingServices;
  QString m_popupService;
  QString m_popupServiceType;
  KURL m_popupURL;

  QString m_initialFrameName;

  static QStringList *s_plstAnimatedLogo;

  static bool s_bMoveSelection;

  static QList<KonqMainView> *s_lstViews;

  typedef QMap<QCString,QCString> ActionSlotMap;
  static ActionSlotMap *s_actionSlotMap;
};

class ViewModeGUIClient : public QObject, public KXMLGUIClient
{
public:
  ViewModeGUIClient( KonqMainView *mainView );

  virtual KAction *action( const QDomElement &element ) const;

  void update( const KTrader::OfferList &services );

private:
  KonqMainView *m_mainView;
  QDomDocument m_doc;
  QDomElement m_menuElement;
  KActionCollection *m_actions;
};

class OpenWithGUIClient : public QObject, public KXMLGUIClient
{
public:
  OpenWithGUIClient( KonqMainView *mainView );

  virtual KAction *action( const QDomElement &element ) const;

  void update( const KTrader::OfferList &services );

private:
  KonqMainView *m_mainView;
  QDomDocument m_doc;
  QDomElement m_menuElement;
  KActionCollection *m_actions;
};

class PopupMenuGUIClient : public KXMLGUIClient
{
public:
  PopupMenuGUIClient( KonqMainView *mainView, const KTrader::OfferList &embeddingServices );
  virtual ~PopupMenuGUIClient();

  virtual KAction *action( const QDomElement &element ) const;

private:
  void addEmbeddingService( QDomElement &menu, int idx, const QString &name, const KService::Ptr &service );

  KonqMainView *m_mainView;

  QDomDocument m_doc;
};

class ToggleViewGUIClient : public QObject, public KXMLGUIClient
{
  Q_OBJECT
public:
  ToggleViewGUIClient( KonqMainView *mainView );
  virtual ~ToggleViewGUIClient();

  bool empty() const { return m_empty; }

private slots:
  void slotToggleView( bool toggle );
  void slotViewAdded( KonqChildView *view );
  void slotViewRemoved( KonqChildView *view );
private:
  KonqMainView *m_mainView;
  QDomDocument m_doc;
  bool m_empty;
  QMap<QString,bool> m_mapOrientation;
};

#endif
