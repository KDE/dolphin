/***************************************************************************
 *   Copyright (C) 2012 by Peter Penz <peter.penz19@gmail.com>             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA            *
 ***************************************************************************/

#ifndef KNEPOMUKROLESPROVIDER_H
#define KNEPOMUKROLESPROVIDER_H

#include <libdolphin_export.h>

#include <QHash>
#include <QSet>
#include <QUrl>

namespace Nepomuk2
{
    class Resource;
}

/**
 * @brief Allows accessing metadata of a file by providing KFileItemModel roles.
 *
 * Is a helper class for KFileItemModelRolesUpdater to retrieve roles that
 * are only accessible with Nepomuk.
 */
class LIBDOLPHINPRIVATE_EXPORT KNepomukRolesProvider
{
public:
    static KNepomukRolesProvider& instance();
    virtual ~KNepomukRolesProvider();

    /**
     * @return Roles that can be provided by KNepomukRolesProvider.
     */
    QSet<QByteArray> roles() const;

    /**
     * @return Values for the roles \a roles that can be determined from the file
     *         with the URL \a url.
     */
    QHash<QByteArray, QVariant> roleValues(const Nepomuk2::Resource& resource,
                                           const QSet<QByteArray>& roles) const;

protected:
    KNepomukRolesProvider();

private:
    /**
     * @return User visible string for the given tag-values.
     */
    QString tagsFromValues(const QStringList& values) const;

    /**
     * @return User visible string for the EXIF-orientation property
     *         which can have the values 0 to 8.
     *         (see http://sylvana.net/jpegcrop/exif_orientation.html)
     */
    QString orientationFromValue(int value) const;

    /**
     * @return Duration in the format HH::MM::SS for the value given
     *         in milliseconds.
     */
    QString durationFromValue(int value) const;

private:
    QSet<QByteArray> m_roles;
    QHash<QUrl, QByteArray> m_roleForUri;

    friend class KNepomukRolesProviderSingleton;
};

#endif

