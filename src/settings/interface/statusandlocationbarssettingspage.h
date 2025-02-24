/*
 * SPDX-FileCopyrightText: 2009 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef STATUSANDLOCATIONBARSSETTINGSPAGE_H
#define STATUSANDLOCATIONBARSSETTINGSPAGE_H

#include "dolphin_generalsettings.h"
#include "folderstabssettingspage.h"
#include "settings/settingspagebase.h"

#include <QUrl>

class QCheckBox;
class QLineEdit;
class QLabel;
class QRadioButton;
class QButtonGroup;

/**
 * @brief Tab page for the 'Behavior' settings of the Dolphin settings dialog.
 */
class StatusAndLocationBarsSettingsPage : public SettingsPageBase
{
    Q_OBJECT

public:
    StatusAndLocationBarsSettingsPage(QWidget *parent, FoldersTabsSettingsPage *foldersPage);
    ~StatusAndLocationBarsSettingsPage() override;

    /** @see SettingsPageBase::applySettings() */
    void applySettings() override;

    /** @see SettingsPageBase::restoreDefaults() */
    void restoreDefaults() override;

private Q_SLOTS:
    void locationSlotSettingsChanged();
    void locationUpdateInitialViewOptions();

private:
    void loadSettings();
    void onShowStatusBarToggled();

private:
    FoldersTabsSettingsPage *foldersTabsPage;
    QCheckBox *m_editableUrl;
    QCheckBox *m_showFullPath;

    QButtonGroup *m_statusBarButtonGroup;
    QRadioButton *m_showStatusBarSmall;
    QRadioButton *m_showStatusBarFullWidth;
    QCheckBox *m_showZoomSlider;
    QRadioButton *m_disableStatusBar;
};

#endif
