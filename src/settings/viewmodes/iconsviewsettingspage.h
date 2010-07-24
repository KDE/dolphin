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
#include "viewsettingspagebase.h"

class DolphinFontRequester;
class IconSizeGroupBox;
class KComboBox;
class KIntSpinBox;

/**
 * @brief Tab page for the 'Icons Mode' settings
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
class IconsViewSettingsPage : public ViewSettingsPageBase
{
    Q_OBJECT

public:
    IconsViewSettingsPage(QWidget* parent);
    virtual ~IconsViewSettingsPage();

    /**
     * Applies the settings for the icons view.
     * The settings are persisted automatically when
     * closing Dolphin.
     */
    virtual void applySettings();

    /** Restores the settings to default values. */
    virtual void restoreDefaults();

private:
    void loadSettings();

private:
    enum
    {
        GridSpacingBase =   8,
        GridSpacingInc  =  12,
        LeftToRightBase = 128,
        LeftToRightInc  =  64,
        TopToBottomBase =  32,
        TopToBottomInc  =  32
    };

    IconSizeGroupBox* m_iconSizeGroupBox;
    KComboBox* m_textWidthBox;
    DolphinFontRequester* m_fontRequester;
    KIntSpinBox* m_textlinesCountBox;

    KComboBox* m_arrangementBox;
    KComboBox* m_gridSpacingBox;
};

#endif
