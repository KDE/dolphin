/* This file is part of the KDE project
   Copyright 2000 Simon Hausmann <hausmann@kde.org>
   Copyright 2000-2006 David Faure <faure@kde.org>

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

#ifndef __KonquerorAdaptor_h__
#define __KonquerorAdaptor_h__

#include <QStringList>
#include <dbus/qdbus.h>

/**
 * DBus interface of a konqueror process
 */
class KonquerorAdaptor : public QDBusAbstractAdaptor
{
  Q_OBJECT
  Q_CLASSINFO("D-Bus Interface", "org.kde.Konqueror")

public:

  KonquerorAdaptor();
  ~KonquerorAdaptor();

public slots:

  /**
   * Opens a new window for the given @p url (using createSimpleWindow, i.e. a single view)
   * @param url the url to open
   * @param startup_id sets the application startup notification (ASN) property on the window, if not empty.
   * @return the DBUS object path of the window
   */
  QDBusObjectPath openBrowserWindow( const QString& url, const QByteArray& startup_id );

  /**
   * Opens a new window for the given @p url (using createNewWindow, i.e. with an appropriate profile)
   * @param url the url to open
   * @param mimetype pass the mimetype of the url, if known, to speed up the process.
   * @param startup_id sets the application startup notification (ASN) property on the window, if not empty.
   * @param tempFile whether to delete the file after use, usually this is false
   * @return the DBUS object path of the window
   */
  QDBusObjectPath createNewWindow( const QString& url, const QString& mimetype, const QByteArray& startup_id, bool tempFile );

  /**
   * Opens a new window like @ref createNewWindow, then selects the given @p filesToSelect
   * @param filesToSelect the files to select in the newly opened file-manager window
   * @param startup_id sets the application startup notification (ASN) property on the window, if not empty.
   * @return the DBUS object path of the window
   */
  QDBusObjectPath createNewWindowWithSelection( const QString& url, const QStringList& filesToSelect, const QByteArray& startup_id );

  /**
   * As the name says, this creates a window from a profile.
   * Used for instance by kfmclient.
   * @param path full path to the profile file
   * @param filename name of the profile file, if under the profiles dir (can be empty if not known, e.g. from khelpcenter)
   * @param startup_id sets the application startup notification (ASN) property on the window, if not empty.
   * @return the DBUS object path of the window
   */
  QDBusObjectPath createBrowserWindowFromProfile( const QString& path, const QString& filename,
                                                  const QByteArray& startup_id );

  /**
   * Creates a window from a profile and a URL.
   * Used by kfmclient to open http URLs with the webbrowsing profile
   * and others with the filemanagement profile.
   * @param path full path to the profile file
   * @param filename name of the profile file, if under the profiles dir
   * @param url the URL to open
   * @param startup_id sets the application startup notification (ASN) property on the window, if not empty.
   * @return the DBUS object path of the window
   */
  QDBusObjectPath createBrowserWindowFromProfileAndUrl( const QString& path, const QString& filename, const QString& url,
                                                        const QByteArray& startup_id );
  /**
   * Creates a window the fastest way : the caller has to provide
   * profile, URL, and mimetype.
   * @param path full path to the profile file
   * @param filename name of the profile file, if under the profiles dir
   * @param url the URL to open
   * @param mimetype the mimetype that the URL we want to open has
   * @param startup_id sets the application startup notification (ASN) property on the window, if not empty.
   * @return the DBUS object path of the window
   */
  QDBusObjectPath createBrowserWindowFromProfileUrlAndMimeType( const QString& path, const QString& filename,
                                                                const QString& url, const QString& mimetype,
                                                                const QByteArray& startup_id );

  /**
   * @return the name of the instance's crash log file
   */
  QString crashLogFile();

  /**
   * @return a list of references to all the windows
   */
  QList<QDBusObjectPath> getWindows();

  /**
   *  Called internally as broadcast when the user adds/removes/renames a view profile
    */
  Q_ASYNC void updateProfileList();

  /**
   * Called internally as broadcast when a URL is to be added to the combobox.
   */
  Q_ASYNC void addToCombo( const QString& url, const QDBusMessage& msg );

  /**
   * Called internall as broadcast when a URL has to be removed from the combo.
   */
  Q_ASYNC void removeFromCombo( const QString& url, const QDBusMessage& msg );

  /**
   * Called internally as a broadcast when the combobox was cleared.
   */
  Q_ASYNC void comboCleared( const QDBusMessage& msg );

  /**
   * Used by kfmclient when the 'minimize memory usage' setting is set
   * to find out if this konqueror can be used.
   */
  bool processCanBeReused( int screen );

  /**
   * Called from konqy_preloader to terminate this Konqueror instance,
   * if it's in the preloaded mode, and there are too many preloaded Konqy's
   */
  Q_ASYNC void terminatePreloaded();

Q_SIGNALS:
  /**
   * Emitted by kcontrol when the global configuration changes
   */
  void reparseConfiguration();
};

#endif
