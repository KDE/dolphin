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

#ifndef SETTINGSPAGEBASE_H
#define SETTINGSPAGEBASE_H

#include <qwidget.h>

/**
 * @brief Base class for the settings pages of the Dolphin settings dialog.
 *
 * @author Peter Penz <peter.penz@gmx.at>
 */
class SettingsPageBase : public QWidget
{
    Q_OBJECT

public:
    SettingsPageBase(QWidget* parent);
    virtual ~SettingsPageBase();

    /**
     * Must be implemented by a derived class to
     * persistently store the settings.
     */
    virtual void applySettings() = 0;
};

#endif
