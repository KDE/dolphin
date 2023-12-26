/*
 * SPDX-FileCopyrightText: 2006 Peter Penz <peter.penz@gmx.at>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef GENERALVIEWSETTINGSPAGE_H
#define GENERALVIEWSETTINGSPAGE_H

#include "settings/settingspagebase.h"
#include <qradiobutton.h>

#include <QUrl>

class QCheckBox;
class QLabel;
class QRadioButton;

/**
 * @brief Tab page for the 'View tab' settings of the Dolphin settings dialog.
 */
class GeneralViewSettingsPage : public SettingsPageBase
{
    Q_OBJECT

public:
    explicit GeneralViewSettingsPage(const QUrl &url, QWidget *parent);
    ~GeneralViewSettingsPage() override;

    /** @see SettingsPageBase::applySettings() */
    void applySettings() override;

    /** @see SettingsPageBase::restoreDefaults() */
    void restoreDefaults() override;

private:
    void loadSettings();

private:
    QUrl m_url;
    QRadioButton *m_localViewProps;
    QRadioButton *m_globalViewProps;
    QLabel *m_configureToolTips;
    QCheckBox *m_showSelectionToggle;
    QCheckBox *m_renameInline;
    QCheckBox *m_openArchivesAsFolder;
    QCheckBox *m_autoExpandFolders;
};

#endif
