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

#ifndef __konq_propsmainview_h
#define __konq_propsmainview_h

#include <kconfig.h>

#include <ktoolbar.h>
#include <kmenubar.h>
#include <kstatusbar.h>

/**
 * @short This class holds the properties for a KfmGui (i.e. a window).
 *
 * Separating them from the KfmGui class allows to store the default
 * values (the one read from kfmrc) in a static instance of this class
 * and to have another instance of this class in each KfmGui, storing the
 * current values of the window.
 *
 * The local values can be read from a kdelnk file, if any (.directory, bookmark, ...).
 */
class KonqPropsMainView
{
  // This is not a Q_OBJECT because we need a copy constructor.
public:
  friend class KonqMainView;
  
  /**
   * To construct a new KonqPropsMainView instance with values taken
   * from defaultProps, use the copy constructor.
   *
   * Constructs a KonqPropsMainView instance from a config file.
   * Set the group before calling.
   * ("Settings" for global props, "URL properties" in local props)
   * TODO : will have to be called on slotConfigure
   * TODO : will have to be called for local properties
   */
  KonqPropsMainView( const KConfig * config );

  /**
   * Destructor
   */
  virtual ~KonqPropsMainView();

  /**
   * Save to config file
   * Set the group before calling.
   * ("Settings" for global props, "URL properties" in local props)
   */
  void saveProps( KConfig * config );

  //////// The read-only access methods. Order is to be kept. See below. /////

  bool isSplitView() const { return m_bSplitView; }
  
  // No *bar access methods (all done from KonqMainView)
  // No width/height access methods either.

  /**
   * The static instance of KonqPropsMainView, holding the default values
   * from the config file
   */
  static KonqPropsMainView * defaultProps() { return m_pDefaultProps; }
  
private:
  // There is no default constructor. Use the provided one or copy constructor
  KonqPropsMainView();

  bool m_bSplitView;

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

  /**
   * The static instance. Only KonqMainView can change its value.
   */
  static KonqPropsMainView * m_pDefaultProps;
};

#endif
