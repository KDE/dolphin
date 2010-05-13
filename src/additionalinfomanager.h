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

#ifndef ADDITIONALINFOMANAGER_H
#define ADDITIONALINFOMANAGER_H

#include <libdolphin_export.h>
#include <kfileitemdelegate.h>

#include <QList>
#include <QMap>

/**
 * @brief Manages the information that is available by KFileItemDelegate::Information.
 *
 * The information that is available by KFileItemDelegate::Information will be shown
 * in Dolphin the following way:
 * - As additional columns in the details view
 * - As additional lines in the icons view
 * - As menu entries in the "Sort By" and "Additional Information" groups
 * - As popup menu entries in the details view header popup
 * - As checkable entries in the View Properties dialog
 *
 * The AdditionalInfoManager provides a central place to get all available keys,
 * the corresponding translations, action names, etc., so that modifications or
 * extensions in KFileItemDelegate only require adjustments in the implementation
 * of this class.
 */
class LIBDOLPHINPRIVATE_EXPORT AdditionalInfoManager
{
public:
    static AdditionalInfoManager& instance();

    /**
     * @return List of all available information entries of KFileItemDelegate.
     *         All entries of this list are keys for accessing the corresponding
     *         data (see actionCollectionName(), translation(), bitValue()).
     */
    KFileItemDelegate::InformationList keys() const;

    /**
     * @return Key for the model column with the index \p columnIndex.
     */
    KFileItemDelegate::Information keyForColumn(int columnIndex) const;

    QString actionCollectionName(KFileItemDelegate::Information info) const;

    QString translation(KFileItemDelegate::Information info) const;

    /**
     * @return Bitvalue for \p info that is stored in a ViewProperties instance.
     */
    int bitValue(KFileItemDelegate::Information info) const;

protected:
    AdditionalInfoManager();
    virtual ~AdditionalInfoManager();
    friend class AdditionalInfoManagerSingleton;

private:
    struct AdditionalInfo {
        const char* const actionCollectionName;
        const char* const translation;
        const int bitValue;
    };

    KFileItemDelegate::InformationList m_informations;
    QMap<KFileItemDelegate::Information, const AdditionalInfo*> m_map;
};

#endif

