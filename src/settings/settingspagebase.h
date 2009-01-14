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
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA          *
 ***************************************************************************/

#ifndef SETTINGSPAGEBASE_H
#define SETTINGSPAGEBASE_H

#include <QtGui/QWidget>

/**
 * @brief Base class for the settings pages of the Dolphin settings dialog.
 *
 * @author Peter Penz <peter.penz@gmx.at>
 */
class SettingsPageBase : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsPageBase(QWidget* parent);
    virtual ~SettingsPageBase();

    /**
     * Must be implemented by a derived class to
     * persistently store the settings.
     */
    virtual void applySettings() = 0;

    /**
     * Must be implemented by a derived class to
     * restored the settings to default values.
     */
    virtual void restoreDefaults() = 0;

signals:
    /** Is emitted if a setting has been changed. */
    void changed();
};

#endif
