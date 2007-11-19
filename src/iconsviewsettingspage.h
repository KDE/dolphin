/***************************************************************************
 *   Copyright (C) 2006 by Peter Penz <peter.penz@gmx.at>                  *
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

#ifndef ICONSVIEWSETTINGSPAGE_H
#define ICONSVIEWSETTINGSPAGE_H

#include <dolphiniconsview.h>
#include <kvbox.h>

class DolphinMainWindow;
class KFontRequester;
class QComboBox;
class QPushButton;
class QSpinBox;

/**
 * @brief Tab page for the 'Icons Mode' and 'Previews Mode' settings
 * of the Dolphin settings dialog.
 *
 * Allows to set:
 * - icon size
 * - preview size
 * - text width
 * - grid spacing
 * - font
 * - number of text lines
 * - arrangement
 *
 * @see DolphinIconsViewSettings
 */
class IconsViewSettingsPage : public KVBox
{
    Q_OBJECT

public:
    IconsViewSettingsPage(DolphinMainWindow* mainWindow, QWidget* parent);
    virtual ~IconsViewSettingsPage();

    /**
     * Applies the settings for the icons view.
     * The settings are persisted automatically when
     * closing Dolphin.
     */
    void applySettings();

    /** Restores the settings to default values. */
    void restoreDefaults();

private slots:
    void openIconSizeDialog();

private:
    void loadSettings();

private:
    enum
    {
        GridSpacingBase =   8,
        GridSpacingInc  =  12,
        LeftToRightBase = 128,
        LeftToRightInc  =  64,
        TopToBottomBase =  64,
        TopToBottomInc  =  32
    };

    DolphinMainWindow* m_mainWindow;
    int m_iconSize;
    int m_previewSize;

    QPushButton* m_iconSizeButton;
    QComboBox* m_textWidthBox;
    KFontRequester* m_fontRequester;
    QSpinBox* m_textlinesCountBox;

    QComboBox* m_arrangementBox;
    QComboBox* m_gridSpacingBox;
};

#endif
