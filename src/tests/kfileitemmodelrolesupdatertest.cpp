/*
 * SPDX-FileCopyrightText: 2026 Méven Car <meven@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kitemviews/kfileitemmodelrolesupdater.h"
#include "kitemviews/kfileitemmodel.h"
#include "testdir.h"

#include <QCryptographicHash>
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
    void testWhatIsFarBehindTheViewIsGivenBack();
    void testRequestedBucketHitIsShownAndFinished();
    void testSmallerBucketFallbackIsShownButNotFinished();

private:
    // Writes a valid freedesktop thumbnail of the given pixel size for url into the given cache
    // tier ("normal", "large", ...). The cache is redirected to a temporary location by the test
    // mode enabled in initTestCase().
    static void writeCachedThumbnail(const QUrl &url, const QString &tier, int sizePx, qint64 mtimeSecs, qint64 fileSize);

    // Creates a single file, loads its directory into the model, and returns its item.
    KFileItem loadSingleFile();

    // Sets the updater up for a pass over a single visible item, with previews shown at a 256 px
    // request and device pixel ratio 1.
    void primeUpdater();

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

/**
 * A thumbnail more than two screens behind the view, on the side the view is moving away from, is
 * given back: the model no longer holds it and it counts as unresolved again.
 */
void KFileItemModelRolesUpdaterTest::testWhatIsFarBehindTheViewIsGivenBack()
{
    const int fileCount = 400;
    for (int i = 0; i < fileCount; ++i) {
        m_testDir->createFile(QStringLiteral("image%1.png").arg(i, 3, 10, QLatin1Char('0')));
    }
    QSignalSpy loadingCompletedSpy(m_model, &KFileItemModel::directoryLoadingCompleted);
    m_model->loadDirectory(m_testDir->url());
    QVERIFY(loadingCompletedSpy.wait());

    m_updater = new KFileItemModelRolesUpdater(m_model, this);
    m_updater->setDevicePixelRatio(1.0);
    m_updater->setIconSize(QSize(128, 128));
    m_updater->m_previewShown = true;
    m_updater->setMaximumVisibleItems(10);
    m_updater->setVisibleIndexRange(200, 10);

    // Stand in for the previews of a window around the view, both further behind it than two
    // screens and just behind it.
    QPixmap thumbnail(128, 128);
    thumbnail.fill(Qt::blue);
    const QList<int> resolved{150, 199, 285, 300};
    for (int index : resolved) {
        SmallHash data;
        data.insert("iconPixmap", thumbnail);
        m_model->setData(index, data);
        m_updater->m_finishedItems.insert(m_model->fileItem(index));
    }

    // Ten screens on, the two screens behind the view begin at 280.
    m_updater->setVisibleIndexRange(300, 10);

    QVERIFY(!m_updater->m_finishedItems.contains(m_model->fileItem(150)));
    QVERIFY(!m_updater->m_finishedItems.contains(m_model->fileItem(199)));
    QVERIFY(m_model->data(150).value(QByteArrayLiteral("iconPixmap")).value<QPixmap>().isNull());
    QVERIFY(m_model->data(199).value(QByteArrayLiteral("iconPixmap")).value<QPixmap>().isNull());

    // What is within two screens behind the view, and what is in view, stays.
    QVERIFY(m_updater->m_finishedItems.contains(m_model->fileItem(285)));
    QVERIFY(m_updater->m_finishedItems.contains(m_model->fileItem(300)));
    QVERIFY(!m_model->data(285).value(QByteArrayLiteral("iconPixmap")).value<QPixmap>().isNull());
    QVERIFY(!m_model->data(300).value(QByteArrayLiteral("iconPixmap")).value<QPixmap>().isNull());
}

