/*
 * SPDX-FileCopyrightText: 2024 Kristen McWilliam <kmcwilliampublic@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef SEARCHSETTINGSPAGE_H
#define SEARCHSETTINGSPAGE_H

#include "settings/settingspagebase.h"

#include <QCheckBox>

/**
 * @brief Tab page for the 'Search' settings of the Dolphin settings dialog.
 */
class SearchSettingsPage : public SettingsPageBase
{
    Q_OBJECT

public:
    SearchSettingsPage(QWidget *parent);
    ~SearchSettingsPage() override;

    /** @see SettingsPageBase::applySettings() */
    void applySettings() override;

    /** @see SettingsPageBase::restoreDefaults() */
    void restoreDefaults() override;

private:
    void loadSettings();

private:
    /**
     * Checkbox to enable or disable the use of the indexing service when
     * searching in Dolphin.
     */
    QCheckBox *m_useIndexing;
};

#endif
