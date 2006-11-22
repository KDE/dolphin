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
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef DETAILSVIEWSETTINGSPAGE_H
#define DETAILSVIEWSETTINGSPAGE_H

#include <q3vbox.h>
class QCheckBox;
class QFontComboBox;
class QSpinBox;
class QComboBox;
class QRadioButton;

/**
 * @brief Represents the page from the Dolphin Settings which allows
 * to modify the settings for the details view.
 *
 *  @author Peter Penz <peter.penz@gmx.at>
 */
class DetailsViewSettingsPage : public Q3VBox
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
    void applySettings();

private:
    QCheckBox* m_dateBox;
    QCheckBox* m_permissionsBox;
    QCheckBox* m_ownerBox;
    QCheckBox* m_groupBox;
    QRadioButton* m_smallIconSize;
    QRadioButton* m_mediumIconSize;
    QRadioButton* m_largeIconSize;

    QFontComboBox* m_fontFamilyBox;
    QSpinBox* m_fontSizeBox;
};

#endif
