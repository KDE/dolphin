/*  This file is part of the KDE project
    Copyright (C) 1997 David Faure <faure@kde.org>

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

#ifndef __kfm_viewprops_h__
#define __kfm_viewprops_h__

#include <qpixmap.h>

#include <kconfig.h>
#include <kurl.h>

class KInstance;
class KonqKfmIconView;
class KonqTreeViewWidget;

/**
 * The class KonqPropsView holds the properties for a Konqueror View
 *
 * Separating them from the view class allows to store the default
 * values (the one read from konquerorrc) in a static instance of this class
 * and to have another instance of this class in each view, storing the
 * current values of the view.
 *
 * The local values can be read from a desktop entry, if any (.directory, bookmark, ...).
 */
class KonqPropsView
{
  // A View can read/write the values directly.

  // This is not a QObject because we need a copy constructor.
public:

  /**
   * Constructs a KonqPropsView instance from an instance config file.
   * defaultProps is a "parent" object. If non null, then this instance
   * is the one used by a view, and its value can differ from the default ones.
   * The instance parameter should be the same for both...
   */
  KonqPropsView( KInstance * instance, KonqPropsView * defaultProps /*= 0L*/ );

  /** Destructor */
  virtual ~KonqPropsView();

  /**
   * Is this the instance representing default properties ?
   */
  bool isDefaultProperties() {
      // No parent -> we are the default properties
      return m_defaultProps == 0L;
  }

  /**
   * Called when entering a directory
   * Checks for a .directory, read it.
   */
  void enterDir( const KURL & dir );

#if 0
  /**
   * Save in local .directory if possible
   */
  void saveLocal( const KURL & dir );

  /**
   * Save those properties as default
   * ("Options"/"Save settings")  ["as default" missing ?]
   */
  void saveAsDefault( KInstance *instance );

  /**
   * Save to config file
   * Set the group before calling.
   * ("Settings" for global props, "ViewNNN" for SM (TODO),
   * "URL properties" for .directory files)
   */
  void saveProps( KConfig * config );
#endif

  void setSaveViewPropertiesLocally( bool value );


  void setShowingDotFiles( bool show );
  bool isShowingDotFiles() const { return m_bShowDot; }

  void setShowingImagePreview( bool show );
  bool isShowingImagePreview() const { return m_bImagePreview; }

  void setHTMLAllowed( bool allowed );
  bool isHTMLAllowed() const { return m_bHTMLAllowed; }

  // TODO : window size

  const QColor& bgColor() const { return m_bgColor; }
  const QPixmap& bgPixmap() const { return m_bgPixmap; }
  QColor m_bgColor;
  QPixmap m_bgPixmap;

protected:

  bool m_bShowDot;
  bool m_bImagePreview;
  bool m_bHTMLAllowed;

protected:

  QString dotDirectory;

  KConfigBase * currentConfig();

  QString currentGroup() {
      return isDefaultProperties() ? "Settings" : "URL properties";
  }

  bool m_bSaveViewPropertiesLocally;
  // Points to the current .directory file if we are in
  // save-view-properties-locally mode, otherwise to the global config
  // It is set to 0L to mark it as "needs to be constructed".
  // This is to be used for SAVING only.
  // Can be a KConfig or a KSimpleConfig
  KConfigBase * m_currentConfig;

  // If this is not a "default properties" instance (but one used by a view)
  // then m_defaultProps points to the "default properties" instance
  // Otherwise it's 0L.
  KonqPropsView * m_defaultProps;

private:

  KonqPropsView( const KonqPropsView & );
  KonqPropsView();
};


#endif
