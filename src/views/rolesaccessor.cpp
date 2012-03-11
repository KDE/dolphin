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

#include "rolesaccessor.h"

#include <KGlobal>
#include <KLocale>

class RolesAccessorSingleton
{
public:
    RolesAccessor instance;
};
K_GLOBAL_STATIC(RolesAccessorSingleton, s_rolesAccessor)

RolesAccessor& RolesAccessor::instance()
{
    return s_rolesAccessor->instance;
}

QList<QByteArray> RolesAccessor::roles() const
{
    return m_roles;
}

QString RolesAccessor::translation(const QByteArray& role) const
{
    return i18nc(m_translation[role]->roleTranslationContext, m_translation[role]->roleTranslation);
}

RolesAccessor::RolesAccessor() :
    m_roles(),
    m_translation()
{
    static const RolesAccessor::Translation translations[] = {
        // role          roleTranslationContext       roleTranslation
        { "name",        I18N_NOOP2_NOSTRIP("@label", "Name") },
        { "size",        I18N_NOOP2_NOSTRIP("@label", "Size") },
        { "date",        I18N_NOOP2_NOSTRIP("@label", "Date") },
        { "permissions", I18N_NOOP2_NOSTRIP("@label", "Permissions") },
        { "owner",       I18N_NOOP2_NOSTRIP("@label", "Owner") },
        { "group",       I18N_NOOP2_NOSTRIP("@label", "Group") },
        { "type",        I18N_NOOP2_NOSTRIP("@label", "Type") },
        { "destination", I18N_NOOP2_NOSTRIP("@label", "Link Destination") },
        { "path",        I18N_NOOP2_NOSTRIP("@label", "Path") }
    };

    for (unsigned int i = 0; i < sizeof(translations) / sizeof(Translation); ++i) {
        m_translation.insert(translations[i].role, &translations[i]);
        m_roles.append(translations[i].role);
    }
}

RolesAccessor::~RolesAccessor()
{
}
