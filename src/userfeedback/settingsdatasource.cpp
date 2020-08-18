/*
 * SPDX-FileCopyrightText: 2020 Elvis Angelaccio <elvis.angelaccio@kde.org
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "settingsdatasource.h"
#include "dolphinmainwindow.h"
#include "dolphin_generalsettings.h"

#include <KLocalizedString>
#include <KUserFeedback/Provider>

#include <QApplication>
#include <QVariant>

SettingsDataSource::SettingsDataSource()
    : KUserFeedback::AbstractDataSource(QStringLiteral("settings"), KUserFeedback::Provider::DetailedSystemInformation)
{}

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
    if (!m_mainWindow) {
        // This assumes there is only one DolphinMainWindow per process.
        const auto topLevelWidgets = QApplication::topLevelWidgets();
        for (const auto widget : topLevelWidgets) {
            if (qobject_cast<DolphinMainWindow *>(widget)) {
                m_mainWindow = static_cast<DolphinMainWindow *>(widget);
                break;
            }
        }
    }

    QVariantMap map;

    if (m_mainWindow) {
        map.insert(QStringLiteral("informationPanelEnabled"), m_mainWindow->isInformationPanelEnabled());
        map.insert(QStringLiteral("foldersPanelEnabled"), m_mainWindow->isFoldersPanelEnabled());
    }

    map.insert(QStringLiteral("tooltipsEnabled"), GeneralSettings::showToolTips());
    map.insert(QStringLiteral("browseArchivesEnable"), GeneralSettings::browseThroughArchives());

    return map;
}
