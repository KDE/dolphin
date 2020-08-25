/*
 * SPDX-FileCopyrightText: 2009 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef NAVIGATIONSETTINGSPAGE_H
#define NAVIGATIONSETTINGSPAGE_H

#include "settings/settingspagebase.h"

class QCheckBox;

/**
 * @brief Page for the 'Navigation' settings of the Dolphin settings dialog.
 */
class NavigationSettingsPage : public SettingsPageBase
{
    Q_OBJECT

public:
    explicit NavigationSettingsPage(QWidget* parent);
    ~NavigationSettingsPage() override;

    /** @see SettingsPageBase::applySettings() */
    void applySettings() override;

    /** @see SettingsPageBase::restoreDefaults() */
    void restoreDefaults() override;

private:
    void loadSettings();

private:
    QCheckBox* m_openArchivesAsFolder;
    QCheckBox* m_autoExpandFolders;
};

#endif
