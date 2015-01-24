/***************************************************************************
 *   Copyright (C) 2011 by Peter Penz <peter.penz19@gmail.com>             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA          *
 ***************************************************************************/

#include "configurepreviewplugindialog.h"

#include <KPluginLoader>
#include <KLocalizedString>
#include <KJobWidgets>
#include <KIO/JobUiDelegate>
#include <KIO/DeleteJob>
#include <KIO/ThumbCreator>

#include <QUrl>
#include <QLibrary>
#include <QVBoxLayout>
#include <QStandardPaths>

ConfigurePreviewPluginDialog::ConfigurePreviewPluginDialog(const QString& pluginName,
                                                           const QString& desktopEntryName,
                                                           QWidget* parent) :
    KDialog(parent)
{
    QSharedPointer<ThumbCreator> previewPlugin;
    const QString pluginPath = KPluginLoader::findPlugin(desktopEntryName);
    if (!pluginPath.isEmpty()) {
        newCreator create = (newCreator)QLibrary::resolve(pluginPath, "new_creator");
        if (create) {
            previewPlugin.reset(dynamic_cast<ThumbCreator*>(create()));
        }
    }

    setCaption(i18nc("@title:window", "Configure Preview for %1", pluginName));
    setMinimumWidth(400);
    setButtons(Ok | Cancel);
    setDefaultButton(Ok);

    if (previewPlugin) {
        auto mainWidget = new QWidget(this);
        mainWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
        setMainWidget(mainWidget);

        auto configurationWidget = previewPlugin->createConfigurationWidget();
        configurationWidget->setParent(mainWidget);

        auto layout = new QVBoxLayout(mainWidget);
        layout->addWidget(configurationWidget);
        layout->addStretch();

        connect(this, &ConfigurePreviewPluginDialog::okClicked, this, [=] {
            // TODO: It would be great having a mechanism to tell PreviewJob that only previews
            // for a specific MIME-type should be regenerated. As this is not available yet we
            // delete the whole thumbnails directory.
            previewPlugin->writeConfiguration(configurationWidget);

            // http://specifications.freedesktop.org/thumbnail-spec/thumbnail-spec-latest.html#DIRECTORY
            const QString thumbnailsPath = QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation) + QLatin1String("/thumbnails/");
            KIO::del(QUrl::fromLocalFile(thumbnailsPath), KIO::HideProgressInfo);
        });
    }
}