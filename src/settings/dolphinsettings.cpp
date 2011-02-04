/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz (peter.penz@gmx.at),                 *
 *   Cvetoslav Ludmiloff and Patrice Tremblay                              *
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

#include "dolphinsettings.h"

#include <KFilePlacesModel>
#include <KComponentData>
#include <KLocale>
#include <KStandardDirs>

#include "dolphin_columnmodesettings.h"
#include "dolphin_detailsmodesettings.h"
#include "dolphin_generalsettings.h"
#include "dolphin_iconsmodesettings.h"

class DolphinSettingsSingleton
{
public:
    DolphinSettings instance;
};
K_GLOBAL_STATIC(DolphinSettingsSingleton, s_settings)

DolphinSettings& DolphinSettings::instance()
{
    return s_settings->instance;
}

void DolphinSettings::save()
{
    m_generalSettings->writeConfig();
    m_iconsModeSettings->writeConfig();
    m_detailsModeSettings->writeConfig();
    m_columnModeSettings->writeConfig();
}

DolphinSettings::DolphinSettings()
{
    m_generalSettings = new GeneralSettings();
    m_iconsModeSettings = new IconsModeSettings();
    m_detailsModeSettings = new DetailsModeSettings();
    m_columnModeSettings = new ColumnModeSettings();
    m_placesModel = new KFilePlacesModel();
}

DolphinSettings::~DolphinSettings()
{
    delete m_generalSettings;
    m_generalSettings = 0;

    delete m_iconsModeSettings;
    m_iconsModeSettings = 0;

    delete m_detailsModeSettings;
    m_detailsModeSettings = 0;

    delete m_columnModeSettings;
    m_columnModeSettings = 0;

    delete m_placesModel;
    m_placesModel = 0;
}
