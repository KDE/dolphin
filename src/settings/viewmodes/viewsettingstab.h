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

#include "settings/viewmodes/viewmodesettings.h"

#include <QWidget>

class DolphinFontRequester;
class QComboBox;
class QCheckBox;
class QSlider;
class QSpinBox;
class QRadioButton;

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

    explicit ViewSettingsTab(Mode mode, QWidget* parent = nullptr);
    ~ViewSettingsTab() override;

    void applySettings();
    void restoreDefaultSettings();

signals:
    void changed();

private slots:

    void slotDefaultSliderMoved(int value);
    void slotPreviewSliderMoved(int value);
private:
    void loadSettings();
    void showToolTip(QSlider* slider, int value);

    ViewModeSettings::ViewMode viewMode() const;

private:
    Mode m_mode;
    QSlider* m_defaultSizeSlider;
    QSlider* m_previewSizeSlider;

    DolphinFontRequester* m_fontRequester;
    QComboBox* m_widthBox;
    QComboBox* m_maxLinesBox;
    QCheckBox* m_expandableFolders;
    QRadioButton* m_numberOfItems;
    QRadioButton* m_sizeOfContents;
    QSpinBox* m_recursiveDirectorySizeLimit;
};

#endif
