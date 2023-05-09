/*
 * SPDX-FileCopyrightText: 2020 Elvis Angelaccio <elvis.angelaccio@kde.org
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "dolphinfeedbackprovider.h"
#include "placesdatasource.h"
#include "settingsdatasource.h"

#include <KUserFeedbackQt6/ApplicationVersionSource>
#include <KUserFeedbackQt6/LocaleInfoSource>
#include <KUserFeedbackQt6/PlatformInfoSource>
#include <KUserFeedbackQt6/QtVersionSource>
#include <KUserFeedbackQt6/ScreenInfoSource>
#include <KUserFeedbackQt6/StartCountSource>
#include <KUserFeedbackQt6/UsageTimeSource>

DolphinFeedbackProvider *DolphinFeedbackProvider::instance()
{
    static DolphinFeedbackProvider s_self;
    return &s_self;
}

DolphinFeedbackProvider::DolphinFeedbackProvider()
    : KUserFeedback::Provider()
{
    setProductIdentifier(QStringLiteral("org.kde.dolphin"));
    setFeedbackServer(QUrl(QStringLiteral("https://telemetry.kde.org")));
    setSubmissionInterval(7);

    addDataSource(new KUserFeedback::ApplicationVersionSource);
    addDataSource(new KUserFeedback::LocaleInfoSource);
    addDataSource(new KUserFeedback::PlatformInfoSource);
    addDataSource(new KUserFeedback::QtVersionSource);
    addDataSource(new KUserFeedback::ScreenInfoSource);
    addDataSource(new KUserFeedback::StartCountSource);
    addDataSource(new KUserFeedback::UsageTimeSource);
    addDataSource(new PlacesDataSource);
    addDataSource(new SettingsDataSource);
}
