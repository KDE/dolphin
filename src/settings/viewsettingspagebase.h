/***************************************************************************
 *   Copyright (C) 2008 by Peter Penz <peter.penz@gmx.at>                  *
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

#ifndef VIEWSETTINGSPAGEBASE_H
#define VIEWSETTINGSPAGEBASE_H

#include <kvbox.h>

/**
 * @brief Base class for view settings configuration pages.
 *
 * @see GeneralViewSettingsPage;
 * @see IconViewSettingsPage
 * @see DetailsViewSettingsPage
 * @see ColumnViewSettingsPage
 */
class ViewSettingsPageBase : public KVBox
{
    Q_OBJECT

public:
    ViewSettingsPageBase(QWidget* parent);
    virtual ~ViewSettingsPageBase();

    /**
     * Applies the settings for the view.
     * The settings are persisted automatically when
     * closing Dolphin.
     */
    virtual void applySettings() = 0;

    /** Restores the settings to default values. */
    virtual void restoreDefaults() = 0;

signals:
    void changed();
};

#endif
