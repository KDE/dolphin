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

#ifndef COLUMNVIEWSETTINGSPAGE_H
#define COLUMNVIEWSETTINGSPAGE_H

#include <settings/viewsettingspagebase.h>

class DolphinMainWindow;
class DolphinFontRequester;
class IconSizeGroupBox;
class KComboBox;

/**
 * @brief Represents the page from the Dolphin Settings which allows
 *        to modify the settings for the details view.
 */
class ColumnViewSettingsPage : public ViewSettingsPageBase
{
    Q_OBJECT

public:
    ColumnViewSettingsPage(QWidget* parent);
    virtual ~ColumnViewSettingsPage();

    /**
     * Applies the settings for the details view.
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
        BaseTextWidth = 200,
        TextInc = 50
    };

    IconSizeGroupBox* m_iconSizeGroupBox;
    DolphinFontRequester* m_fontRequester;
    KComboBox* m_textWidthBox;
};

#endif
