/*
 * SPDX-FileCopyrightText: 2012 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef CONFIRMATIONSSETTINGSPAGE_H
#define CONFIRMATIONSSETTINGSPAGE_H

#include "config-dolphin.h"
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
    explicit ConfirmationsSettingsPage(QWidget *parent);
    ~ConfirmationsSettingsPage() override;

    /** @see SettingsPageBase::applySettings() */
    void applySettings() override;

    /** @see SettingsPageBase::restoreDefaults() */
    void restoreDefaults() override;

private:
    void loadSettings();

private:
    QCheckBox *m_confirmMoveToTrash;
    QCheckBox *m_confirmEmptyTrash;
    QCheckBox *m_confirmDelete;

#if HAVE_TERMINAL
    QCheckBox *m_confirmClosingTerminalRunningProgram;
#endif

    QCheckBox *m_confirmClosingMultipleTabs;
    QComboBox *m_confirmScriptExecution;
    QCheckBox *m_confirmOpenManyFolders;
    QCheckBox *m_confirmOpenManyTerminals;
    QCheckBox *m_confirmRisksOfActingAsAdmin;
};

#endif
