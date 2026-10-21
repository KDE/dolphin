/*
 * SPDX-FileCopyrightText: 2026 Méven Car <meven@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kitemviews/kfileitemmodelrolesupdater.h"
#include "kitemviews/kfileitemmodel.h"
#include "testdir.h"

#include <KIO/PreviewJob>
#include <KSambaShare>

#include <QTimer>

#include <QSignalSpy>
#include <QTest>

#include "kitemviews/private/kdirectorycontentscounterworker.h"

class KFileItemModelRolesUpdaterTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void testSharesChangedUpdatesOverlays();
    void testDirectoryContentsCounterWorkerDoesNotChangeCwd();
    void testAFileThatKeepsChangingGetsItsPreviewOnceItIsQuiet();
};

void KFileItemModelRolesUpdaterTest::testSharesChangedUpdatesOverlays()
{
#ifdef Q_OS_WIN
    QSKIP("KFileItem::overlays() does not look at the samba shares on Windows");
#endif
    TestDir testDir;
    testDir.createDir(QStringLiteral("folder"));

    KFileItemModel model;
    KFileItemModelRolesUpdater updater(&model);
    updater.setRoles({"text", "type"});
    QSignalSpy loadingCompletedSpy(&model, &KFileItemModel::directoryLoadingCompleted);
    model.loadDirectory(testDir.url());
    QVERIFY(loadingCompletedSpy.wait());
    QCOMPARE(model.count(), 1);
    updater.setVisibleIndexRange(0, 1);
    // rolesData() resolved the folder, which connects to KSambaShare.
    QTRY_VERIFY(model.data(0).contains("type"));

    // An overlay that KSambaShare no longer reports, written without the updater seeing it.
    SmallHash data = model.data(0);
    data.insert("iconOverlays", QStringList{QStringLiteral("emblem-shared")});
    model.blockSignals(true);
    model.setData(0, data);
    model.blockSignals(false);

    KSambaShare *const shares = KSambaShare::instance();
    Q_EMIT shares->changed();
    QTRY_VERIFY(!model.data(0).value("iconOverlays").toStringList().contains(QStringLiteral("emblem-shared")));
}

void KFileItemModelRolesUpdaterTest::testAFileThatKeepsChangingGetsItsPreviewOnceItIsQuiet()
{
    // A file that is being downloaded changes about every 500 ms. Its preview used to be
    // generated again for each change, which kept a worker busy for the whole download. Another
    // file that changes once in the meantime still gets its preview at once.
    TestDir testDir;
    testDir.createFile(QStringLiteral("growing.txt"), "start\n");
    testDir.createFile(QStringLiteral("other.txt"), "other\n");

    KFileItemModel model;
    model.setRoles({"text", "size"});
    KFileItemModelRolesUpdater updater(&model);
    updater.setEnabledPlugins(KIO::PreviewJob::availablePlugins());
    updater.setIconSize(QSize(128, 128));
    updater.setPreviewsShown(true);
    QSignalSpy loadingCompletedSpy(&model, &KFileItemModel::directoryLoadingCompleted);
    model.loadDirectory(testDir.url());
    QVERIFY(loadingCompletedSpy.wait());
    QCOMPARE(model.count(), 2);
    updater.setVisibleIndexRange(0, model.count());
    QTRY_VERIFY(model.data(model.index(QUrl::fromLocalFile(testDir.path() + QStringLiteral("/growing.txt")))).contains("iconPixmap"));
    QTRY_VERIFY(model.data(model.index(QUrl::fromLocalFile(testDir.path() + QStringLiteral("/other.txt")))).contains("iconPixmap"));

    QHash<QString, int> previews;
    QHash<QString, int> sizeReports;
    connect(&model, &KFileItemModel::itemsChanged, this, [&](const KItemRangeList &ranges, const QSet<QByteArray> &roles) {
        for (const KItemRange &range : ranges) {
            for (int i = range.index; i < range.index + range.count; ++i) {
                if (roles.contains("iconPixmap")) {
                    ++previews[model.fileItem(i).name()];
                }
                if (roles.contains("size")) {
                    ++sizeReports[model.fileItem(i).name()];
                }
            }
        }
    });

    QFile growing(testDir.path() + QStringLiteral("/growing.txt"));
    QVERIFY(growing.open(QIODevice::Append));
    QTimer writer;
    writer.setInterval(100);
    connect(&writer, &QTimer::timeout, this, [&growing]() {
        growing.write("more data\n");
        growing.flush();
    });
    writer.start();
    QTRY_VERIFY_WITH_TIMEOUT(sizeReports.value(QStringLiteral("growing.txt")) >= 3, 10000);

    QFile other(testDir.path() + QStringLiteral("/other.txt"));
    QVERIFY(other.open(QIODevice::Append));
    other.write("changed once\n");
    other.close();
    QTRY_COMPARE_WITH_TIMEOUT(previews.value(QStringLiteral("other.txt")), 1, 1500);

    const int reportsBefore = sizeReports.value(QStringLiteral("growing.txt"));
    QTRY_VERIFY_WITH_TIMEOUT(sizeReports.value(QStringLiteral("growing.txt")) >= reportsBefore + 3, 10000);
    writer.stop();
    // The first change is resolved at once, the others wait until the file is quiet.
    QVERIFY2(previews.value(QStringLiteral("growing.txt")) <= 1, qPrintable(QString::number(previews.value(QStringLiteral("growing.txt")))));
    const int whileWriting = previews.value(QStringLiteral("growing.txt"));
    QTRY_COMPARE_WITH_TIMEOUT(previews.value(QStringLiteral("growing.txt")), whileWriting + 1, 5000);
}

void KFileItemModelRolesUpdaterTest::testDirectoryContentsCounterWorkerDoesNotChangeCwd()
{
#ifdef Q_OS_WIN
    QSKIP("fts is not used on Windows");
#endif
    TestDir testDir;
    testDir.createDir(QStringLiteral("dir1/dir2/dir3"));
    testDir.createFile(QStringLiteral("dir1/dir2/dir3/file1.txt"), "content1");
    testDir.createFile(QStringLiteral("dir1/file2.txt"), "content2");

    const QString initialCwd = QDir::currentPath();

    KDirectoryContentsCounterWorker worker;
    QSignalSpy resultSpy(&worker, &KDirectoryContentsCounterWorker::result);
    QSignalSpy intermediateSpy(&worker, &KDirectoryContentsCounterWorker::intermediateResult);

    QString intermediateCwd;
    connect(&worker, &KDirectoryContentsCounterWorker::intermediateResult, this, [&intermediateCwd]() {
        intermediateCwd = QDir::currentPath();
    });

    worker.countDirectoryContents(testDir.url().toLocalFile(), KDirectoryContentsCounterWorker::NoOptions, 10);

    QCOMPARE(resultSpy.count(), 1);
    const QList<QVariant> arguments = resultSpy.takeFirst();
    QCOMPARE(arguments.at(1).toInt(), 1); // 1 item directly inside testDir (dir1)
    if (!intermediateCwd.isEmpty()) {
        QCOMPARE(intermediateCwd, initialCwd);
    }
    QCOMPARE(QDir::currentPath(), initialCwd);
}

QTEST_MAIN(KFileItemModelRolesUpdaterTest)

#include "kfileitemmodelrolesupdatertest.moc"
