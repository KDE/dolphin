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

#include "mousemode.h"

#include <qcolor.h>
#include <qpixmap.h>

#include <kconfig.h>

class KfmViewSettings; // see below

class KonqBaseView;
class KonqKfmIconView;

// The class KonqPropsView holds the properties for a KonqBaseView
//
// Separating them from the KonqBaseView class allows to store the default
// values (the one read from kfmrc) in a static instance of this class
// and to have another instance of this class in each KonqBaseView, storing the
// current values of the view.
//
// The local values can be read from a desktop entry, if any (.directory, bookmark, ...).

class KonqPropsView
{
  // A BaseView can read/write the values directly.
  friend KonqBaseView;
  friend KonqKfmIconView; // seems it doesn't inherit "friendliness" !

  // This is not a Q_OBJECT because we need a copy constructor.
public:
  
  // The static instance of KonqPropsView, holding the default values
  // from the config file
  static KonqPropsView * defaultProps();
  
  // To construct a new KonqPropsView instance with values taken
  // from defaultProps, use the copy constructor.
  
  // Constructs a KonqPropsView instance from a config file.
  // Set the group before calling.
  // ("Settings" for global props, "ViewNNN" for local props)
  // TODO : will have to be called for local properties
  KonqPropsView( const KConfig * config );

  // Destructor
  virtual ~KonqPropsView();

  // Save to config file
  // Set the group before calling.
  // ("Settings" for global props, "ViewNNN" for local props)
  void saveProps( KConfig * config );

  //////// The read-only access methods. Order is to be kept. /////

//  KonqBaseView::ViewMode viewMode() { return m_viewMode; }
  bool isShowingDotFiles() { return m_bShowDot; }
  bool isShowingImagePreview() { return m_bImagePreview; }
  bool isHTMLAllowed() { return m_bHTMLAllowed; }
  // Cache ?
  const QPixmap& bgPixmap() { return m_bgPixmap; }
  
protected:

  // The static instance. Only KonqBaseView can change its value.
  static KonqPropsView * m_pDefaultProps;

//  KonqBaseView::ViewMode m_viewMode;
  bool m_bShowDot;
  bool m_bImagePreview;
  bool m_bHTMLAllowed;
  // bool m_bCache; ?
  QPixmap m_bgPixmap; // one per view or one per GUI ?

private:

  // There is no default constructor. Use the provided one or copy constructor
  KonqPropsView();
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
  static KfmViewSettings * defaultFMSettings();
  // A static instance of KfmViewSettings, holding the values for HTML
  static KfmViewSettings * defaultHTMLSettings();

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
  MouseMode mouseMode() { return m_mouseMode; }

  const QColor& bgColor() { return m_bgColor; }
  const QColor& textColor() { return m_textColor; }
  const QColor& linkColor() { return m_linkColor; }
  const QColor& vLinkColor() { return m_vLinkColor; }

  bool underlineLink() { return m_underlineLink; }

  bool autoLoadImages() { return m_bAutoLoadImages; }

protected:
  static KfmViewSettings * m_pDefaultFMSettings;
  static KfmViewSettings * m_pDefaultHTMLSettings;
  
  QString m_strStdFontName;
  QString m_strFixedFontName;
  int m_iFontSize;  
  
  bool m_bChangeCursor;
  MouseMode m_mouseMode;
  QColor m_bgColor;
  QColor m_textColor;
  QColor m_linkColor;
  QColor m_vLinkColor;
  bool m_underlineLink;
  
  bool m_bAutoLoadImages;

private:
  // There is no default constructor. Use the provided one.
  KfmViewSettings();
  // No copy constructor either. What for ?
  KfmViewSettings( const KfmViewSettings &);
};

#endif
