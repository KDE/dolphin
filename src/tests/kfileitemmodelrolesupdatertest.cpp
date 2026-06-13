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

class KFileItemModelRolesUpdaterTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void init();
    void cleanup();

    void testRequestedBucketHitIsShownAndFinished();
    void testSmallerBucketFallbackIsShownButNotFinished();

private:
    // Writes a valid freedesktop thumbnail of the given pixel size for url into the given
    // cache tier ("normal", "large", ...). The cache is redirected to a temporary location
    // by the test mode enabled in initTestCase().
    static void writeCachedThumbnail(const QUrl &url, const QString &tier, int sizePx, qint64 mtimeSecs, qint64 fileSize);

    // Creates a single file, loads its directory into the model, and returns its item.
    KFileItem loadSingleFile();

    // Sets the updater up for a synchronous updateVisibleIcons() over a single visible item,
    // with previews shown at a 256 px request and device pixel ratio 1.
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
    m_updater->updateVisibleIcons();

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
    m_updater->updateVisibleIcons();

    // The smaller thumbnail is shown at once (upscaled) rather than a generic icon, but the
    // item is left unfinished so the asynchronous job still generates the proper size.
    SmallHash data = m_model->data(index);
    QVERIFY(!data["iconPixmap"].value<QPixmap>().isNull());
    QVERIFY(!m_updater->m_finishedItems.contains(item));
}

QTEST_MAIN(KFileItemModelRolesUpdaterTest)

#include "kfileitemmodelrolesupdatertest.moc"
