/*
 * SPDX-FileCopyrightText: 2009 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef STATUSBARSETTINGSPAGE_H
#define STATUSBARSETTINGSPAGE_H

#include "settings/settingspagebase.h"

class QCheckBox;

/**
 * @brief Tab page for the 'Status Bar' settings of the Dolphin settings dialog.
 */
class StatusBarSettingsPage : public SettingsPageBase
{
    Q_OBJECT

public:
    explicit StatusBarSettingsPage(QWidget *parent);
    ~StatusBarSettingsPage() override;

    /** @see SettingsPageBase::applySettings() */
    void applySettings() override;

    /** @see SettingsPageBase::restoreDefaults() */
    void restoreDefaults() override;

private:
    void loadSettings();
    void onShowStatusBarToggled();

private:
    QCheckBox *m_showStatusBar;
    QCheckBox *m_showZoomSlider;
    QCheckBox *m_showSpaceInfo;
};

#endif
