/*
 * SPDX-FileCopyrightText: 2020 Elvis Angelaccio <elvis.angelaccio@kde.org
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef USERFEEDBACKSETTINGSPAGE_H
#define USERFEEDBACKSETTINGSPAGE_H

#include "settings/settingspagebase.h"

namespace KUserFeedback
{
class FeedbackConfigWidget;
}

/**
 * @brief Page for the 'User Feedback' settings of the Dolphin settings dialog.
 */
class UserFeedbackSettingsPage : public SettingsPageBase
{
    Q_OBJECT

public:
    explicit UserFeedbackSettingsPage(QWidget *parent);
    ~UserFeedbackSettingsPage() override;

    /** @see SettingsPageBase::applySettings() */
    void applySettings() override;

    /** @see SettingsPageBase::restoreDefaults() */
    void restoreDefaults() override;

private:
    KUserFeedback::FeedbackConfigWidget *m_feedbackWidget = nullptr;
};

#endif // USERFEEDBACKSETTINGSPAGE_H
