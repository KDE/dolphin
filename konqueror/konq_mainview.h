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

#include <kfileitem.h>

#include <kpartsmainwindow.h>
#include <kbookmark.h>
#include <dcopobject.h>
#include <kxmlgui.h>
#include <ktrader.h>

class QAction;
class KAction;
class KActionMenu;
class KSelectAction;
class KToggleAction;
class KonqChildView;
class KonqViewManager;
class KStatusBar;
class KNewMenu;
class KProgress;
class KonqPart;
class KonqComboAction;
class KonqHistoryAction;
class KonqBookmarkBar;
struct HistoryEntry;
class KonqFrameBase;
class KBookmarkMenu;
class ViewModeGUIServant;

namespace KParts {
 class BrowserExtension;
 class ReadOnlyPart;
};

class KonqMainView : public KParts::MainWindow,
                     virtual public KBookmarkOwner,
                     public DCOPObject
{
  Q_OBJECT
  K_DCOP
public:
  KonqMainView( const QString &initialURL = QString::null, bool openInitialURL = true, const char *name = 0 );
  ~KonqMainView();

  void openFilteredURL( KonqChildView *_view, const QString &_url );

  void openURL( KonqChildView *_view, const KURL &_url, bool reload = false,
                int xOffset = 0, int yOffset = 0, const QString &serviceType = QString::null );

  KonqViewManager *viewManager() const { return m_pViewManager; }

public slots:

  void slotPopupMenu( const QPoint &_global, const KFileItemList &_items );

  void openURL( const KURL &url, bool reload, int xOffset, int yOffset, const QString &serviceType );

  void slotCreateNewWindow( const KURL &url );

  void slotNewWindow();
  void slotRun();
  void slotOpenTerminal();
  void slotOpenLocation();
  void slotToolFind();

  // View menu
  void slotViewModeToggle( bool toggle );
  void slotShowHTML();
  void slotToggleDirTree( bool );
  void slotReload();
  void slotStop();

  // Go menu
  void slotUp();
  void slotBack();
  void slotForward();
  void slotHome();
  void slotShowCache();
  void slotShowHistory();
  void slotEditMimeTypes();
  void slotEditApplications();
  void slotEditDirTree();

  void slotSaveSettings();
  void slotConfigureFileManager();
  void slotConfigureBrowser();
  void slotConfigureFileTypes();
  void slotConfigureNetwork();
  void slotConfigureKeys();

  void slotViewChanged( KParts::ReadOnlyPart *oldView, KParts::ReadOnlyPart *newView );

  void slotRunFinished();

  void slotSetStatusBarText( const QString &text );

public:
k_dcop:

  virtual void reparseConfiguration();

public:
  bool openView( QString serviceType, const KURL &_url, KonqChildView *childView );

  void insertChildView( KonqChildView *childView );
  void removeChildView( KonqChildView *childView );
  KonqChildView *childView( KParts::ReadOnlyPart *view );

  int viewCount() { return m_mapViews.count(); }
  QValueList<KParts::ReadOnlyPart *> viewList();

  QMap<KParts::ReadOnlyPart *, KonqChildView *> viewMap() const { return m_mapViews; }

  KonqChildView *currentChildView() { return m_currentView; }
  KParts::ReadOnlyPart *currentView();

  virtual void customEvent( QCustomEvent *event );

  /// Overloaded functions of KBookmarkOwner
  virtual void openBookmarkURL( const QString & _url );
  virtual QString currentTitle();
  virtual QString currentURL();

  void setLocationBarURL( KonqChildView *childView, const QString &url );

  void enableAllActions( bool enable );

  static void setMoveSelection( bool b ) { s_bMoveSelection = b; }

  void updateStatusBar();
  void updateToolBarActions();

  void speedProgress( int bytesPerSecond );

protected slots:
  void slotPartActivated( KParts::Part *part );
  void slotEnableAction( const char * name, bool enabled );

  void slotAnimatedLogoTimeout();

  void slotURLEntered( const QString &text );

  void slotFileNewAboutToShow();

  void slotSplitViewHorizontal();
  void slotSplitViewVertical();
  void slotSplitWindowHorizontal();
  void slotSplitWindowVertical();
  void slotRemoveView();

  void slotSaveDefaultProfile();

  void slotDatabaseChanged(); // connect to KSycoca

  void slotCut();
  void slotCopy();
  void slotPaste();
  void slotTrash();
  void slotDelete();

  void slotSetLocationBarURL( const QString &url );

  void slotAbout();
  void slotReportBug();

  void slotUpAboutToShow();
  void slotBackAboutToShow();
  void slotForwardAboutToShow();

  void slotUpActivated( int id );
  void slotBackActivated( int id );
  void slotForwardActivated( int id );

  void slotComboPlugged();

  void slotShowMenuBar();
  void slotShowStatusBar();
  void slotShowToolBar();
  void slotShowLocationBar();
  void slotShowBookmarkBar();

protected:

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

  KAction *m_paRun;
  KAction *m_paOpenTerminal;
  KAction *m_paOpenLocation;
  KAction *m_paToolFind;

  KAction *m_paPrint;

  KActionMenu *m_pamEdit;
  KActionMenu *m_pamView;
  KActionMenu *m_pamBookmarks;

  KonqHistoryAction *m_paUp;
  KonqHistoryAction *m_paBack;
  KonqHistoryAction *m_paForward;
  KAction *m_paHome;

  KAction *m_paCache;
  KAction *m_paHistory;
  KAction *m_paMimeTypes;
  KAction *m_paApplications;
  KAction *m_paDirTree;

  KAction *m_paSaveSettings;
  KAction *m_paSaveLocalProperties;

  KAction *m_paConfigureFileManager;
  KAction *m_paConfigureBrowser;
  KAction *m_paConfigureFileTypes;
  KAction *m_paConfigureNetwork;
  KAction *m_paConfigureKeys;

  KAction *m_paSplitViewHor;
  KAction *m_paSplitViewVer;
  KAction *m_paSplitWindowHor;
  KAction *m_paSplitWindowVer;
  KAction *m_paRemoveView;

  KAction *m_paSaveDefaultProfile;

  KAction *m_paSaveRemoveViewProfile;
  KActionMenu *m_pamLoadViewProfile;

  KAction *m_paAbout;
  KAction *m_paReportBug;

  KAction *m_paReload;
  KAction *m_paCut;
  KAction *m_paCopy;
  KAction *m_paPaste;
  KAction *m_paStop;

  KAction *m_paTrash;
  KAction *m_paDelete;

  KAction *m_paAnimatedLogo;

  KonqComboAction *m_paURLCombo;
  KonqBookmarkBar *m_paBookmarkBar;

  KToggleAction *m_ptaUseHTML;
  KToggleAction *m_ptaShowDirTree;

  KAction *m_paShellClose;
  KAction *m_paShellHelpAboutKDE;
  KHelpMenu *m_helpMenu;

  KToggleAction *m_paShowMenuBar;
  KToggleAction *m_paShowStatusBar;
  KToggleAction *m_paShowToolBar;
  KToggleAction *m_paShowLocationBar;
  KToggleAction *m_paShowBookmarkBar;

  typedef QMap<KParts::ReadOnlyPart *, KonqChildView *> MapViews;

  MapViews m_mapViews;

  KonqChildView *m_currentView;

  KBookmarkMenu* m_pBookmarkMenu;

  KonqViewManager *m_pViewManager;

  QGuardedPtr<KStatusBar> m_statusBar;

  QGuardedPtr<KProgress> m_progressBar;

  QTimer m_animatedLogoTimer;
  unsigned int m_animatedLogoCounter;

  bool m_bURLEnterLock;

  KonqChildView *m_oldView;

  QGuardedPtr<QComboBox> m_combo;

  ViewModeGUIServant *m_viewModeGUIServant;

  static QList<QPixmap> *s_plstAnimatedLogo;

  static bool s_bMoveSelection;
};

class ViewModeGUIServant : public QObject, public KParts::XMLGUIServant
{
  Q_OBJECT
public:
  ViewModeGUIServant( KonqMainView *mainView );

  virtual QAction *action( const QDomElement &element );
  virtual QDomDocument document() const;

  void update( const KTrader::OfferList &services );

private:
  KonqMainView *m_mainView;
  QDomDocument m_doc;
  QDomElement m_menuElement;
  QActionCollection *m_actions;
};

#endif
