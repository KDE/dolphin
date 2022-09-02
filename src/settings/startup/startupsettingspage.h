/*
 * SPDX-FileCopyrightText: 2008 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef STARTUPSETTINGSPAGE_H
#define STARTUPSETTINGSPAGE_H

#include "settings/settingspagebase.h"

#include <QUrl>

class QCheckBox;
class QLineEdit;
class QRadioButton;

/**
 * @brief Page for the 'Startup' settings of the Dolphin settings dialog.
 *
 * The startup settings allow to set the home URL and to configure the
 * state of the view mode, split mode and the filter bar when starting Dolphin.
 */
class StartupSettingsPage : public SettingsPageBase
{
    Q_OBJECT

public:
    StartupSettingsPage(const QUrl& url, QWidget* parent);
    ~StartupSettingsPage() override;

    /** @see SettingsPageBase::applySettings() */
    void applySettings() override;

    /** @see SettingsPageBase::restoreDefaults() */
    void restoreDefaults() override;

private Q_SLOTS:
    void slotSettingsChanged();
    void updateInitialViewOptions();
    void selectHomeUrl();
    void useCurrentLocation();
    void useDefaultLocation();

private:
    void loadSettings();
    void showSetDefaultDirectoryError();

private:
    QUrl m_url;
    QLineEdit* m_homeUrl;
    QWidget* m_homeUrlBoxLayoutContainer;
    QWidget* m_buttonBoxLayoutContainer;
    QRadioButton* m_rememberOpenedTabsRadioButton;
    QRadioButton* m_homeUrlRadioButton;

    QCheckBox* m_splitView;
    QCheckBox* m_editableUrl;
    QCheckBox* m_showFullPath;
    QCheckBox* m_filterBar;
    QCheckBox* m_showFullPathInTitlebar;
};

#endif
