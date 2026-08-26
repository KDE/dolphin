/*
 * SPDX-FileCopyrightText: 2026 KsmBL <katzen.sind.lecker69@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef POWERCOPYSETTINGSPAGE_H
#define POWERCOPYSETTINGSPAGE_H

#include "settings/settingspagebase.h"

class QCheckBox;
class QSpinBox;

/**
 * @brief How many files Dolphin copies at the same time.
 */
class PowerCopySettingsPage : public SettingsPageBase
{
    Q_OBJECT

public:
    explicit PowerCopySettingsPage(QWidget *parent);

    /** @see SettingsPageBase::applySettings() */
    void applySettings() override;

    /** @see SettingsPageBase::restoreDefaults() */
    void restoreDefaults() override;

private:
    void loadSettings();
    void updateEnabledState();

    QCheckBox *m_enabled;
    QSpinBox *m_filesInFlight;
    QSpinBox *m_minimumFiles;
};

#endif
