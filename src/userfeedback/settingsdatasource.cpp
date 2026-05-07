/*
 * SPDX-FileCopyrightText: 2020 Elvis Angelaccio <elvis.angelaccio@kde.org
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "settingsdatasource.h"
#include "dolphin_generalsettings.h"
#include "dolphinmainwindow.h"

#include <KLocalizedString>
#include <KUserFeedback/Provider>

#include <QApplication>
#include <QVariant>

SettingsDataSource::SettingsDataSource()
    : KUserFeedback::AbstractDataSource(QStringLiteral("settings"), KUserFeedback::Provider::DetailedSystemInformation)
{
}

QString SettingsDataSource::name() const
{
    return i18nc("name of kuserfeedback data source provided by dolphin", "Settings");
}

QString SettingsDataSource::description() const
{
    return i18nc("description of kuserfeedback data source provided by dolphin", "A subset of Dolphin settings.");
}

QVariant SettingsDataSource::data()
{
    QVariantMap map;

    // This assumes there is only one DolphinMainWindow per process.
    const auto topLevelWidgets = QApplication::topLevelWidgets();
    for (const auto widget : topLevelWidgets) {
        if (auto mainWindow = qobject_cast<DolphinMainWindow *>(widget)) {
            map.insert(QStringLiteral("informationPanelEnabled"), mainWindow->isInformationPanelEnabled());
            map.insert(QStringLiteral("foldersPanelEnabled"), mainWindow->isFoldersPanelEnabled());
            break;
        }
    }

    map.insert(QStringLiteral("tooltipsEnabled"), GeneralSettings::showToolTips());
    map.insert(QStringLiteral("browseArchivesEnable"), GeneralSettings::browseThroughArchives());

    return map;
}
