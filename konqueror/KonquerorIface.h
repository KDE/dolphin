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
   Note : it would be very nice to be able to return a DCOPRef in each of those
   DCOP calls which create a window. But Konqueror uses DCOP internally
   (history broadcasts etc.), and DCOP doesn't support re-entrancy, so we can't
   do that. Sorry.
  */

  /**
   * Opens a new window for the given @p url (using createSimpleWindow, i.e. a single view)
   */
  ASYNC openBrowserWindow( const QString &url );
  /**
   * Like @ref openBrowserWindow , with setting the application startup notification ( ASN )
   * property on the window.
   */
  ASYNC openBrowserWindowASN( const QString &url, const QCString &startup_id );

  /**
   * Opens a new window for the given @p url (using createNewWindow, i.e. with an appropriate profile)
   */
  ASYNC createNewWindow( const QString &url );
  /**
   * Like @ref createNewWindow , with setting the application startup notification ( ASN )
   * property on the window.
   */
  ASYNC createNewWindowASN( const QString &url, const QCString &startup_id );

  /**
   * Opens a new window for the given @p url (using createNewWindow, i.e. with an appropriate profile)
   * @param mimetype to speed it up.
   */
  ASYNC createNewWindow( const QString &url, const QString & mimetype );
  /**
   * Like @ref createNewWindow , with setting the application startup notification ( ASN )
   * property on the window.
   */
  ASYNC createNewWindowASN( const QString &url, const QString & mimetype,
      const QCString &startup_id );

  /**
   * As the name says, this creates a window from a profile.
   * Used for instance by khelpcenter.
   */
  ASYNC createBrowserWindowFromProfile( const QString &path );
  /**
   * Like @ref createBrowserWindowFromProfile , with setting the application startup
   * notification ( ASN ) property on the window.
   */
  ASYNC createBrowserWindowFromProfileASN( const QString &path, const QCString &startup_id );

  /**
   * As the name says, this creates a window from a profile.
   * Used for instance by kfmclient.
   * @param path full path to the profile file
   * @param filename name of the profile file, if under the profiles dir
   */
  ASYNC createBrowserWindowFromProfile( const QString &path, const QString &filename );
  /**
   * Like @ref createBrowserWindowFromProfile , with setting the application startup
   * notification ( ASN ) property on the window.
   */
  ASYNC createBrowserWindowFromProfileASN( const QString &path, const QString &filename,
      const QCString &startup_id );

  /**
   * Creates a window from a profile and a URL.
   * Used by kfmclient to open http URLs with the webbrowsing profile
   * and others with the filemanagement profile.
   * @param path full path to the profile file
   * @param filename name of the profile file, if under the profiles dir
   * @param url the URL to open
   */
  ASYNC createBrowserWindowFromProfileAndURL( const QString &path, const QString &filename, const QString &url );
  /**
   * Like @ref createBrowserWindowFromProfileAndURL , with setting the application startup
   * notification ( ASN ) property on the window.
   */
  ASYNC createBrowserWindowFromProfileAndURLASN( const QString &path, const QString &filename, const QString &url,
      const QCString &startup_id );

  /**
   * Creates a window the fastest way : the caller has to provide
   * profile, URL, and mimetype.
   * @param path full path to the profile file
   * @param filename name of the profile file, if under the profiles dir
   * @param url the URL to open
   * @param mimetype the mimetype that the URL we want to open has
   */
  ASYNC createBrowserWindowFromProfileAndURL( const QString &path, const QString &filename, const QString &url, const QString &mimetype );
  /**
   * Like @ref createBrowserWindowFromProfileAndURL , with setting the application startup
   * notification ( ASN ) property on the window.
   */
  ASYNC createBrowserWindowFromProfileAndURLASN( const QString &path, const QString &filename, const QString &url, const QString &mimetype,
      const QCString& startup_id );

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

  /**
   * Called internally as broadcast when a URL is to be added to the combobox.
   */
  ASYNC addToCombo( QString, QCString );

  /**
   * Called internall as broadcast when a URL has to be removed from the combo.
   */
  ASYNC removeFromCombo( QString, QCString );
    
  /**
   * Called internally as a broadcast when the combobox was cleared.
   */
  ASYNC comboCleared( QCString );

};

#endif

