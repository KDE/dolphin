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

#include <QHash>
#include <QSet>
#include <QUrl>

class QUrl;

/**
 * @brief Allows accessing metadata of a file by providing KFileItemModel roles.
 *
 * Is a helper class for KFileItemModelRolesUpdater to retrieve roles that
 * are only accessible with Nepomuk.
 */
class KNepomukRolesProvider
{
public:
    static KNepomukRolesProvider& instance();
    virtual ~KNepomukRolesProvider();

    /**
     * @return True if the values of the role can be determined by Nepomuk.
     */
    bool isNepomukRole(const QByteArray& role) const;

    /**
     * @return Values for the roles \a roles that can be determined from the file
     *         with the URL \a url.
     */
    QHash<QByteArray, QVariant> roleValues(const QUrl& url, const QSet<QByteArray>& roles) const;

protected:
    KNepomukRolesProvider();

private:
    QString tagsFromValues(const QStringList& values) const;

private:
    QSet<QByteArray> m_roles;
    QHash<QUrl, QByteArray> m_roleForUri;

    friend class KNepomukRolesProviderSingleton;
};

#endif

