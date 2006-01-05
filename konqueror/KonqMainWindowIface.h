/* This file is part of the KDE project
   Copyright (C) 2000 Simon Hausmann <hausmann@kde.org>
   Copyright (C) 2000 David Faure <faure@kde.org>

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#ifndef __KonqMainWindowIface_h__
#define __KonqMainWindowIface_h__

#include <dcopobject.h>
#include <dcopref.h>
#include <kmainwindowiface.h>

class KonqMainWindow;
class KDCOPActionProxy;

/**
 * DCOP interface for a konqueror main window
 */
class KonqMainWindowIface : public KMainWindowInterface
{
  K_DCOP
public:

  KonqMainWindowIface( KonqMainWindow * mainWindow );
  ~KonqMainWindowIface();

k_dcop:

  void openURL( QString url );
  void newTab( QString url );

  void openURL( QString url, bool tempFile );
  void newTab( QString url, bool tempFile );

  void newTabASN( QString url, const DCOPCString& startup_id, bool tempFile );

  /**
   * Reloads the current view.
   */
  void reload();

  /**
   * @return reference to the current KonqView
   */
  DCOPRef currentView();
  /**
   * @return reference to the current part
   */
  DCOPRef currentPart();

  DCOPRef action( const DCOPCString &name );
  DCOPCStringList actions();
  QMap<DCOPCString,DCOPRef> actionMap();

  /**
   * Used by kfmclient when searching a window to open a tab within
   */
  bool windowCanBeUsedForTab();

public:
    virtual DCOPCStringList functionsDynamic();
    virtual bool processDynamic( const DCOPCString &fun, const QByteArray &data, DCOPCString &replyType, QByteArray &replyData );

private:

  KonqMainWindow * m_pMainWindow;
  KDCOPActionProxy *m_dcopActionProxy;

};

#endif

