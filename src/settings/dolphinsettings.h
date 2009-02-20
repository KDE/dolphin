/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz                                      *
 *   peter.penz@gmx.at                                                     *
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

#ifndef DOLPHINSETTINGS_H
#define DOLPHINSETTINGS_H

#include <libdolphin_export.h>

class ColumnModeSettings;
class DetailsModeSettings;
class GeneralSettings;
class IconsModeSettings;
class KFilePlacesModel;

/**
 * @brief Manages and stores all settings from Dolphin.
 *
 * The following properties are stored:
 * - home URL
 * - default view mode
 * - URL navigator state (editable or not)
 * - Text completion state
 * - split view
 * - properties for views
 */
class LIBDOLPHINPRIVATE_EXPORT DolphinSettings
{
public:
    static DolphinSettings& instance();

    GeneralSettings* generalSettings() const;
    IconsModeSettings* iconsModeSettings() const;
    DetailsModeSettings* detailsModeSettings() const;
    ColumnModeSettings* columnModeSettings() const;
    KFilePlacesModel* placesModel() const;
    virtual void save();

protected:
    DolphinSettings();
    virtual ~DolphinSettings();
    friend class DolphinSettingsSingleton;

private:
    GeneralSettings* m_generalSettings;
    IconsModeSettings* m_iconsModeSettings;
    DetailsModeSettings* m_detailsModeSettings;
    ColumnModeSettings* m_columnModeSettings;
    KFilePlacesModel* m_placesModel;
};

inline GeneralSettings* DolphinSettings::generalSettings() const
{
    return m_generalSettings;
}

inline IconsModeSettings* DolphinSettings::iconsModeSettings() const
{
    return m_iconsModeSettings;
}

inline DetailsModeSettings* DolphinSettings::detailsModeSettings() const
{
    return m_detailsModeSettings;
}

inline ColumnModeSettings* DolphinSettings::columnModeSettings() const
{
    return m_columnModeSettings;
}

inline KFilePlacesModel* DolphinSettings::placesModel() const
{
    return m_placesModel;
}

#endif
