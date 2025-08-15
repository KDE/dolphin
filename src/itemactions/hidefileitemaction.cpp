/*
 * SPDX-FileCopyrightText: 2025 Kostiantyn Korchuhanov <kostiantyn.korchanov@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "hidefileitemaction.h"
#include "../dolphindebug.h"

#ifdef QT_DBUS_LIB
#include <KDirNotify>
#endif
#include <KConfigGroup>
#include <KFileItem>
#include <KFileItemListProperties>
#include <KLocalizedString>
#include <KPluginFactory>
#include <KSharedConfig>

#include <QAction>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QUrl>

K_PLUGIN_CLASS_WITH_JSON(HideFileItemAction, "hidefileitemaction.json");

HideFileItemAction::HideFileItemAction(QObject *parent)
    : KAbstractFileItemActionPlugin(parent)
{
}

static bool pluginIsEnabled()
{
    KConfigGroup showGroup(KSharedConfig::openConfig(QStringLiteral("kservicemenurc")), QStringLiteral("Show"));
    return showGroup.readEntry(QStringLiteral("hidefileitemaction"), false);
}

static QStringList readHiddenFileEntries(const QString &hiddenPath)
{
    QStringList entriesNames;
    QFile readHiddenFile(hiddenPath);
    if (readHiddenFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&readHiddenFile);
        while (!in.atEnd()) {
            entriesNames.append(in.readLine());
        }
    }
    return entriesNames;
}

static bool fileAlreadyHidden(const KFileItem &item)
{
    QString fileName = item.name();
    QString parentPath = QFileInfo(item.localPath()).absolutePath();
    QString hiddenPath = QDir(parentPath).filePath(".hidden");

    QStringList entriesNames = readHiddenFileEntries(hiddenPath);

    return entriesNames.contains(fileName);
}

static void writeHiddenFileEntries(const QString &hiddenPath, const QStringList &entriesNames)
{
    QFile writeHiddenFile(hiddenPath);
    if (writeHiddenFile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate)) {
        QTextStream out(&writeHiddenFile);
        out << entriesNames.join("\n");
    }
}

static void writeFileToHidden(const QString &hiddenPath, const QString &fileName)
{
    QStringList entriesNames = readHiddenFileEntries(hiddenPath);
    if (!entriesNames.contains(fileName)) {
        entriesNames.append(fileName);
        writeHiddenFileEntries(hiddenPath, entriesNames);
    }
}

static void deleteFileFromHidden(const QString &hiddenPath, const QString &fileName)
{
    QStringList entriesNames = readHiddenFileEntries(hiddenPath);
    entriesNames.removeAll(fileName);
    writeHiddenFileEntries(hiddenPath, entriesNames);
}

QList<QAction *> HideFileItemAction::actions(const KFileItemListProperties &fileItemInfos, QWidget *parentWidget)
{
    if (!pluginIsEnabled()) {
        return {};
    }
    const KFileItemList selectedItems = fileItemInfos.items();
    if (selectedItems.isEmpty()) {
        return {};
    }

    const QUrl parentFileUrl = selectedItems.first().url().adjusted(QUrl::RemoveFilename);
    const QString parentFilePath = parentFileUrl.toLocalFile();
    const QString hiddenFilePath = QDir(parentFilePath).filePath(".hidden");

    for (const KFileItem &item : selectedItems) {
        if (item.url().adjusted(QUrl::RemoveFilename).toLocalFile() != parentFilePath) {
            return {};
        }
    }

    QFileInfo parentFileInfo(parentFilePath);
    QFileInfo hiddenFileInfo(hiddenFilePath);
    const bool canWrite = parentFileInfo.isWritable() && (!hiddenFileInfo.exists() || hiddenFileInfo.isWritable());

    QAction *hideFolder = new QAction(i18nc("@action:inmenu", "Hide"), parentWidget);
    hideFolder->setIcon(QIcon::fromTheme("hide_table_row"));
    hideFolder->setEnabled(canWrite);

    connect(hideFolder, &QAction::triggered, this, [selectedItems, hiddenFilePath, parentFilePath]() {
        for (const KFileItem &item : selectedItems) {
            if (item.name() == ".hidden") {
                continue;
            }

            QString fileName = item.name();
            writeFileToHidden(hiddenFilePath, fileName);

#ifdef QT_DBUS_LIB
            org::kde::KDirNotify::emitFilesChanged({QUrl::fromLocalFile(parentFilePath)});
#endif
        }
    });

    QAction *unhideFolder = new QAction(i18nc("@action:inmenu", "Unhide"), parentWidget);
    unhideFolder->setIcon(QIcon::fromTheme("view-visible"));
    unhideFolder->setEnabled(canWrite);

    connect(unhideFolder, &QAction::triggered, this, [selectedItems, hiddenFilePath, parentFilePath]() {
        for (const KFileItem &item : selectedItems) {
            if (item.name() == ".hidden") {
                continue;
            }

            QString fileName = item.name();
            deleteFileFromHidden(hiddenFilePath, fileName);

#ifdef QT_DBUS_LIB
            org::kde::KDirNotify::emitFilesChanged({QUrl::fromLocalFile(parentFilePath)});
#endif
        }
    });

    bool anyFileHidden = false;
    bool anyFileVisible = false;
    for (const KFileItem &item : selectedItems) {
        if (fileAlreadyHidden(item)) {
            anyFileHidden = true;
        } else {
            anyFileVisible = true;
        }
        if (anyFileHidden && anyFileVisible) {
            break;
        }
    }
    if (anyFileHidden && anyFileVisible) {
        return {hideFolder, unhideFolder};
    }
    if (anyFileVisible) {
        return {hideFolder};
    }
    return {unhideFolder};
}

#include "hidefileitemaction.moc"
#include "moc_hidefileitemaction.cpp"
