/*
 * SPDX-FileCopyrightText: 2006 Peter Penz <peter.penz@gmx.at>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef GENERALSETTINGSPAGE_H
#define GENERALSETTINGSPAGE_H

#include "settings/settingspagebase.h"

#include <QWidget>

class QUrl;
class SettingsPageBase;

/**
 * @brief Page for the 'General' settings of the Dolphin settings dialog.
 *
 * The general settings include:
 * - Behavior
 * - Previews
 * - Context Menu
 * - Status Bar
 */
class GeneralSettingsPage : public SettingsPageBase
{
    Q_OBJECT

public:
    GeneralSettingsPage(const QUrl &url, QWidget *parent);
    ~GeneralSettingsPage() override;

    /** @see SettingsPageBase::applySettings() */
    void applySettings() override;

    /** @see SettingsPageBase::restoreDefaults() */
    void restoreDefaults() override;

private:
    QList<SettingsPageBase *> m_pages;
};

#endif
