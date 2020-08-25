/*
 * SPDX-FileCopyrightText: 2009 Shaun Reich <shaun.reich@kdemail.net>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
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
    explicit TrashSettingsPage(QWidget* parent);
    ~TrashSettingsPage() override;

    /** @see SettingsPageBase::applySettings() */
    void applySettings() override;

    /** @see SettingsPageBase::restoreDefaults() */
    void restoreDefaults() override;

private:
    void loadSettings();
    KCModuleProxy *m_proxy;
};

#endif
