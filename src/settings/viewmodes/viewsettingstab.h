/***************************************************************************
 *   Copyright (C) 2008-2011 by Peter Penz <peter.penz19@gmail.com>        *
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

#ifndef VIEWSETTINGSTAB_H
#define VIEWSETTINGSTAB_H

#include <QWidget>
#include <settings/viewmodes/viewmodesettings.h>

class DolphinFontRequester;
class KComboBox;
class QCheckBox;
class QSlider;

/**
 * @brief Represents one tab of the view-settings page.
 */
class ViewSettingsTab : public QWidget
{
    Q_OBJECT

public:
    enum Mode
    {
        IconsMode,
        CompactMode,
        DetailsMode
    };

    explicit ViewSettingsTab(Mode mode, QWidget* parent = 0);
    virtual ~ViewSettingsTab();

    void applySettings();
    void restoreDefaultSettings();

signals:
    void changed();

private:
    void loadSettings();

    ViewModeSettings::ViewMode viewMode() const;

private:
    Mode m_mode;
    QSlider* m_defaultSizeSlider;
    QSlider* m_previewSizeSlider;

    DolphinFontRequester* m_fontRequester;
    KComboBox* m_textWidthBox;
    QCheckBox* m_expandableFolders;
};

#endif
