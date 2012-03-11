/***************************************************************************
 *   Copyright (C) 2010 by Peter Penz <peter.penz19@gmail.com>             *
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

#ifndef ROLESACCESSOR_H
#define ROLESACCESSOR_H

#include <libdolphin_export.h>

#include <QList>
#include <QHash>

/**
 * @brief Allows to access the available roles that can be shown in a view.
 */
class LIBDOLPHINPRIVATE_EXPORT RolesAccessor
{
public:
    static RolesAccessor& instance();

    /**
     * @return List of all available roles.
     */
    QList<QByteArray> roles() const;

    /**
     * @return Translation of the role that can be shown e.g. in the header
     *         of a view or as menu-entry.
     */
    QString translation(const QByteArray& role) const;

protected:
    RolesAccessor();
    virtual ~RolesAccessor();
    friend class RolesAccessorSingleton;

private:
    struct Translation {
        const char* const role;
        const char* const roleTranslationContext;
        const char* const roleTranslation;
    };

    QList<QByteArray> m_roles;
    QHash<QByteArray, const Translation*> m_translation;
};

#endif

