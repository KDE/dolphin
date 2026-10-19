/*
 * SPDX-FileCopyrightText: 2006 Peter Penz <peter.penz@gmx.at>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef VIEWSETTINGSPAGE_H
#define VIEWSETTINGSPAGE_H

#include "settings/settingspagebase.h"

class ViewSettingsTab;
class QWidget;
class ContentDisplayTab;
class QTabWidget;

/**
 * @brief Page for the 'View' settings of the Dolphin settings dialog.
 *
 * The views settings allow to set the properties for the icons mode,
 * the details mode and the column mode.
 */
class ViewSettingsPage : public SettingsPageBase
{
    Q_OBJECT

public:
    explicit ViewSettingsPage(const QUrl &url, QWidget *parent);
    ~ViewSettingsPage() override;

    /** @see SettingsPageBase::applySettings() */
    void applySettings() override;

    /** @see SettingsPageBase::restoreDefaults() */
    void restoreDefaults() override;

Q_SIGNALS:
    /** Is emitted when the default display style has been written, without waiting for applySettings(). */
    void defaultViewPropertiesChanged();

private:
    ContentDisplayTab *contentDisplayTab;
    QTabWidget *tabWidget;
    QList<SettingsPageBase *> m_tabs;
};

#endif
