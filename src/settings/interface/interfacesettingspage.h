/*
 * SPDX-FileCopyrightText: 2006 Peter Penz <peter.penz@gmx.at>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef INTERFACESETTINGSPAGE_H
#define INTERFACESETTINGSPAGE_H

#include "settings/settingspagebase.h"

#include <QWidget>

class QUrl;
class SettingsPageBase;

/**
 * @brief Page for the 'Interface' settings of the Dolphin settings dialog.
 *
 * The interface settings include:
 * - Folders & Tabs
 * - Previews
 * - Context Menu
 */
class InterfaceSettingsPage : public SettingsPageBase
{
    Q_OBJECT

public:
    InterfaceSettingsPage(QWidget *parent);
    ~InterfaceSettingsPage() override;

    /** @see SettingsPageBase::applySettings() */
    void applySettings() override;

    /** @see SettingsPageBase::restoreDefaults() */
    void restoreDefaults() override;

private:
    QList<SettingsPageBase *> m_pages;
};

#endif
