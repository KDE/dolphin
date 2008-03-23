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
#ifndef VIEWSETTINGSPAGE_H
#define VIEWSETTINGSPAGE_H

#include <QtGui/QWidget>
#include <settingspagebase.h>

class ViewSettingsPageBase;
class DolphinMainWindow;

/**
 * @brief Page for the 'View' settings of the Dolphin settings dialog.
 *
 * The views settings allow to set the properties for the icons mode,
 * the details mode and the column mode.
 */
class ViewSettingsPage : public SettingsPageBase
{
    Q_OBJECT

public:
    ViewSettingsPage(DolphinMainWindow* mainWindow, QWidget* parent);
    virtual ~ViewSettingsPage();

    /** @see SettingsPageBase::applySettings() */
    virtual void applySettings();

    /** @see SettingsPageBase::restoreDefaults() */
    virtual void restoreDefaults();

private:
    QList<ViewSettingsPageBase*> m_pages;
};

#endif
