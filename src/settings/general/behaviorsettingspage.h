/*
 * SPDX-FileCopyrightText: 2006 Peter Penz <peter.penz@gmx.at>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
#ifndef BEHAVIORSETTINGSPAGE_H
#define BEHAVIORSETTINGSPAGE_H

#include "dolphin_generalsettings.h"
#include "settings/settingspagebase.h"

#include <QUrl>

class QCheckBox;
class QLabel;
class QRadioButton;

/**
 * @brief Tab page for the 'Behavior' settings of the Dolphin settings dialog.
 */
class BehaviorSettingsPage : public SettingsPageBase
{
    Q_OBJECT

public:
    BehaviorSettingsPage(const QUrl &url, QWidget *parent);
    ~BehaviorSettingsPage() override;

    /** @see SettingsPageBase::applySettings() */
    void applySettings() override;

    /** @see SettingsPageBase::restoreDefaults() */
    void restoreDefaults() override;

private:
    void loadSettings();
    void setSortingChoiceValue(GeneralSettings *settings);
    void loadSortingChoiceSettings();

private:
    QUrl m_url;

    QRadioButton *m_localViewProps;
    QRadioButton *m_globalViewProps;

    QCheckBox *m_showToolTips;
    QLabel *m_configureToolTips;
    QCheckBox *m_showSelectionToggle;

    QRadioButton *m_naturalSorting;
    QRadioButton *m_caseSensitiveSorting;
    QRadioButton *m_caseInsensitiveSorting;

    QCheckBox *m_renameInline;
    QCheckBox *m_useTabForSplitViewSwitch;
    QCheckBox *m_closeActiveSplitView;
};

#endif
