/***************************************************************************
 *   Copyright (C) 2012 by Peter Penz <peter.penz19@gmail.com>             *
 *   Copyright (C) 2013 by Vishesh Handa <me@vhanda.in>                    *
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

#ifndef KBALOO_ROLESPROVIDER_H
#define KBALOO_ROLESPROVIDER_H

#include "dolphin_export.h"

#include <QHash>
#include <QSet>
#include <QVariant>

namespace Baloo {
    class File;
}

/**
 * @brief Allows accessing metadata of a file by providing KFileItemModel roles.
 *
 * Is a helper class for KFileItemModelRolesUpdater to retrieve roles that
 * are only accessible with Baloo.
 */
class DOLPHIN_EXPORT KBalooRolesProvider
{
public:
    static KBalooRolesProvider& instance();
    virtual ~KBalooRolesProvider();

    /**
     * @return Roles that can be provided by KBalooRolesProvider.
     */
    QSet<QByteArray> roles() const;

    /**
     * @return Values for the roles \a roles that can be determined from the file
     *         with the URL \a url.
     */
    QHash<QByteArray, QVariant> roleValues(const Baloo::File& file,
                                           const QSet<QByteArray>& roles) const;

    QByteArray roleForProperty(const QString& property) const;

protected:
    KBalooRolesProvider();

private:
    /**
     * @return User visible string for the given tag-values.
     *         The tag-values are sorted in alphabetical order.
     */
    QString tagsFromValues(const QStringList& values) const;

private:
    QSet<QByteArray> m_roles;
    QHash<QString, QByteArray> m_roleForProperty;

    friend struct KBalooRolesProviderSingleton;
};

#endif

