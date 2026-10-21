/*
 * SPDX-FileCopyrightText: 2026 Méven Car <meven@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "../itemactions/movetonewfolderitemaction.h"

#include <KFileItem>
#include <KFileItemListProperties>
#include <KProtocolInfo>

#include <QAction>
#include <QFile>
#include <QScopeGuard>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>

class MoveToNewFolderItemActionTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void offeredForFilesInAWritableFolder();
    void notOfferedInAReadOnlyFolder();
    void notOfferedForRecentlyUsedItems();

private:
    static KFileItemList createFiles(const QString &folder);
    static int actionCount(const KFileItemList &items);
};

void MoveToNewFolderItemActionTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
}

KFileItemList MoveToNewFolderItemActionTest::createFiles(const QString &folder)
{
    KFileItemList items;
    for (const QString &name : {QStringLiteral("a.txt"), QStringLiteral("b.txt")}) {
        QFile file(folder + QLatin1Char('/') + name);
        file.open(QIODevice::WriteOnly);
        items << KFileItem(QUrl::fromLocalFile(file.fileName()));
    }
    return items;
}

int MoveToNewFolderItemActionTest::actionCount(const KFileItemList &items)
{
    MoveToNewFolderItemAction plugin(nullptr);
    QWidget parentWidget;
    const QList<QAction *> actions = plugin.actions(KFileItemListProperties(items), &parentWidget);
    return actions.count();
}

void MoveToNewFolderItemActionTest::offeredForFilesInAWritableFolder()
{
    QTemporaryDir dir;
    QCOMPARE(actionCount(createFiles(dir.path())), 1);
}

void MoveToNewFolderItemActionTest::notOfferedInAReadOnlyFolder()
{
    QTemporaryDir dir;
    const KFileItemList items = createFiles(dir.path());
    QFile::setPermissions(dir.path(), QFileDevice::ReadOwner | QFileDevice::ExeOwner);
    auto restore = qScopeGuard([&dir]() {
        QFile::setPermissions(dir.path(), QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
    });

    QCOMPARE(actionCount(items), 0);
}

void MoveToNewFolderItemActionTest::notOfferedForRecentlyUsedItems()
{
    if (!KProtocolInfo::isKnownProtocol(QStringLiteral("recentlyused"))) {
        QSKIP("The recentlyused worker is not installed.");
    }
    // Recently used items keep a recentlyused:/ url, with the file as their target.
    QTemporaryDir dir;
    KFileItemList items;
    for (const KFileItem &file : createFiles(dir.path())) {
        KIO::UDSEntry entry;
        entry.fastInsert(KIO::UDSEntry::UDS_NAME, file.name() + QStringLiteral("-1"));
        entry.fastInsert(KIO::UDSEntry::UDS_FILE_TYPE, S_IFREG);
        entry.fastInsert(KIO::UDSEntry::UDS_TARGET_URL, file.url().toString());
        entry.fastInsert(KIO::UDSEntry::UDS_LOCAL_PATH, file.localPath());
        items << KFileItem(entry, QUrl(QStringLiteral("recentlyused:/")), false, true);
    }

    QCOMPARE(actionCount(items), 0);
}

QTEST_MAIN(MoveToNewFolderItemActionTest)

#include "movetonewfolderitemactiontest.moc"
