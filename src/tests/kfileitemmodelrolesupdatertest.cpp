/*
 * SPDX-FileCopyrightText: 2026 Méven Car <meven@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kitemviews/kfileitemmodelrolesupdater.h"
#include "kitemviews/kfileitemmodel.h"
#include "testdir.h"

#include <QDir>
#include <QFileInfo>
#include <QImage>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTest>
#include <QUrl>
#include <algorithm>

class KFileItemModelRolesUpdaterTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void init();
    void cleanup();

    void testOnlyTheWindowAroundTheViewIsResolved();
    void testTheWindowLeansTheWayTheViewIsMoving();
    void testASmallMovementDoesNotTurnTheWindowRound();

private:
    TestDir *m_testDir = nullptr;
    KFileItemModel *m_model = nullptr;
    KFileItemModelRolesUpdater *m_updater = nullptr;
};

void KFileItemModelRolesUpdaterTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    // Start from an empty thumbnail cache so only the thumbnails written here are found.
    QDir(QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation) + QStringLiteral("/thumbnails/")).removeRecursively();
}

void KFileItemModelRolesUpdaterTest::init()
{
    m_testDir = new TestDir();
    m_model = new KFileItemModel();
}

void KFileItemModelRolesUpdaterTest::cleanup()
{
    delete m_updater;
    m_updater = nullptr;
    delete m_model;
    m_model = nullptr;
    delete m_testDir;
    m_testDir = nullptr;
}

/**
 * The items whose roles are resolved are the ones in view plus the screens the window reaches, and
 * no more, however many items the directory holds.
 */
void KFileItemModelRolesUpdaterTest::testOnlyTheWindowAroundTheViewIsResolved()
{
    const int fileCount = 400;
    for (int i = 0; i < fileCount; ++i) {
        m_testDir->createFile(QStringLiteral("image%1.png").arg(i, 3, 10, QLatin1Char('0')));
    }
    QSignalSpy loadingCompletedSpy(m_model, &KFileItemModel::directoryLoadingCompleted);
    m_model->loadDirectory(m_testDir->url());
    QVERIFY(loadingCompletedSpy.wait());
    QCOMPARE(m_model->count(), fileCount);

    m_updater = new KFileItemModelRolesUpdater(m_model, this);
    m_updater->setMaximumVisibleItems(10);
    m_updater->setVisibleIndexRange(100, 10);

    const QList<int> indexes = m_updater->indexesToResolve();

    // Ten in view, six screens ahead of them and one behind.
    QCOMPARE(indexes.count(), 10 + 60 + 10);
    // What the view shows comes first, so it is made first.
    for (int i = 0; i < 10; ++i) {
        QCOMPARE(indexes.at(i), 100 + i);
    }
    const auto smallest = std::min_element(indexes.cbegin(), indexes.cend());
    const auto largest = std::max_element(indexes.cbegin(), indexes.cend());
    QCOMPARE(*smallest, 90);
    QCOMPARE(*largest, 169);
}

/**
 * The window reaches further in the direction the view is travelling, and the items on that side
 * are resolved before the ones behind it.
 */
void KFileItemModelRolesUpdaterTest::testTheWindowLeansTheWayTheViewIsMoving()
{
    const int fileCount = 400;
    for (int i = 0; i < fileCount; ++i) {
        m_testDir->createFile(QStringLiteral("image%1.png").arg(i, 3, 10, QLatin1Char('0')));
    }
    QSignalSpy loadingCompletedSpy(m_model, &KFileItemModel::directoryLoadingCompleted);
    m_model->loadDirectory(m_testDir->url());
    QVERIFY(loadingCompletedSpy.wait());

    m_updater = new KFileItemModelRolesUpdater(m_model, this);
    m_updater->setMaximumVisibleItems(10);

    // Moving forwards: six screens ahead of the view, one behind it.
    m_updater->setVisibleIndexRange(200, 10);
    QList<int> indexes = m_updater->indexesToResolve();
    QCOMPARE(*std::min_element(indexes.cbegin(), indexes.cend()), 190);
    QCOMPARE(*std::max_element(indexes.cbegin(), indexes.cend()), 269);
    // After the ten in view, the next one asked for lies ahead.
    QCOMPARE(indexes.at(10), 210);

    // Moving backwards: the six screens are now the ones the view is heading into.
    m_updater->setVisibleIndexRange(100, 10);
    indexes = m_updater->indexesToResolve();
    QCOMPARE(*std::min_element(indexes.cbegin(), indexes.cend()), 40);
    QCOMPARE(*std::max_element(indexes.cbegin(), indexes.cend()), 119);
    QCOMPARE(indexes.at(10), 99);
}

/**
 * The window keeps reaching the way the view has been travelling until the view moves back by half
 * a screen, so that a notch of a wheel does not turn it round.
 */
void KFileItemModelRolesUpdaterTest::testASmallMovementDoesNotTurnTheWindowRound()
{
    const int fileCount = 400;
    for (int i = 0; i < fileCount; ++i) {
        m_testDir->createFile(QStringLiteral("image%1.png").arg(i, 3, 10, QLatin1Char('0')));
    }
    QSignalSpy loadingCompletedSpy(m_model, &KFileItemModel::directoryLoadingCompleted);
    m_model->loadDirectory(m_testDir->url());
    QVERIFY(loadingCompletedSpy.wait());

    m_updater = new KFileItemModelRolesUpdater(m_model, this);
    m_updater->setMaximumVisibleItems(10);
    m_updater->setVisibleIndexRange(200, 10);

    // Four items back is less than half of the ten on screen: still six screens ahead, one behind.
    m_updater->setVisibleIndexRange(196, 10);
    QList<int> indexes = m_updater->indexesToResolve();
    QCOMPARE(*std::max_element(indexes.cbegin(), indexes.cend()), 265);
    QCOMPARE(*std::min_element(indexes.cbegin(), indexes.cend()), 186);

    // Five items back is half of them, and the window turns round.
    m_updater->setVisibleIndexRange(191, 10);
    indexes = m_updater->indexesToResolve();
    QCOMPARE(*std::min_element(indexes.cbegin(), indexes.cend()), 131);
    QCOMPARE(*std::max_element(indexes.cbegin(), indexes.cend()), 210);
}

QTEST_MAIN(KFileItemModelRolesUpdaterTest)

#include "kfileitemmodelrolesupdatertest.moc"
