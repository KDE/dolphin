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

#ifndef __konq_htmlsettings_h__
#define __konq_htmlsettings_h__

class KConfig;
#include <qcolor.h>
#include <qstring.h>

/**
 * Settings for the HTML veiw
 * We need this in konqueror since we need to call reparseConfiguration
 * on the static instance (even when we don't have an HTML view any longer).  
 */
class KonqHTMLSettings
{
protected:
  /**
   * @internal Constructor
   */
  KonqHTMLSettings();

  /** Called by constructor and reparseConfiguration */
  void init( KConfig * config );

  /** Destructor. Don't delete any instance by yourself. */
  virtual ~KonqHTMLSettings() {};

public:

  /**
   * The static instance of KonqHTMLSettings
   */
  static KonqHTMLSettings * defaultHTMLSettings();
  /**
   * Reparse the configuration to update the already-created instances
   */
  static void reparseConfiguration();

  // Behaviour settings
  bool changeCursor() { return m_bChangeCursor; }
  bool underlineLink() { return m_underlineLink; }

  // Font settings
  const QString& stdFontName() { return m_strStdFontName; }
  const QString& fixedFontName() { return m_strFixedFontName; }
  int fontSize() { return m_iFontSize; }

  // Color settings
  const QColor& bgColor() { return m_bgColor; }
  const QColor& textColor() { return m_textColor; }
  const QColor& linkColor() { return m_linkColor; }
  const QColor& vLinkColor() { return m_vLinkColor; }

  // Autoload images
  bool autoLoadImages() { return m_bAutoLoadImages; }

  // Java and JavaScript
  bool enableJava() { return m_bEnableJava; }
  bool enableJavaScript() { return m_bEnableJavaScript; }
  QString javaPath() { return m_strJavaPath; }

private:
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

  static KonqHTMLSettings * s_HTMLSettings;
private:
  // There is no copy constructors. Use the static method
  KonqHTMLSettings( const KonqHTMLSettings &);
};

#endif
