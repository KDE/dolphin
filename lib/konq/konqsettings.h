/* This file is part of the KDE project
   Copyright (C) 1999 David Faure <faure@kde.org>

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

#ifndef __konq_settings_h__
#define __konq_settings_h__

class KConfig;
#include <qcolor.h>
#include <qstring.h>

/**
 * The class KonqSettings holds the general settings for konqueror/kdesktop.
 * There is no 'local' (per-URL) instance of it.
 * All those settings can only be changed in kcmkonq.
 *
 * For advanced customization, there can be one instance
 * all HTML views, one for all FileManager views, and one for the desktop.
 *
 * Current limitation : html and fm views use the same group in konquerorrc
 * since kcmkonq doesn't allow to set those separately at the moment.
 *
 * <vapor>
 * Or even one for icon view and one for tree view, ...
 * But then all that has to be done in kcmkonq as well.
 * </vapor>
 */

class KonqSettings
{
protected:
  /** @internal
   * Constructs a KonqSettings instance from a config file.
   * Set the group before calling.
   * (done by defaultFMSettings and defaultHTMLSettings)
   */
  KonqSettings( KConfig * config );

  /** Called by constructor and reparseConfiguration */
  void init( KConfig * config );

  /** Destructor. Don't delete any instance by yourself. */
  virtual ~KonqSettings();

public:

  /**
   * A static instance of KonqSettings, holding the values for KDesktop
   */
  static inline KonqSettings * defaultDesktopSettings();
  /**
   * A static instance of KonqSettings, holding the values for FileManager
   */
  static inline KonqSettings * defaultFMSettings();
  /**
   * A static instance of KonqSettings, holding the values for HTML
   */
  static inline KonqSettings * defaultHTMLSettings();

  /**
   * Reparse the configuration to update the already-created instances
   */
  static void reparseConfiguration();


  // Behaviour settings
  bool singleClick() { return m_bSingleClick; }
  int autoSelect() { return m_iAutoSelect; }
  bool changeCursor() { return m_bChangeCursor; }
  bool underlineLink() { return m_underlineLink; }

  // Font settings
  const char* stdFontName() { return m_strStdFontName; }
  const char* fixedFontName() { return m_strFixedFontName; }
  int fontSize() { return m_iFontSize; }

  // Color settings
  const QColor& bgColor() { return m_bgColor; }
  const QColor& textColor() { return m_textColor; }
  const QColor& linkColor() { return m_linkColor; }
  const QColor& vLinkColor() { return m_vLinkColor; }

  // Autoload images (only means something for HMTL configuration)
  bool autoLoadImages() { return m_bAutoLoadImages; }

    // Java and JavaScript (only for HTML)
    bool enableJava() { return m_bEnableJava; }
    bool enableJavaScript() { return m_bEnableJavaScript; }
    QString javaPath() { return m_strJavaPath; }
 
    
protected:
  // The three instances
  static KonqSettings * s_pSettings[3];

  static KonqSettings * getInstance( int nr );

  bool m_bSingleClick;
  int m_iAutoSelect;
  bool m_bChangeCursor;
  bool m_underlineLink;

  QString m_strStdFontName;
  QString m_strFixedFontName;
  int m_iFontSize;

  QColor m_bgColor;
  QColor m_textColor;
  QColor m_linkColor;
  QColor m_vLinkColor;

  bool m_bAutoLoadImages;
    bool m_bEnableJava;
    bool m_bEnableJavaScript;
    QString m_strJavaPath;

private:
  // There is no default constructor. Use the provided ones.
  KonqSettings();
  // No copy constructor either. What for ?
  KonqSettings( const KonqSettings &);
};

#endif
