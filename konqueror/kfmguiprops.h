/* This file is part of the KDE project
   Copyright (C) 1999 Faure David <faure@kde.org>
 
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

#ifndef __kfm_guiprops_h__
#define __kfm_guiprops_h__

#include "kfmview.h"
#include "kconfig.h"
#include "kfm_abstract_gui.h"

#include <ktoolbar.h>
#include <kmenubar.h>
#include <kstatusbar.h>

#include <qcolor.h>
#include <qpixmap.h>

// This class holds the properties for a KfmGui (i.e. a window).
//
// Separating them from the KfmGui class allows to store the default
// values (the one read from kfmrc) in a static instance of this class
// and to have another instance of this class in each KfmGui, storing the
// current values of the window.
//
// If necessary, the local values can be read from a local kdelnk file.

class KfmGuiProps  // : public QObject
{
    // Q_OBJECT No copy constructor in QObject. Forget it.
public:
  
  // The static instance of KfmGuiProps, holding the default values
  // from the config file
  static KfmGuiProps * defaultProps;
  
  // Constructs a new KfmGuiProps instance with values taken
  // from defaultProps. Done for each new window.
  // KfmGuiProps( ); NO. Done with the copy constructor.

  // Constructs a KfmGuiProps instance from a config file.
  // Set the group before calling.
  // TODO : will have to be called on slotConfigure
  KfmGuiProps( const KConfig * config );

  // Destructor
  virtual ~KfmGuiProps();

  // Save to config file
  void saveProps( KConfig * config );
  
  KfmView::ViewMode viewMode() { return m_viewMode; }  
  KfmView::ViewMode viewMode2() { return m_viewMode2; }
  KfmAbstractGui::MouseMode mouseMode() { return m_mouseMode; }
  
  const QColor& bgColor() { return m_bgColor; }
  const QColor& textColor() { return m_textColor; }
  const QColor& linkColor() { return m_linkColor; }
  const QColor& vLinkColor() { return m_vLinkColor; }

  bool isShowingDotFiles() { return m_bShowDot; } // what about m_ShowDot2 ?
  bool isShowingImagePreview() { return m_bImagePreview; } // same question ?
  bool isShowingDirTree() { return m_bDirTree; }
  bool isSplitView() { return m_bSplitView; }
  // TODO cache
  bool changeCursor() { return m_bChangeCursor; }
  bool underlineLink() { return m_underlineLink; }
  
  const char* stdFontName() { return m_strStdFontName; }
  const char* fixedFontName() { return m_strFixedFontName; }
  int fontSize() { return m_iFontSize; }

  const QPixmap& bgPixmap() { return m_bgPixmap; }  

  // TODO *bar stuff

  int width() { return m_width; }
  int height() { return m_height; }

  // A KfmGui can change the values directly.
  friend class KfmGui;
  
protected:
  // The members. Same order as the retrieval methods above. Keep order !
  
  KfmView::ViewMode m_viewMode;
  KfmView::ViewMode m_viewMode2;
  KfmAbstractGui::MouseMode m_mouseMode;

  QColor m_bgColor;
  QColor m_textColor;
  QColor m_linkColor;
  QColor m_vLinkColor;

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
  
  QPixmap m_bgPixmap;

  bool m_bShowToolBar;
  KToolBar::BarPosition m_toolBarPos;
  bool m_bShowStatusBar;
  KStatusBar::Position m_statusBarPos;
  bool m_bShowMenuBar;
  KMenuBar::menuPosition m_menuBarPos;
  bool m_bShowLocationBar;
  KToolBar::BarPosition m_locationBarPos;

  int m_width;
  int m_height;
};

#endif
