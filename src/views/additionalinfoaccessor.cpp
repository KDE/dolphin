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

#include "dolphinmodel.h"
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

KFileItemDelegate::InformationList AdditionalInfoAccessor::keys() const
{
    return m_information;
}

KFileItemDelegate::Information AdditionalInfoAccessor::keyForColumn(int columnIndex) const
{
    KFileItemDelegate::Information info = KFileItemDelegate::NoInformation;

    switch (columnIndex) {
    case DolphinModel::Size:           info = KFileItemDelegate::Size; break;
    case DolphinModel::ModifiedTime:   info = KFileItemDelegate::ModificationTime; break;
    case DolphinModel::Permissions:    info = KFileItemDelegate::Permissions; break;
    case DolphinModel::Owner:          info = KFileItemDelegate::Owner; break;
    case DolphinModel::Group:          info = KFileItemDelegate::OwnerAndGroup; break;
    case DolphinModel::Type:           info = KFileItemDelegate::FriendlyMimeType; break;
    case DolphinModel::LinkDest:       info = KFileItemDelegate::LinkDest; break;
    case DolphinModel::LocalPathOrUrl: info = KFileItemDelegate::LocalPathOrUrl; break;
    default: break;
    }

    return info;
}

QString AdditionalInfoAccessor::actionCollectionName(KFileItemDelegate::Information info,
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

QString AdditionalInfoAccessor::translation(KFileItemDelegate::Information info) const
{
    return i18nc(m_map[info]->context, m_map[info]->translation);
}

QString AdditionalInfoAccessor::value(KFileItemDelegate::Information info) const
{
    return m_map[info]->value;
}

DolphinView::Sorting AdditionalInfoAccessor::sorting(KFileItemDelegate::Information info) const
{
    return m_map[info]->sorting;
}

int AdditionalInfoAccessor::bitValue(KFileItemDelegate::Information info) const
{
    return m_map[info]->bitValue;
}

AdditionalInfoAccessor::AdditionalInfoAccessor() :
    m_information(),
    m_map()
{
    static const AdditionalInfoAccessor::AdditionalInfo additionalInfo[] = {
        // Entries for view-properties version 1:
        { "size",        I18N_NOOP2_NOSTRIP("@label", "Size"),             "Size",            DolphinView::SortBySize,          1 },
        { "date",        I18N_NOOP2_NOSTRIP("@label", "Date"),             "Date",            DolphinView::SortByDate,          2 },
        { "permissions", I18N_NOOP2_NOSTRIP("@label", "Permissions"),      "Permissions",     DolphinView::SortByPermissions,   4 },
        { "owner",       I18N_NOOP2_NOSTRIP("@label", "Owner"),            "Owner",           DolphinView::SortByOwner,         8 },
        { "group",       I18N_NOOP2_NOSTRIP("@label", "Group"),            "Group",           DolphinView::SortByGroup,        16 },
        { "type",        I18N_NOOP2_NOSTRIP("@label", "Type"),             "Type",            DolphinView::SortByType,         32 },
        { "destination", I18N_NOOP2_NOSTRIP("@label", "Link Destination"), "LinkDestination", DolphinView::SortByDestination,  64 },
        { "path",        I18N_NOOP2_NOSTRIP("@label", "Path"),             "Path",            DolphinView::SortByPath,        128 }
        // Entries for view-properties version >= 2 (the last column can be set to 0):
    };

    m_map.insert(KFileItemDelegate::Size, &additionalInfo[0]);
    m_map.insert(KFileItemDelegate::ModificationTime, &additionalInfo[1]);
    m_map.insert(KFileItemDelegate::Permissions, &additionalInfo[2]);
    m_map.insert(KFileItemDelegate::Owner, &additionalInfo[3]);
    m_map.insert(KFileItemDelegate::OwnerAndGroup, &additionalInfo[4]);
    m_map.insert(KFileItemDelegate::FriendlyMimeType, &additionalInfo[5]);
    m_map.insert(KFileItemDelegate::LinkDest, &additionalInfo[6]);
    m_map.insert(KFileItemDelegate::LocalPathOrUrl, &additionalInfo[7]);

    // The m_information list defines all available keys and the sort order
    // (don't use m_information = m_map.keys(), as the order is undefined).
    m_information.append(KFileItemDelegate::Size);
    m_information.append(KFileItemDelegate::ModificationTime);
    m_information.append(KFileItemDelegate::Permissions);
    m_information.append(KFileItemDelegate::Owner);
    m_information.append(KFileItemDelegate::OwnerAndGroup);
    m_information.append(KFileItemDelegate::FriendlyMimeType);
    m_information.append(KFileItemDelegate::LinkDest);
    m_information.append(KFileItemDelegate::LocalPathOrUrl);
}

AdditionalInfoAccessor::~AdditionalInfoAccessor()
{
}
