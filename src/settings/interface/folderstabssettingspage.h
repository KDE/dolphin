/*
 * SPDX-FileCopyrightText: 2008 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef FOLDERSTABSSETTINGSPAGE_H
#define FOLDERSTABSSETTINGSPAGE_H

#include "dolphin_generalsettings.h"
#include "settings/settingspagebase.h"

#include <QUrl>
#include <qobject.h>

class QCheckBox;
class QLineEdit;
class QLabel;
class QRadioButton;

/**
 * @brief Tab page for the 'Behavior' settings of the Dolphin settings dialog.
 */
class FoldersTabsSettingsPage : public SettingsPageBase
{
    Q_OBJECT

public:
    FoldersTabsSettingsPage(QWidget *parent);
    ~FoldersTabsSettingsPage() override;

    /** @see SettingsPageBase::applySettings() */
    void applySettings() override;

    /** @see SettingsPageBase::restoreDefaults() */
    void restoreDefaults() override;

public:
    QWidget *m_homeUrlBoxLayoutContainer;
    QWidget *m_buttonBoxLayoutContainer;
    QRadioButton *m_homeUrlRadioButton;

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
    QLineEdit *m_homeUrl;
    QRadioButton *m_rememberOpenedTabsRadioButton;

    QRadioButton *m_openNewTabAfterLastTab;
    QRadioButton *m_openNewTabAfterCurrentTab;

    QCheckBox *m_splitView;
    QCheckBox *m_filterBar;
    QCheckBox *m_showFullPathInTitlebar;
    QCheckBox *m_openExternallyCalledFolderInNewTab;
    QCheckBox *m_useTabForSplitViewSwitch;
    QCheckBox *m_closeActiveSplitView;
};

#endif
