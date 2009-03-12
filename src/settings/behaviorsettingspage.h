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
#ifndef BEHAVIORSETTINGSPAGE_H
#define BEHAVIORSETTINGSPAGE_H

#include <settings/settingspagebase.h>
#include <kurl.h>

class DolphinMainWindow;
class QCheckBox;
class QRadioButton;

/**
 * @brief Tab page for the 'Behavior' settings of the Dolphin settings dialog.
 */
class BehaviorSettingsPage : public SettingsPageBase
{
    Q_OBJECT

public:
    BehaviorSettingsPage(const KUrl& url, QWidget* parent);
    virtual ~BehaviorSettingsPage();

    /** @see SettingsPageBase::applySettings() */
    virtual void applySettings();

    /** @see SettingsPageBase::restoreDefaults() */
    virtual void restoreDefaults();

private:
    void loadSettings();

private:
    KUrl m_url;

    QRadioButton* m_localProps;
    QRadioButton* m_globalProps;

    QCheckBox* m_confirmMoveToTrash;
    QCheckBox* m_confirmDelete;
    QCheckBox* m_confirmClosingMultipleTabs;

    QCheckBox* m_renameInline;
    QCheckBox* m_showToolTips;
    QCheckBox* m_showSelectionToggle;
};

#endif
