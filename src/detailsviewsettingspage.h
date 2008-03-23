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

#ifndef DETAILSVIEWSETTINGSPAGE_H
#define DETAILSVIEWSETTINGSPAGE_H

#include <viewsettingspagebase.h>

class DolphinFontRequester;
class QCheckBox;
class QRadioButton;

/**
 * @brief Represents the page from the Dolphin Settings which allows
 *        to modify the settings for the details view.
 */
class DetailsViewSettingsPage : public ViewSettingsPageBase
{
    Q_OBJECT

public:
    DetailsViewSettingsPage(QWidget* parent);
    virtual ~DetailsViewSettingsPage();

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
    QRadioButton* m_smallIconSize;
    QRadioButton* m_mediumIconSize;
    QRadioButton* m_largeIconSize;
    DolphinFontRequester* m_fontRequester;
    QCheckBox* m_expandableFolders;
};

#endif
