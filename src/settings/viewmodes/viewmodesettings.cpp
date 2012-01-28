/***************************************************************************
 *   Copyright (C) 2011 by Peter Penz <peter.penz19@gmail.com>             *
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

#include "viewmodesettings.h"

#include "dolphin_iconsmodesettings.h"
#include "dolphin_detailsmodesettings.h"
#include "dolphin_compactmodesettings.h"

#define VIEWMODESETTINGS_SET_VALUE(mode, setValue, value) \
    switch (mode) { \
    case ViewModeSettings::IconsMode:   IconsModeSettings::setValue(value); break; \
    case ViewModeSettings::CompactMode: CompactModeSettings::setValue(value); break; \
    case ViewModeSettings::DetailsMode: DetailsModeSettings::setValue(value); break; \
    default: Q_ASSERT(false); break; \
    }

#define VIEWMODESETTINGS_RETURN_VALUE(mode, getValue, type) \
    type value; \
    switch (m_mode) { \
    case IconsMode:   value = IconsModeSettings::getValue(); break; \
    case CompactMode: value = CompactModeSettings::getValue(); break; \
    case DetailsMode: value = DetailsModeSettings::getValue(); break; \
    default:          value = IconsModeSettings::getValue(); \
                      Q_ASSERT(false); \
                      break; \
    } \
    return value

ViewModeSettings::ViewModeSettings(ViewMode mode) :
    m_mode(mode)
{
}

ViewModeSettings::~ViewModeSettings()
{
}

void ViewModeSettings::setIconSize(int size) const
{
    VIEWMODESETTINGS_SET_VALUE(m_mode, setIconSize, size);
}

int ViewModeSettings::iconSize() const
{
    VIEWMODESETTINGS_RETURN_VALUE(m_mode, iconSize, int);
}

void ViewModeSettings::setPreviewSize(int size) const
{
    VIEWMODESETTINGS_SET_VALUE(m_mode, setPreviewSize, size);
}

int ViewModeSettings::previewSize() const
{
    VIEWMODESETTINGS_RETURN_VALUE(m_mode, previewSize, int);
}

void ViewModeSettings::setUseSystemFont(bool flag)
{
    VIEWMODESETTINGS_SET_VALUE(m_mode, setUseSystemFont, flag);
}

bool ViewModeSettings::useSystemFont() const
{
    VIEWMODESETTINGS_RETURN_VALUE(m_mode, useSystemFont, bool);
}

void ViewModeSettings::setFontFamily(const QString& fontFamily)
{
    VIEWMODESETTINGS_SET_VALUE(m_mode, setFontFamily, fontFamily);
}

QString ViewModeSettings::fontFamily() const
{
    VIEWMODESETTINGS_RETURN_VALUE(m_mode, fontFamily, QString);
}

void ViewModeSettings::setFontSize(qreal fontSize)
{
    VIEWMODESETTINGS_SET_VALUE(m_mode, setFontSize, fontSize);
}

qreal ViewModeSettings::fontSize() const
{
    VIEWMODESETTINGS_RETURN_VALUE(m_mode, fontSize, qreal);
}

void ViewModeSettings::setItalicFont(bool italic)
{
    VIEWMODESETTINGS_SET_VALUE(m_mode, setItalicFont, italic);
}

bool ViewModeSettings::italicFont() const
{
    VIEWMODESETTINGS_RETURN_VALUE(m_mode, italicFont, bool);
}

void ViewModeSettings::setFontWeight(int fontWeight)
{
    VIEWMODESETTINGS_SET_VALUE(m_mode, setFontWeight, fontWeight);
}

int ViewModeSettings::fontWeight() const
{
    VIEWMODESETTINGS_RETURN_VALUE(m_mode, fontWeight, int);
}

void ViewModeSettings::readConfig()
{
    switch (m_mode) {
    case ViewModeSettings::IconsMode:   IconsModeSettings::self()->readConfig(); break;
    case ViewModeSettings::CompactMode: CompactModeSettings::self()->readConfig(); break;
    case ViewModeSettings::DetailsMode: DetailsModeSettings::self()->readConfig(); break;
    default: Q_ASSERT(false); break;
    }
}

void ViewModeSettings::writeConfig()
{
    switch (m_mode) {
    case ViewModeSettings::IconsMode:   IconsModeSettings::self()->writeConfig(); break;
    case ViewModeSettings::CompactMode: CompactModeSettings::self()->writeConfig(); break;
    case ViewModeSettings::DetailsMode: DetailsModeSettings::self()->writeConfig(); break;
    default: Q_ASSERT(false); break;
    }
}
