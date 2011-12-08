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

#ifndef VIEWMODESETTINGS_H
#define VIEWMODESETTINGS_H

#include <QString>

/**
 * @short Helper class for accessing similar properties of IconsModeSettings,
 *        CompactModeSettings and DetailsModeSettings.
 */
class ViewModeSettings
{
public:
    enum ViewMode
    {
        IconsMode,
        CompactMode,
        DetailsMode
    };

    ViewModeSettings(ViewMode mode);
    virtual ~ViewModeSettings();

    void setIconSize(int size) const;
    int iconSize() const;

    void setPreviewSize(int size) const;
    int previewSize() const;

    void setUseSystemFont(bool flag);
    bool useSystemFont() const;

    void setFontFamily(const QString& fontFamily);
    QString fontFamily() const;

    void setFontSize(qreal fontSize);
    qreal fontSize() const;

    void setItalicFont(bool italic);
    bool italicFont() const;

    void setFontWeight(int fontWeight);
    int fontWeight() const;

    void readConfig();
    void writeConfig();

private:
    ViewMode m_mode;
};

#endif
