/*  This file is part of the KDE project
    Copyright (C) 1998-2000 David Faure <faure@kde.org>

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

#ifndef __konq_propsmainview_h
#define __konq_propsmainview_h

#include <kconfig.h>

#include <ktoolbar.h>
#include <kmenubar.h>
#include <kstatusbar.h>

/**
 * @short This class holds the properties for a KonqMainView (i.e. a window).
 *
 * Separating them from the KonqMainView class allows to store the default
 * values (the one read from konquerorrc) in a static instance of this class
 * and to have another instance of this class in each KonqMainView, storing the
 * current values of the window.
 *
 * The local values can be read from a desktop entry, if any (.directory, bookmark, ...).
 */
class KonqPropsMainView
{
  // This is not a Q__OBJECT because we need a copy constructor.
public:

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
  KonqPropsMainView( KConfig * config );

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

  int width() { return m_width; }
  int height() { return m_height; }

  // for each toggable view, on or off...

  QStringList toggableViewsShown();

  /**
   * The static instance of KonqPropsMainView, holding the default values
   * from the config file
   */
  static KonqPropsMainView * defaultProps() { return m_pDefaultProps; }

private:
  // There is no default constructor. Use the provided one or copy constructor
  KonqPropsMainView();

  /* Handled by KTMainWindow - except for save default settings....
  bool m_bShowToolBar;
  KToolBar::BarPosition m_toolBarPos;
  bool m_bShowStatusBar;
  KStatusBar::Position m_statusBarPos;
  bool m_bShowMenuBar;
  KMenuBar::menuPosition m_menuBarPos;
  bool m_bShowLocationBar;
  KToolBar::BarPosition m_locationBarPos;
  */

  int m_width;
  int m_height;

  /**
   * The static instance. Only KonqMainView can change its value.
   */
  static KonqPropsMainView * m_pDefaultProps;
};

#endif
