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
#include "kfmguiprops.h"

#include <qpixmap.h>
#include <qlist.h>
#include <qpopmenu.h>
#include <qpixmap.h>
#include <qtimer.h>
#include <qlayout.h>

#include <string>
#include <list>

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

  void setStatusBarText( const char *_text );
  void setLocationBarURL( const char *_url );
  void setUpURL( const char *_url );
  
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
  KfmView::ViewMode m_currentViewMode;
  
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
