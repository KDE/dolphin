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
class EmbedData;

namespace KParts {
 class BrowserExtension;
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

  void openURL( KonqChildView *_view, const KURL &_url, bool reload = false,
                int xOffset = 0, int yOffset = 0, const QString &serviceType = QString::null );

  KonqViewManager *viewManager() const { return m_pViewManager; }

  virtual QWidget *createContainer( QWidget *parent, int index, const QDomElement &element, const QByteArray &containerStateBuffer, int &id );

  void attachToolbars( KonqFrame *frame );

  virtual void saveProperties( KConfig *config );
  virtual void readProperties( KConfig *config );

public slots:

  void slotPopupMenu( const QPoint &_global, const KonqFileItemList &_items );
  void slotPopupMenu( const QPoint &_global, const KURL &_url, const QString &_mimeType, mode_t mode );
  void slotPopupMenu( KXMLGUIClient *client, const QPoint &_global, const KonqFileItemList &_items );
  void slotPopupMenu( KXMLGUIClient *client, const QPoint &_global, const KURL &_url, const QString &_mimeType, mode_t mode );

  void openURL( const KURL &url, bool reload, int xOffset, int yOffset, const QString &serviceType );

  void openURL( const KURL &url, const KParts::URLArgs &args );

  void slotCreateNewWindow( const KURL &url, const KParts::URLArgs &args );

  void slotNewWindow();
  void slotRun();
  void slotOpenTerminal();
  void slotOpenLocation();
  void slotToolFind();

  // View menu
  void slotViewModeToggle( bool toggle );
  void slotShowHTML();
  void slotToggleDirTree( bool );
  void slotToggleCmdLine( bool );
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

  void slotSelectionInfo( const KonqFileItemList &items );

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

  /// Overloaded of KTMainWindow
  virtual void setCaption( const QString &caption );

  /// Overloaded functions of KBookmarkOwner
  virtual void openBookmarkURL( const QString & _url );
  virtual QString currentTitle() { return m_title; }
  virtual QString currentURL();

  void setLocationBarURL( KonqChildView *childView, const QString &url );

  void enableAllActions( bool enable );

  static void setMoveSelection( bool b ) { s_bMoveSelection = b; }

  void updateStatusBar();
  void updateToolBarActions();

  void speedProgress( int bytesPerSecond );

  bool fullScreenMode() { return m_bFullScreen; }

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

  // Connected to KonqPopupMenu
  void slotOpenEmbedded( const QString & serviceType, const KURL & url, const QString & serviceName );
  void slotOpenEmbeddedDoIt();

  // Connected to KSycoca
  void slotDatabaseChanged();

  void slotCut();
  void slotCopy();
  void slotPaste();
  void slotTrash();
  void slotDelete();
  void slotShred();
  void slotPrint();

  void slotOpenWith();

  void slotSetLocationBarURL( const QString &url );

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

  void slotFullScreenStart();
  void slotFullScreenStop();

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

  KAction *m_paRun;
  KAction *m_paOpenTerminal;
  KAction *m_paOpenLocation;
  KAction *m_paToolFind;

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
  KToggleAction *m_ptaShowDirTree;
  KToggleAction *m_ptaShowCmdLine;

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

  EmbedData * m_embeddingData;

  typedef QMap<KParts::ReadOnlyPart *, KonqChildView *> MapViews;

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

  static QStringList *s_plstAnimatedLogo;

  static bool s_bMoveSelection;
};

class ViewModeGUIClient : public QObject, public KXMLGUIClient
{
public:
  ViewModeGUIClient( KonqMainView *mainView );

  virtual KAction *action( const QDomElement &element );
  virtual QDomDocument document() const;

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

  virtual KAction *action( const QDomElement &element );
  virtual QDomDocument document() const;

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
  PopupMenuGUIClient( KonqMainView *mainView );
  virtual ~PopupMenuGUIClient();

  virtual KActionCollection *actionCollection() const;

private:
  KonqMainView *m_mainView;
};

#endif
