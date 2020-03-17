/*
 * Copyright 2015 Ashish Bansal<bansal.ashish096@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef DBUSINTERFACE_H
#define DBUSINTERFACE_H

#include <QObject>

class DBusInterface : QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.freedesktop.FileManager1")

public:
    DBusInterface();
    Q_SCRIPTABLE void ShowFolders(const QStringList& uriList, const QString& startUpId);
    Q_SCRIPTABLE void ShowItems(const QStringList& uriList, const QString& startUpId);
    Q_SCRIPTABLE void ShowItemProperties(const QStringList& uriList, const QString& startUpId);

    /**
     * Set whether this interface has been created by dolphin --daemon.
     */
    void setAsDaemon();

    /**
     * @return Whether this interface has been created by dolphin --daemon.
     */
    bool isDaemon() const;

private:
    bool m_isDaemon = false;
};

#endif // DBUSINTERFACE_H
