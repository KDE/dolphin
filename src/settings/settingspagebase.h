/*
 * SPDX-FileCopyrightText: 2006 Peter Penz <peter.penz@gmx.at>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef SETTINGSPAGEBASE_H
#define SETTINGSPAGEBASE_H

#include <QWidget>

/**
 * @brief Base class for the settings pages of the Dolphin settings dialog.
 */
class SettingsPageBase : public QWidget
{
    Q_OBJECT

public:
    explicit SettingsPageBase(QWidget* parent = nullptr);
    ~SettingsPageBase() override;

    /**
     * Must be implemented by a derived class to
     * persistently store the settings.
     */
    virtual void applySettings() = 0;

    /**
     * Must be implemented by a derived class to
     * restored the settings to default values.
     */
    virtual void restoreDefaults() = 0;

Q_SIGNALS:
    /** Is emitted if a setting has been changed. */
    void changed();
};

#endif
