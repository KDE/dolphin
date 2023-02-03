/*
 * SPDX-FileCopyrightText: 2020 Elvis Angelaccio <elvis.angelaccio@kde.org
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "userfeedbacksettingspage.h"
#include "userfeedback/dolphinfeedbackprovider.h"

#include <KUserFeedback/FeedbackConfigWidget>
#include <KUserFeedback/Provider>

#include <QVBoxLayout>

UserFeedbackSettingsPage::UserFeedbackSettingsPage(QWidget *parent)
    : SettingsPageBase(parent)
{
    auto layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);

    m_feedbackWidget = new KUserFeedback::FeedbackConfigWidget(this);
    m_feedbackWidget->setFeedbackProvider(DolphinFeedbackProvider::instance());

    layout->addWidget(m_feedbackWidget);

    connect(m_feedbackWidget, &KUserFeedback::FeedbackConfigWidget::configurationChanged, this, &UserFeedbackSettingsPage::changed);
}

UserFeedbackSettingsPage::~UserFeedbackSettingsPage()
{
}

void UserFeedbackSettingsPage::applySettings()
{
    auto feedbackProvider = DolphinFeedbackProvider::instance();
    feedbackProvider->setTelemetryMode(m_feedbackWidget->telemetryMode());
    feedbackProvider->setSurveyInterval(m_feedbackWidget->surveyInterval());
}

void UserFeedbackSettingsPage::restoreDefaults()
{
    auto feedbackProvider = DolphinFeedbackProvider::instance();
    feedbackProvider->setTelemetryMode(KUserFeedback::Provider::NoTelemetry);
    feedbackProvider->setSurveyInterval(-1);
}
