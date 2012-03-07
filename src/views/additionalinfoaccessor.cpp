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

#include "additionalinfoaccessor.h"

#include <KGlobal>
#include <KLocale>

class AdditionalInfoAccessorSingleton
{
public:
    AdditionalInfoAccessor instance;
};
K_GLOBAL_STATIC(AdditionalInfoAccessorSingleton, s_additionalInfoManager)

AdditionalInfoAccessor& AdditionalInfoAccessor::instance()
{
    return s_additionalInfoManager->instance;
}

QList<DolphinView::AdditionalInfo> AdditionalInfoAccessor::keys() const
{
    return m_map.keys();
}

QByteArray AdditionalInfoAccessor::role(DolphinView::AdditionalInfo info) const
{
    return m_map[info]->role;
}

DolphinView::AdditionalInfo AdditionalInfoAccessor::additionalInfo(const QByteArray& role) const
{
    return m_infoForRole.value(role);
}

QString AdditionalInfoAccessor::actionCollectionName(DolphinView::AdditionalInfo info,
                                                     ActionCollectionType type) const
{
    QString name;
    switch (type) {
    case SortByType:
        name = QLatin1String("sort_by_") + QLatin1String(m_map[info]->role);
        break;

    case AdditionalInfoType:
        name = QLatin1String("show_") + QLatin1String(m_map[info]->role);
        break;
    }

    return name;
}

QString AdditionalInfoAccessor::translation(DolphinView::AdditionalInfo info) const
{
    return i18nc(m_map[info]->roleTranslationContext, m_map[info]->roleTranslation);
}

QString AdditionalInfoAccessor::value(DolphinView::AdditionalInfo info) const
{
    return m_map[info]->value;
}

DolphinView::Sorting AdditionalInfoAccessor::sorting(DolphinView::AdditionalInfo info) const
{
    return m_map[info]->sorting;
}

AdditionalInfoAccessor::AdditionalInfoAccessor() :
    m_map(),
    m_infoForRole()
{
    static const AdditionalInfoAccessor::AdditionalInfo additionalInfo[] = {
        // role          roleTranslationContext       roleTranslation      value              sorting
        { "size",        I18N_NOOP2_NOSTRIP("@label", "Size"),             "Size",            DolphinView::SortBySize},
        { "date",        I18N_NOOP2_NOSTRIP("@label", "Date"),             "Date",            DolphinView::SortByDate},
        { "permissions", I18N_NOOP2_NOSTRIP("@label", "Permissions"),      "Permissions",     DolphinView::SortByPermissions},
        { "owner",       I18N_NOOP2_NOSTRIP("@label", "Owner"),            "Owner",           DolphinView::SortByOwner},
        { "group",       I18N_NOOP2_NOSTRIP("@label", "Group"),            "Group",           DolphinView::SortByGroup},
        { "type",        I18N_NOOP2_NOSTRIP("@label", "Type"),             "Type",            DolphinView::SortByType},
        { "destination", I18N_NOOP2_NOSTRIP("@label", "Link Destination"), "LinkDestination", DolphinView::SortByDestination},
        { "path",        I18N_NOOP2_NOSTRIP("@label", "Path"),             "Path",            DolphinView::SortByPath}
    };

    m_map.insert(DolphinView::SizeInfo, &additionalInfo[0]);
    m_map.insert(DolphinView::DateInfo, &additionalInfo[1]);
    m_map.insert(DolphinView::PermissionsInfo, &additionalInfo[2]);
    m_map.insert(DolphinView::OwnerInfo, &additionalInfo[3]);
    m_map.insert(DolphinView::GroupInfo, &additionalInfo[4]);
    m_map.insert(DolphinView::TypeInfo, &additionalInfo[5]);
    m_map.insert(DolphinView::DestinationInfo, &additionalInfo[6]);
    m_map.insert(DolphinView::PathInfo, &additionalInfo[7]);

    QMapIterator<DolphinView::AdditionalInfo, const AdditionalInfo*> it(m_map);
    while (it.hasNext()) {
        it.next();
        m_infoForRole.insert(it.value()->role, it.key());
    }
}

AdditionalInfoAccessor::~AdditionalInfoAccessor()
{
}
