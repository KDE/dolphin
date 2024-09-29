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

    QAction *createFolderFromSelected = new QAction(i18nc("@action:inmenu", "Move to New Folder…"), parentWidget);
    createFolderFromSelected->setIcon(QIcon::fromTheme(QStringLiteral("folder-new")));
    connect(createFolderFromSelected, &QAction::triggered, this, [=]() {
        QString selectedFileDirPath = selectedItems.at(0).url().toString().remove(selectedItems.at(0).name());
        if (selectedFileDirPath.endsWith(QStringLiteral("/"))) {
            selectedFileDirPath.removeLast();
        }
        const QUrl newFolderDirUrl(selectedFileDirPath);

        auto newFileMenu = new KNewFileMenu(parentWidget);
        newFileMenu->setWorkingDirectory(newFolderDirUrl);
        newFileMenu->createDirectory();

        connect(newFileMenu, &KNewFileMenu::directoryCreated, this, [=](const QUrl &createdUrl) {
            KIO::CopyJob *job = KIO::move(selectedItems.urlList(), createdUrl);
            KIO::FileUndoManager::self()->recordCopyJob(job);
            newFileMenu->deleteLater();
        });
    });

    return {createFolderFromSelected};
}

#include "movetonewfolderitemaction.moc"

#include "moc_movetonewfolderitemaction.cpp"
