/***************************************************************************
 *   Copyright (C) 2009 by Shaun Reich shaun.reich@kdemail.net             *
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
#ifndef TRASHSETTINGSPAGE_H
#define TRASHSETTINGSPAGE_H

#include "settings/settingspagebase.h"
class KCModuleProxy;

/**
 * @brief Tab page for the 'Trash' settings of the Dolphin settings dialog, it uses the KCM.
 */
class TrashSettingsPage : public SettingsPageBase
{
    Q_OBJECT

public:
    TrashSettingsPage(QWidget* parent);
    virtual ~TrashSettingsPage();

    /** @see SettingsPageBase::applySettings() */
    virtual void applySettings();

    /** @see SettingsPageBase::restoreDefaults() */
    virtual void restoreDefaults();

private:
    void loadSettings();
    KCModuleProxy *m_proxy;
};

#endif