void KFileItemModelRolesUpdaterTest::writeCachedThumbnail(const QUrl &url, const QString &tier, int sizePx, qint64 mtimeSecs, qint64 fileSize)
{
    const QByteArray encoded = url.toEncoded(QUrl::RemovePassword | QUrl::FullyEncoded);
    const QString name = QString::fromLatin1(QCryptographicHash::hash(encoded, QCryptographicHash::Md5).toHex()) + QStringLiteral(".png");
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::GenericCacheLocation) + QStringLiteral("/thumbnails/") + tier + QLatin1Char('/');
    QDir().mkpath(dir);

    QImage thumb(sizePx, sizePx, QImage::Format_ARGB32);
    thumb.fill(Qt::blue);
    thumb.setText(QStringLiteral("Thumb::URI"), QString::fromUtf8(encoded));
    thumb.setText(QStringLiteral("Thumb::MTime"), QString::number(mtimeSecs));
    thumb.setText(QStringLiteral("Thumb::Size"), QString::number(fileSize));
    thumb.save(dir + name, "png");
}

KFileItem KFileItemModelRolesUpdaterTest::loadSingleFile()
{
    m_testDir->createFile("image.png");
    QSignalSpy loadingCompletedSpy(m_model, &KFileItemModel::directoryLoadingCompleted);
    m_model->loadDirectory(m_testDir->url());
    loadingCompletedSpy.wait();
    return m_model->fileItem(0);
}

void KFileItemModelRolesUpdaterTest::primeUpdater()
{
    m_updater = new KFileItemModelRolesUpdater(m_model, this);
    m_updater->setDevicePixelRatio(1.0);
    // An icon size above 128 selects the 256 px ("large") cache bucket.
    m_updater->setIconSize(QSize(256, 256));
    // Set the members directly so the asynchronous update flow is not started.
    m_updater->m_previewShown = true;
    m_updater->m_firstVisibleIndex = 0;
    m_updater->m_lastVisibleIndex = 0;
}

void KFileItemModelRolesUpdaterTest::testRequestedBucketHitIsShownAndFinished()
{
    const KFileItem item = loadSingleFile();
    QCOMPARE(m_model->count(), 1);
    const int index = m_model->index(item);

    // A current thumbnail cached in the requested (large) bucket.
    writeCachedThumbnail(item.targetUrl(), QStringLiteral("large"), 256, item.time(KFileItem::ModificationTime).toSecsSinceEpoch(), item.size());

    primeUpdater();
    // The pass the updater makes over the items on screen, which asks the cache through a job that
    // makes nothing and writes what it finds into the model.
    m_updater->readCachedPreviewsForVisibleItems();
    QTRY_VERIFY(!m_model->data(0).value(QByteArrayLiteral("iconPixmap")).value<QPixmap>().isNull());

    // Shown on the first paint, and final: the item is finished, so the asynchronous
    // preview job will not read the cache entry again.
    SmallHash data = m_model->data(index);
    QVERIFY(!data["iconPixmap"].value<QPixmap>().isNull());
    QVERIFY(m_updater->m_finishedItems.contains(item));
}

void KFileItemModelRolesUpdaterTest::testSmallerBucketFallbackIsShownButNotFinished()
{
    const KFileItem item = loadSingleFile();
    QCOMPARE(m_model->count(), 1);
    const int index = m_model->index(item);

    // Nothing is cached in the requested (large) bucket, only in the smaller (normal) one.
    writeCachedThumbnail(item.targetUrl(), QStringLiteral("normal"), 128, item.time(KFileItem::ModificationTime).toSecsSinceEpoch(), item.size());

    primeUpdater();
    // The pass the updater makes over the items on screen, which asks the cache through a job that
    // makes nothing and writes what it finds into the model.
    m_updater->readCachedPreviewsForVisibleItems();
    QTRY_VERIFY(!m_model->data(0).value(QByteArrayLiteral("iconPixmap")).value<QPixmap>().isNull());

    // The smaller thumbnail is shown at once (upscaled) rather than a generic icon, but the
    // item is left unfinished so the asynchronous job still generates the proper size.
    SmallHash data = m_model->data(index);
    QVERIFY(!data["iconPixmap"].value<QPixmap>().isNull());
    QVERIFY(!m_updater->m_finishedItems.contains(item));
}

QTEST_MAIN(KFileItemModelRolesUpdaterTest)

#include "kfileitemmodelrolesupdatertest.moc"
