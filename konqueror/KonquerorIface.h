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
   the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.
*/

#ifndef __KonquerorIface_h__
#define __KonquerorIface_h__

#include <dcopobject.h>
#include <qvaluelist.h>
#include <dcopref.h>

/**
 * DCOP interface for konqueror
 */
class KonquerorIface : virtual public DCOPObject
{
  K_DCOP
public:

  KonquerorIface();
  ~KonquerorIface();

k_dcop:
  /**
   * Opens a new window for the given @p url (using createSimpleWindow, i.e. a single view)
   */
  void openBrowserWindow( const QString &url );

  /**
   * Opens a new window for the given @p url (using createNewWindow, i.e. with an appropriate profile)
   */
  void createNewWindow( const QString &url );

  /**
   * Opens a new window for the given @p url (using createNewWindow, i.e. with an appropriate profile)
   * @param mimetype to speed it up.
   */
  void createNewWindow( const QString &url, const QString & mimetype );

  /**
   * As the name says, this creates a window from a profile.
   * Used for instance by khelpcenter.
   */
  void createBrowserWindowFromProfile( const QString &path );

  /**
   * As the name says, this creates a window from a profile.
   * Used for instance by kfmclient.
   * @param path full path to the profile file
   * @param filename name of the profile file, if under the profiles dir
   */
  void createBrowserWindowFromProfile( const QString &path, const QString &filename );

  /**
   * Creates a window from a profile and a URL.
   * Used by kfmclient to open http URLs with the webbrowsing profile
   * and others with the filemanagement profile.
   * @param path full path to the profile file
   * @param filename name of the profile file, if under the profiles dir
   * @param url the URL to open
   */
  void createBrowserWindowFromProfileAndURL( const QString &path, const QString &filename, const QString &url );

  /**
   * Creates a window the fastest way : the caller has to provide
   * profile, URL, and mimetype.
   * @param path full path to the profile file
   * @param filename name of the profile file, if under the profiles dir
   * @param url the URL to open
   * @param mimetype the mimetype that the URL we want to open has
   */
  void createBrowserWindowFromProfileAndURL( const QString &path, const QString &filename, const QString &url, const QString &mimetype );

  /**
   * Called by kcontrol when the global configuration changes
   */
  ASYNC reparseConfiguration();

  /**
   * @return a list of references to all the windows
   */
  QValueList<DCOPRef> getWindows();

  /**
   *  Called internally as broadcast when the user adds/removes/renames a view profile
    */
  ASYNC updateProfileList();
};

#endif

