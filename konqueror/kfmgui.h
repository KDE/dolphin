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

#ifndef __kfm_gui_h__
#define __kfm_gui_h__

#include <ktopwidget.h>
#include <kstatusbar.h>
#include <ktoolbar.h>
#include <kmenubar.h>
#include <kpanner.h>
#include <kaccel.h>

#include "kfmview.h"
#include "kfmpopup.h"
#include "kfm_abstract_gui.h"

#include <qpixmap.h>
#include <qlist.h>
#include <qpopmenu.h>
#include <qpixmap.h>
#include <qtimer.h>
#include <qlayout.h>

#include <string>
#include <list>

// browser/tree window color defaults -- Bernd
#define HTML_DEFAULT_BG_COLOR white
#define HTML_DEFAULT_LNK_COLOR red
#define HTML_DEFAULT_TXT_COLOR black
#define HTML_DEFAULT_VLNK_COLOR magenta

// lets be modern .. -- Bernd
#define DEFAULT_VIEW_FONT "helvetica"
#define DEFAULT_VIEW_FIXED_FONT "courier"
#define DEFAULT_VIEW_FONT_SIZE 12

// the default size of the kfm browswer windows
// these are optimized sizes displaying a maximum number
// of icons. -- Bernd
#define KFMGUI_HEIGHT 360
#define KFMGUI_WIDTH 540

class KBookmarkMenu;
class KURLCompletion;

class KfmGui : public KTopLevelWidget, public KfmAbstractGui
{
  Q_OBJECT
public:
  KfmGui( const char *_url );
  ~KfmGui();
  
  /////////////////////////
  // Overloaded functions from @ref KfmAbstractGUI
  /////////////////////////  
  const QColor& bgColor() { return m_bgColor; }
  const QColor& textColor() { return m_textColor; }
  const QColor& linkColor() { return m_linkColor; }
  const QColor& vLinkColor() { return m_vLinkColor; }
  
  const char* stdFontName() { return m_strStdFontName; }
  const char* fixedFontName() { return m_strFixedFontName; }
  int fontSize() { return m_iFontSize; }

  const QPixmap& bgPixmap() { return m_bgPixmap; }
  
  MouseMode mouseMode() { return m_mouseMode; }

  bool underlineLink() { return m_underlineLink; }
  bool changeCursor() { return m_bChangeCursor; }
  bool isShowingDotFiles() { return m_bShowDot; }

  void setStatusBarText( const char *_text );
  void setLocationBarURL( const char *_url );
  void setUpURL( const char *_url );
  
  KfmView::ViewMode viewMode() { return m_viewMode; }

  void addHistory( const char *_url, int _xoffset, int _yoffset );

  void createGUI( const char *_url );
  
  bool hasUpURL() { return !m_strUpURL.isEmpty(); }
  bool hasBackHistory() { return m_lstBack.size() > 0; }
  bool hasForwardHistory() { return m_lstForward.size() > 0; }

protected slots:
  /////////////////////////
  // MenuBar
  /////////////////////////
  void slotLargeIcons();
  void slotSmallIcons();
  void slotTreeView();
  void slotHTMLView();
  void slotSaveGeometry();
  void slotShowCache();
  void slotShowHistory();
  void slotOpenLocation();
  void slotSplitView();
  void slotConfigureKeys();
  void slotAboutApp();
  
  /////////////////////////
  // Accel
  /////////////////////////
  void slotFocusLeftView();
  void slotFocusRightView();
  
  /////////////////////////
  // Location Bar
  /////////////////////////
  void slotURLEntered();

  /////////////////////////
  // Animated Logo
  /////////////////////////
  void slotAnimatedLogoTimeout();
  void slotStartAnimation();
  void slotStopAnimation();
  
  /////////////////////////
  // ToolBar
  /////////////////////////
  void slotStop();
  void slotNewWindow();
  void slotUp();
  void slotHome();
  void slotBack();
  void slotForward();
  void slotReload();
  
  void slotGotFocus( KfmView* _view );

protected:
  struct History
  {
    QString m_strURL;
    int m_iXOffset;
    int m_iYOffset;
  };
  
  void initConfig();
  void initGui();
  void initPanner();
  void initMenu();
  void initStatusBar();
  void initToolBar();
  void initView();
  
  void setViewModeMenu( KfmView::ViewMode _viewMode );

  KMenuBar *m_pMenu;
  KStatusBar *m_pStatusBar;
  KToolBar* m_pToolbar;
  KToolBar* m_pLocationBar;
  KBookmarkMenu* m_pBookmarkMenu;
  QPopupMenu* m_pViewMenu;
  KURLCompletion* m_pCompletion;
  KPanner* m_pPanner;
  QWidget* m_pPannerChild0;
  QWidget* m_pPannerChild1;
  QGridLayout* m_pPannerChild0GM;
  QGridLayout* m_pPannerChild1GM;
  
  bool m_bShowToolBar;
  KToolBar::BarPosition m_toolBarPos;
  bool m_bShowStatusBar;
  KStatusBar::Position m_statusBarPos;
  bool m_bShowMenuBar;
  KMenuBar::menuPosition m_menuBarPos;
  bool m_bShowLocationBar;
  KToolBar::BarPosition m_locationBarPos;

  /**
   * The menu "New" in the "File" menu.
   * Since the items of this menu are not connected themselves
   * we need a pointer to this menu to get information about the
   * selected menu item.
   */
  KNewMenu *m_pMenuNew;

  KfmView* m_pView;
  KfmView* m_pView2;
  KfmView* m_currentView;
  KfmView::ViewMode m_viewMode;
  KfmView::ViewMode m_viewMode2;
  KfmView::ViewMode m_currentViewMode;
  
  MouseMode m_mouseMode;
  bool m_bShowDot;
  bool m_bShowDot2;
  bool m_bImagePreview;
  bool m_bImagePreview2;

  bool m_bDirTree;
  bool m_bSplitView;
  // bool m_bHTMLView;
  bool m_bCache;
  bool m_bChangeCursor;
  bool m_underlineLink;

  QString m_strStdFontName;
  QString m_strFixedFontName;
  int m_iFontSize;
  
  QColor m_bgColor;
  QColor m_textColor;
  QColor m_linkColor;
  QColor m_vLinkColor;

  QPixmap m_bgPixmap;
  
  /**
   * Set to true while the constructor is running.
   * @ref #initConfig needs to know about that.
   */
  bool m_bInit;

  unsigned int m_animatedLogoCounter;
  QTimer m_animatedLogoTimer;

  QString m_strUpURL;

  list<History> m_lstBack;
  list<History> m_lstForward;
  bool m_bBack;
  bool m_bForward;

  KAccel* m_pAccel;
  
  static QList<QPixmap>* s_lstAnimatedLogo;
  static QList<KfmGui>* s_lstWindows;
};

#endif
