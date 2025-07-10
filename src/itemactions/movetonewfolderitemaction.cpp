/*
 * SPDX-FileCopyrightText: 2024 Ahmet Hakan Çelik <mail@ahakan.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "movetonewfolderitemaction.h"

#include <KFileItem>
#include <KLocalizedString>
#include <KPluginFactory>
#include <KNewFileMenu>
#include <KJobWidgets>
#include <KIO/CopyJob>
#include <KIO/JobUiDelegate>
#include <KIO/FileUndoManager>

#include <QUrl>

K_PLUGIN_CLASS_WITH_JSON(MoveToNewFolderItemAction, "movetonewfolderitemaction.json")

MoveToNewFolderItemAction::MoveToNewFolderItemAction(QObject *parent)
    : KAbstractFileItemActionPlugin(parent)
{

}

QList<QAction *> MoveToNewFolderItemAction::actions(const KFileItemListProperties &fileItemInfos, QWidget *parentWidget)
{
    const KFileItemList &selectedItems = fileItemInfos.items();
    
    if (selectedItems.size() == 1 && selectedItems[0].isDir()) {
        // skip single directory like the current working directory
        return {};
    }

    QAction *createFolderFromSelected = new QAction(i18nc("@action:inmenu", "Move to New Folder…"), parentWidget);
    createFolderFromSelected->setIcon(QIcon::fromTheme(QStringLiteral("folder-new")));
    connect(createFolderFromSelected, &QAction::triggered, this, [=]() {
        const QUrl selectedFileDirPath = selectedItems.at(0).url().adjusted(QUrl::RemoveFilename | QUrl::StripTrailingSlash);
        auto newFileMenu = new KNewFileMenu(parentWidget);
        newFileMenu->setWorkingDirectory(selectedFileDirPath);
        newFileMenu->createDirectory();

        connect(newFileMenu, &KNewFileMenu::directoryCreated, this, [=](const QUrl &createdUrl) {
            KIO::CopyJob *job = KIO::move(selectedItems.urlList(), createdUrl);
            KJobWidgets::setWindow(job, parentWidget);
            KIO::FileUndoManager::self()->recordCopyJob(job);
            newFileMenu->deleteLater();
        });
    });

    return {createFolderFromSelected};
}

#include "movetonewfolderitemaction.moc"

#include "moc_movetonewfolderitemaction.cpp"
