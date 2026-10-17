/*
 * SPDX-FileCopyrightText: 2026 Méven Car <meven@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "statusbar/mountpointobserver.h"

#include <KMountPoint>

#include <QDir>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QStorageInfo>
#include <QTemporaryDir>
#include <QTest>

class MountPointObserverTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void testSpaceOfTheFolderFileSystem();
    void testOneObserverPerMountPoint();
};

namespace
{
// The size and the available space that the observer reports, or nothing if it does not report within 10 seconds.
std::optional<std::pair<quint64, quint64>> reportedSpace(MountPointObserver *observer)
{
    QSignalSpy spy(observer, &MountPointObserver::spaceInfoChanged);
    observer->update();
    if (!spy.wait(10000)) {
        return std::nullopt;
    }
    return std::make_pair(spy.at(0).at(0).value<quint64>(), spy.at(0).at(1).value<quint64>());
}
}

void MountPointObserverTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
}

void MountPointObserverTest::testSpaceOfTheFolderFileSystem()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());

    MountPointObserver *observer = MountPointObserver::observerForUrl(QUrl::fromLocalFile(dir.path()));
    observer->ref();
    const auto space = reportedSpace(observer);
    observer->deref();

    QVERIFY(space.has_value());
    const QStorageInfo storage(dir.path());
    QCOMPARE(space->first, quint64(storage.bytesTotal()));
    QVERIFY(space->second > 0);
}

void MountPointObserverTest::testOneObserverPerMountPoint()
{
    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    QVERIFY(QDir(dir.path()).mkdir(QStringLiteral("sub")));

    MountPointObserver *observer = MountPointObserver::observerForUrl(QUrl::fromLocalFile(dir.path()));
    observer->ref();
    QCOMPARE(MountPointObserver::observerForUrl(QUrl::fromLocalFile(dir.filePath(QStringLiteral("sub")))), observer);

    const QString home = QDir::homePath();
    const auto tempMount = KMountPoint::currentMountPoints().findByPath(dir.path());
    const auto homeMount = KMountPoint::currentMountPoints().findByPath(home);
    if (tempMount && homeMount && tempMount->mountPoint() != homeMount->mountPoint()) {
        QVERIFY(MountPointObserver::observerForUrl(QUrl::fromLocalFile(home)) != observer);
    }
    observer->deref();
}

QTEST_GUILESS_MAIN(MountPointObserverTest)

#include "mountpointobservertest.moc"
