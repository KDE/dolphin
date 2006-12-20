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
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA            *
 ***************************************************************************/

#ifndef GENERALVIEWSETTINGSPAGE_H
#define GENERALVIEWSETTINGSPAGE_H

#include <kvbox.h>

class QRadioButton;

/**
 * @brief Represents the page from the Dolphin Settings which allows
 * to modify general settings for the view modes.
 *
 *  @author Peter Penz <peter.penz@gmx.at>
 */
class GeneralViewSettingsPage : public KVBox
{
    Q_OBJECT

public:
    GeneralViewSettingsPage(QWidget* parent);
    virtual ~GeneralViewSettingsPage();

    /**
     * Applies the general settings for the view modes
     * The settings are persisted automatically when
     * closing Dolphin.
     */
    void applySettings();

private:
    QRadioButton* m_localProps;
    QRadioButton* m_globalProps;
};

#endif
