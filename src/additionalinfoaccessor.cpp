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
#include <kglobal.h>
#include <klocale.h>

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
    return m_informations;
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

QString AdditionalInfoAccessor::actionCollectionName(KFileItemDelegate::Information info) const
{
    return QLatin1String(m_map[info]->actionCollectionName);
}

QString AdditionalInfoAccessor::translation(KFileItemDelegate::Information info) const
{
    return i18n(m_map[info]->translation);
}

int AdditionalInfoAccessor::bitValue(KFileItemDelegate::Information info) const
{
    return m_map[info]->bitValue;
}

AdditionalInfoAccessor::AdditionalInfoAccessor() :
    m_informations(),
    m_map()
{
    static const AdditionalInfoAccessor::AdditionalInfo additionalInfos[] = {
        { "size",        I18N_NOOP2("@label", "Size"),          1 },
        { "date",        I18N_NOOP2("@label", "Date"),          2 },
        { "permissions", I18N_NOOP2("@label", "Permissions"),   4 },
        { "owner",       I18N_NOOP2("@label", "Owner"),         8 },
        { "group",       I18N_NOOP2("@label", "Group"),        16 },
        { "type",        I18N_NOOP2("@label", "Type"),         32 },
        { "destination", I18N_NOOP2("@label", "Destination"),  64 },
        { "path",        I18N_NOOP2("@label", "Path"),        128 }
    };

    m_map.insert(KFileItemDelegate::Size, &additionalInfos[0]);
    m_map.insert(KFileItemDelegate::ModificationTime, &additionalInfos[1]);
    m_map.insert(KFileItemDelegate::Permissions, &additionalInfos[2]);
    m_map.insert(KFileItemDelegate::Owner, &additionalInfos[3]);
    m_map.insert(KFileItemDelegate::OwnerAndGroup, &additionalInfos[4]);
    m_map.insert(KFileItemDelegate::FriendlyMimeType, &additionalInfos[5]);
    m_map.insert(KFileItemDelegate::LinkDest, &additionalInfos[6]);
    m_map.insert(KFileItemDelegate::LocalPathOrUrl, &additionalInfos[7]);

    m_informations = m_map.keys();
}

AdditionalInfoAccessor::~AdditionalInfoAccessor()
{
}

