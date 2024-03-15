/*
 * SPDX-FileCopyrightText: 2024 Benedikt Thiemer <numerfolt@posteo.de>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef PANELSETTINGSPAGE_H
#define PANELSETTINGSPAGE_H

#include "config-dolphin.h"
#include "settings/settingspagebase.h"

class QCheckBox;
class QRadioButton;

/**
 * @brief Page for the information panel.
 */
class PanelSettingsPage : public SettingsPageBase
{
    Q_OBJECT

public:
    explicit PanelSettingsPage(QWidget *parent = nullptr);
    ~PanelSettingsPage() override;

    /** @see SettingsPageBase::applySettings() */
    void applySettings() override;

    /** @see SettingsPageBase::restoreDefaults() */
    void restoreDefaults() override;

private:
    void loadSettings();
    void showPreviewToggled();

private:
    QCheckBox *m_showPreview;
    QCheckBox *m_autoPlayMedia;
    QCheckBox *m_showHovered;
    QRadioButton *m_dateFormatLong;
    QRadioButton *m_dateFormatShort;
};

#endif
