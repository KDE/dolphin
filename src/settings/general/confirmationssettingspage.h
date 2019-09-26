/***************************************************************************
 *   Copyright (C) 2012 by Peter Penz <peter.penz19@gmail.com>             *
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
#ifndef CONFIRMATIONSSETTINGSPAGE_H
#define CONFIRMATIONSSETTINGSPAGE_H

#include "config-terminal.h"
#include "settings/settingspagebase.h"

class QCheckBox;
class QComboBox;

/**
 * @brief Page for the enabling or disabling confirmation dialogs.
 */
class ConfirmationsSettingsPage : public SettingsPageBase
{
    Q_OBJECT

public:
    explicit ConfirmationsSettingsPage(QWidget* parent);
    ~ConfirmationsSettingsPage() override;

    /** @see SettingsPageBase::applySettings() */
    void applySettings() override;

    /** @see SettingsPageBase::restoreDefaults() */
    void restoreDefaults() override;

private:
    void loadSettings();

private:
    QCheckBox* m_confirmMoveToTrash;
    QCheckBox* m_confirmEmptyTrash;
    QCheckBox* m_confirmDelete;

#ifdef HAVE_TERMINAL
    QCheckBox* m_confirmClosingTerminalRunningProgram;
#endif

    QCheckBox* m_confirmClosingMultipleTabs;
    QComboBox* m_confirmScriptExecution;
};

#endif
