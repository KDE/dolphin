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

#ifndef __kfm_viewprops_h__
#define __kfm_viewprops_h__

#include "kfm_abstract_gui.h"
#include "kfmview.h"

#include <qcolor.h>
#include <qpixmap.h>

#include <kconfig.h>

class KfmViewSettings; // see below

// The class KfmViewProps holds the properties for a KfmView
//
// Separating them from the KfmView class allows to store the default
// values (the one read from kfmrc) in a static instance of this class
// and to have another instance of this class in each KfmView, storing the
// current values of the view.
//
// The local values can be read from a kdelnk file, if any (.directory, bookmark, ...).

class KfmViewProps
{
  // This is not a Q_OBJECT because we need a copy constructor.
public:
  
  // The static instance of KfmViewProps, holding the default values
  // from the config file
  static KfmViewProps * defaultProps() { return m_pDefaultProps; }
  
  // To construct a new KfmViewProps instance with values taken
  // from defaultProps, use the copy constructor.
  
  // Constructs a KfmViewProps instance from a config file.
  // Set the group before calling.
  // ("Settings" for global props, "ViewNNN" for local props)
  // TODO : will have to be called on slotConfigure
  // TODO : will have to be called for local properties
  KfmViewProps( const KConfig * config );

  // Destructor
  virtual ~KfmViewProps();

  // Save to config file
  // Set the group before calling.
  // ("Settings" for global props, "ViewNNN" for local props)
  void saveProps( KConfig * config );

  //////// The read-only access methods. Order is to be kept. /////

  KfmView::ViewMode viewMode() { return m_viewMode; }
  bool isShowingDotFiles() { return m_bShowDot; }
  bool isShowingImagePreview() { return m_bImagePreview; }
  // HTMLView ?
  // Cache ?

  // A KfmView can read/write the values directly.
  friend class KfmView;

protected:

  // The static instance. Only KfmView can change its value.
  static KfmViewProps * m_pDefaultProps;

  KfmView::ViewMode m_viewMode;
  bool m_bShowDot;
  bool m_bImagePreview;
  // bool m_bHTMLView; ?
  // bool m_bCache; ?

private:

  // There is no default constructor. Use the provided one or copy constructor
  KfmViewProps::KfmViewProps();
};

// The class KfmViewSettings holds the general settings, that apply to
// either all HTML views or all FileManager views.
// -> only two instances of this class are created.
// There is no 'local' instance of it.
// Well, in fact there could even be also one per FileManager mode (long, short, tree...)

class KfmViewSettings
{
  // This is not a Q_OBJECT, even if we don't need a copy constructor. :)
public:
    
  // A static instance of KfmViewSettings, holding the values for FileManager
  static KfmViewSettings * defaultFMSettings() { return m_pDefaultFMSettings; }
  // A static instance of KfmViewSettings, holding the values for HTML
  static KfmViewSettings * defaultHTMLSettings() { return m_pDefaultHTMLSettings; }

  // Constructs a KfmViewSettings instance from a config file.
  // Set the group before calling.
  // ("Settings" for global props, "URL properties" in local props)
  // TODO : will have to be called on slotConfigure
  KfmViewSettings( const KConfig * config );

  // Destructor
  virtual ~KfmViewSettings();

  // Save to config file
  // Set the group before calling.
  // (e.g. "KFM HTML Defaults" and "KFM FM Defaults" groups)
  void saveProps( KConfig * config );

  //////// The read-only access methods. Order is to be kept. See below. /////

  const char* stdFontName() { return m_strStdFontName; }
  const char* fixedFontName() { return m_strFixedFontName; }
  int fontSize() { return m_iFontSize; }

  bool changeCursor() { return m_bChangeCursor; }
  KfmAbstractGui::MouseMode mouseMode() { return m_mouseMode; }

  const QColor& bgColor() { return m_bgColor; }
  const QColor& textColor() { return m_textColor; }
  const QColor& linkColor() { return m_linkColor; }
  const QColor& vLinkColor() { return m_vLinkColor; }

  bool underlineLink() { return m_underlineLink; }

  friend class KfmView; // should be used only for setting the two static instances,
  // not for changing other member values.
  
protected:
  static KfmViewSettings * m_pDefaultFMSettings;
  static KfmViewSettings * m_pDefaultHTMLSettings;
  
  QString m_strStdFontName;
  QString m_strFixedFontName;
  int m_iFontSize;  
  
  bool m_bChangeCursor;
  KfmAbstractGui::MouseMode m_mouseMode;
  QColor m_bgColor;
  QColor m_textColor;
  QColor m_linkColor;
  QColor m_vLinkColor;
  bool m_underlineLink;

private:
  // There is no default constructor. Use the provided one.
  KfmViewSettings::KfmViewSettings();
  // No copy constructor either. What for ?
  KfmViewSettings::KfmViewSettings( const KfmViewSettings &);
};

#endif
