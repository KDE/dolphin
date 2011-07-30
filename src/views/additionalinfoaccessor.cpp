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
    return m_infoList;
}

QByteArray AdditionalInfoAccessor::role(DolphinView::AdditionalInfo info) const
{
    QByteArray role;
    switch (info) {
    case DolphinView::NameInfo:        role = "name"; break;
    case DolphinView::SizeInfo:        role = "size"; break;
    case DolphinView::DateInfo:        role = "date"; break;
    case DolphinView::PermissionsInfo: role = "permissions"; break;
    case DolphinView::OwnerInfo:       role = "owner"; break;
    case DolphinView::GroupInfo:       role = "group"; break;
    case DolphinView::TypeInfo:        role = "type"; break;
    case DolphinView::DestinationInfo: role = "destination"; break;
    case DolphinView::PathInfo:        role = "path"; break;
    default: break;
    }

    return role;
}

QString AdditionalInfoAccessor::actionCollectionName(DolphinView::AdditionalInfo info,
                                                     ActionCollectionType type) const
{
    QString name;
    switch (type) {
    case SortByType:
        name = QLatin1String("sort_by_") + QLatin1String(m_map[info]->actionCollectionName);
        break;

    case AdditionalInfoType:
        name = QLatin1String("show_") + QLatin1String(m_map[info]->actionCollectionName);
        break;
    }

    return name;
}

QString AdditionalInfoAccessor::translation(DolphinView::AdditionalInfo info) const
{
    return i18nc(m_map[info]->context, m_map[info]->translation);
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
    m_infoList(),
    m_map()
{
    static const AdditionalInfoAccessor::AdditionalInfo additionalInfo[] = {
        // Entries for view-properties version 1:
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

    // The m_infoList defines all available keys and the sort order
    // (don't use m_information = m_map.keys(), as the order would be undefined).
    m_infoList.append(DolphinView::SizeInfo);
    m_infoList.append(DolphinView::DateInfo);
    m_infoList.append(DolphinView::PermissionsInfo);
    m_infoList.append(DolphinView::OwnerInfo);
    m_infoList.append(DolphinView::GroupInfo);
    m_infoList.append(DolphinView::TypeInfo);
    m_infoList.append(DolphinView::DestinationInfo);
    m_infoList.append(DolphinView::PathInfo);
}

AdditionalInfoAccessor::~AdditionalInfoAccessor()
{
}
