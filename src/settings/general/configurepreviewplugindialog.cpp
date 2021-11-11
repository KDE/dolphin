/*
 * SPDX-FileCopyrightText: 2011 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "configurepreviewplugindialog.h"

#include <KIO/DeleteJob>
#include <KIO/JobUiDelegate>
#include <KIO/ThumbCreator>
#include <KJobWidgets>
#include <KLocalizedString>
#include <QPluginLoader>

#include <QDialogButtonBox>
#include <QPushButton>
#include <QStandardPaths>
#include <QUrl>
#include <QVBoxLayout>

ConfigurePreviewPluginDialog::ConfigurePreviewPluginDialog(const QString& pluginName,
                                                           const QString& desktopEntryName,
                                                           QWidget* parent) :
    QDialog(parent)
{
    QSharedPointer<ThumbCreator> previewPlugin;
    const QString pluginPath = QPluginLoader(desktopEntryName).fileName();
    if (!pluginPath.isEmpty()) {
        newCreator create = (newCreator)QLibrary::resolve(pluginPath, "new_creator");
        if (create) {
            previewPlugin.reset(dynamic_cast<ThumbCreator*>(create()));
        }
    }

    setWindowTitle(i18nc("@title:window", "Configure Preview for %1", pluginName));
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Minimum);
    setMinimumWidth(400);

    auto layout = new QVBoxLayout(this);

    if (previewPlugin) {
        auto configurationWidget = previewPlugin->createConfigurationWidget();
        configurationWidget->setParent(this);
        layout->addWidget(configurationWidget);

        layout->addStretch();

        connect(this, &ConfigurePreviewPluginDialog::accepted, this, [=] {
            // TODO: It would be great having a mechanism to tell PreviewJob that only previews
            // for a specific MIME-type should be regenerated. As this is not available yet we
            // delete the whole thumbnails directory.
            previewPlugin->writeConfiguration(configurationWidget);

            // https://specifications.freedesktop.org/thumbnail-spec/thumbnail-spec-latest.html#DIRECTORY
            const QString thumbnailsPath = QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation) + QLatin1String("/thumbnails/");
            KIO::del(QUrl::fromLocalFile(thumbnailsPath), KIO::HideProgressInfo);
        });
    }

    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &ConfigurePreviewPluginDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &ConfigurePreviewPluginDialog::reject);
    layout->addWidget(buttonBox);

    auto okButton = buttonBox->button(QDialogButtonBox::Ok);
    okButton->setShortcut(Qt::CTRL | Qt::Key_Return);
    okButton->setDefault(true);
}
