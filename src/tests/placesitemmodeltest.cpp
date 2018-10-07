/***************************************************************************
 *   Copyright (C) 2017 by Renato Araujo Oliveira <renato.araujo@kdab.com> *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA            *
 ***************************************************************************/

#include <QTest>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QAction>
#include <QDBusInterface>

#include <KBookmarkManager>
#include <KConfig>
#include <KConfigGroup>
#include <KAboutData>
#include <KFilePlacesModel>

#include "panels/places/placesitemmodel.h"
#include "panels/places/placesitem.h"
#include "views/viewproperties.h"

Q_DECLARE_METATYPE(KItemRangeList)
Q_DECLARE_METATYPE(KItemRange)

#ifdef Q_OS_WIN
//c:\ as root for windows
#define KDE_ROOT_PATH "C:\\"
#else
#define KDE_ROOT_PATH "/"
#endif

static QString bookmarksFile()
{
    return QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + "/user-places.xbel";
}

class PlacesItemModelTest : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void cleanup();

    void initTestCase();
    void cleanupTestCase();

    void testModelSort();
    void testGroups();
    void testDeletePlace();
    void testPlaceItem_data();
    void testPlaceItem();
    void testTearDownDevice();
    void testDefaultViewProperties_data();
    void testDefaultViewProperties();
    void testClear();
    void testHideItem();
    void testSystemItems();
    void testEditBookmark();
    void testEditAfterCreation();
    void testEditMetadata();
    void testRefresh();
    void testIcons_data();
    void testIcons();
    void testDragAndDrop();
    void testHideDevices();
    void testDuplicatedEntries();
    void renameAfterCreation();

private:
    PlacesItemModel* m_model;
    QSet<int> m_tobeRemoved;
    QMap<QString, QDBusInterface *> m_interfacesMap;
    int m_expectedModelCount = 15;
    bool m_hasDesktopFolder = false;
    bool m_hasDownloadsFolder = false;

    void setBalooEnabled(bool enabled);
    int indexOf(const QUrl &url);
    QDBusInterface *fakeManager();
    QDBusInterface *fakeDevice(const QString &udi);
    QStringList placesUrls(PlacesItemModel *model = nullptr) const;
    QStringList initialUrls() const;
    void createPlaceItem(const QString &text, const QUrl &url, const QString &icon);
    void removePlaceAfter(int index);
    void cancelPlaceRemoval(int index);
    void removeTestUserData();
    QMimeData *createMimeData(const QList<int> &indexes) const;
};

#define CHECK_PLACES_URLS(urls)                                             \
    {                                                                       \
        QStringList places = placesUrls();                                  \
        if (places != urls) {                                               \
            qWarning() << "Expected:" << urls;                              \
            qWarning() << "Got:" << places;                                 \
            QCOMPARE(places, urls);                                         \
        }                                                                   \
    }

void PlacesItemModelTest::setBalooEnabled(bool enabled)
{
    KConfig config(QStringLiteral("baloofilerc"));
    KConfigGroup basicSettings = config.group("Basic Settings");
    basicSettings.writeEntry("Indexing-Enabled", enabled);
    config.sync();
}

int PlacesItemModelTest::indexOf(const QUrl &url)
{
    for (int r = 0; r < m_model->count(); r++) {
        if (m_model->placesItem(r)->url() == url) {
            return r;
        }
    }
    return -1;
}

QDBusInterface *PlacesItemModelTest::fakeManager()
{
    return fakeDevice(QStringLiteral("/org/kde/solid/fakehw"));
}

QDBusInterface *PlacesItemModelTest::fakeDevice(const QString &udi)
{
    if (m_interfacesMap.contains(udi)) {
        return m_interfacesMap[udi];
    }

    QDBusInterface *iface = new QDBusInterface(QDBusConnection::sessionBus().baseService(), udi);
    m_interfacesMap[udi] = iface;

    return iface;
}

QStringList PlacesItemModelTest::placesUrls(PlacesItemModel *model) const
{
    QStringList urls;
    if (!model) {
        model = m_model;
    }

    for (int row = 0; row < model->count(); ++row) {
        urls << model->placesItem(row)->url().toDisplayString(QUrl::PreferLocalFile);
    }
    return urls;
}

QStringList PlacesItemModelTest::initialUrls() const
{
    static QStringList urls;
    if (urls.isEmpty()) {
        urls << QDir::homePath();

        if (m_hasDesktopFolder) {
            urls << QDir::homePath() + QStringLiteral("/Desktop");
        }

        if (m_hasDownloadsFolder) {
            urls << QDir::homePath() + QStringLiteral("/Downloads");
        }

        urls << QStringLiteral(KDE_ROOT_PATH) << QStringLiteral("trash:/")
             << QStringLiteral("remote:/")
             << QStringLiteral("/media/nfs")
             << QStringLiteral("timeline:/today") << QStringLiteral("timeline:/yesterday")
             << QStringLiteral("search:/documents") << QStringLiteral("search:/images") << QStringLiteral("search:/audio") << QStringLiteral("search:/videos")
             << QStringLiteral("/foreign")
             << QStringLiteral("/media/floppy0") << QStringLiteral("/media/XO-Y4") << QStringLiteral("/media/cdrom");
    }
    return urls;
}

void PlacesItemModelTest::createPlaceItem(const QString &text, const QUrl &url, const QString &icon)
{
    m_model->createPlacesItem(text, url, icon);
}

void PlacesItemModelTest::removePlaceAfter(int index)
{
    m_tobeRemoved.insert(index);
}

void PlacesItemModelTest::cancelPlaceRemoval(int index)
{
    m_tobeRemoved.remove(index);
}

void PlacesItemModelTest::removeTestUserData()
{
    // user hardcoded path to avoid removal of any user personal data
    QDir dir(QStringLiteral("/home/renato/.qttest/share/placesitemmodeltest"));
    if (dir.exists()) {
        QVERIFY(dir.removeRecursively());
    }
}

QMimeData *PlacesItemModelTest::createMimeData(const QList<int> &indexes) const
{
    QByteArray itemData;
    QDataStream stream(&itemData, QIODevice::WriteOnly);
    QList<QUrl> urls;

    for (int index : indexes) {
        const QUrl itemUrl = m_model->placesItem(index)->url();
        if (itemUrl.isValid()) {
            urls << itemUrl;
        }
        stream << index;
    }

    QMimeData* mimeData = new QMimeData();
    mimeData->setUrls(urls);
    // copied from PlacesItemModel::internalMimeType()
    const QString internalMimeType = "application/x-dolphinplacesmodel-" +
            QString::number((qptrdiff)m_model);
    mimeData->setData(internalMimeType, itemData);
    return mimeData;
}

void PlacesItemModelTest::init()
{
    m_model = new PlacesItemModel();
    // WORKAROUND: need to wait for bookmark to load, check: PlacesItemModel::updateBookmarks
    QTest::qWait(300);
    QCOMPARE(m_model->count(), m_expectedModelCount);
}

void PlacesItemModelTest::cleanup()
{
    const auto tobeRemoved = m_tobeRemoved;
    for (const int i : tobeRemoved) {
        int before = m_model->count();
        m_model->deleteItem(i);
        QTRY_COMPARE(m_model->count(), before - 1);
    }
    m_tobeRemoved.clear();
    delete m_model;
    m_model = nullptr;
    removeTestUserData();
}

void PlacesItemModelTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    // remove test user data
    removeTestUserData();

    const QString fakeHw = QFINDTESTDATA("data/fakecomputer.xml");
    QVERIFY(!fakeHw.isEmpty());
    qputenv("SOLID_FAKEHW", QFile::encodeName(fakeHw));

    setBalooEnabled(true);
    const QString bookmarsFileName = bookmarksFile();
    if (QFileInfo::exists(bookmarsFileName)) {
        // Ensure we'll have a clean bookmark file to start
        QVERIFY(QFile::remove(bookmarsFileName));
    }

    if (QDir(QStandardPaths::writableLocation(QStandardPaths::DesktopLocation)).exists()) {
        m_hasDesktopFolder = true;
        m_expectedModelCount++;
    }

    if (QDir(QStandardPaths::writableLocation(QStandardPaths::DownloadLocation)).exists()) {
        m_hasDownloadsFolder = true;
        m_expectedModelCount++;
    }

    qRegisterMetaType<KItemRangeList>();
    qRegisterMetaType<KItemRange>();
}

void PlacesItemModelTest::cleanupTestCase()
{
    qDeleteAll(m_interfacesMap);
    QFile::remove(bookmarksFile());

    // Remove any previous properties file
    removeTestUserData();
}

void PlacesItemModelTest::testModelSort()
{
    CHECK_PLACES_URLS(initialUrls());
}

void PlacesItemModelTest::testGroups()
{
    const auto groups = m_model->groups();
    int expectedGroupSize = 3;
    if (m_hasDesktopFolder) {
        expectedGroupSize++;
    }
    if (m_hasDownloadsFolder) {
        expectedGroupSize++;
    }

    QCOMPARE(groups.size(), 6);

    QCOMPARE(groups.at(0).first, 0);
    QCOMPARE(groups.at(0).second.toString(), QStringLiteral("Places"));

    QCOMPARE(groups.at(1).first, expectedGroupSize);
    QCOMPARE(groups.at(1).second.toString(), QStringLiteral("Remote"));

    QCOMPARE(groups.at(2).first, expectedGroupSize + 2);
    QCOMPARE(groups.at(2).second.toString(), QStringLiteral("Recently Saved"));

    QCOMPARE(groups.at(3).first, expectedGroupSize + 4);
    QCOMPARE(groups.at(3).second.toString(), QStringLiteral("Search For"));

    QCOMPARE(groups.at(4).first, expectedGroupSize + 8);
    QCOMPARE(groups.at(4).second.toString(), QStringLiteral("Devices"));

    QCOMPARE(groups.at(5).first, expectedGroupSize + 9);
    QCOMPARE(groups.at(5).second.toString(), QStringLiteral("Removable Devices"));
}

void PlacesItemModelTest::testPlaceItem_data()
{
    QTest::addColumn<QUrl>("url");
    QTest::addColumn<bool>("expectedIsHidden");
    QTest::addColumn<bool>("expectedIsSystemItem");
    QTest::addColumn<QString>("expectedGroup");
    QTest::addColumn<bool>("expectedStorageSetupNeeded");

    // places
    QTest::newRow("Places - Home") << QUrl::fromLocalFile(QDir::homePath()) << false << true << QStringLiteral("Places") << false;

    // baloo -search
    QTest::newRow("Baloo - Documents") << QUrl("search:/documents") << false << true << QStringLiteral("Search For") << false;

    // baloo - timeline
    QTest::newRow("Baloo - Today") << QUrl("timeline:/today") << false << true << QStringLiteral("Recently Saved") << false;

    // devices
    QTest::newRow("Devices - Floppy") << QUrl("file:///media/floppy0") << false << false << QStringLiteral("Removable Devices") << false;
}

void PlacesItemModelTest::testPlaceItem()
{
    QFETCH(QUrl, url);
    QFETCH(bool, expectedIsHidden);
    QFETCH(bool, expectedIsSystemItem);
    QFETCH(QString, expectedGroup);
    QFETCH(bool, expectedStorageSetupNeeded);

    const int index = indexOf(url);
    PlacesItem *item = m_model->placesItem(index);
    QCOMPARE(item->url(), url);
    QCOMPARE(item->isHidden(), expectedIsHidden);
    QCOMPARE(item->isSystemItem(), expectedIsSystemItem);
    QCOMPARE(item->group(), expectedGroup);
    QCOMPARE(item->storageSetupNeeded(), expectedStorageSetupNeeded);
}

void PlacesItemModelTest::testDeletePlace()
{
    const QUrl tempUrl = QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::TempLocation));
    QStringList urls = initialUrls();
    QSignalSpy itemsInsertedSpy(m_model, &PlacesItemModel::itemsInserted);
    QSignalSpy itemsRemovedSpy(m_model, &PlacesItemModel::itemsRemoved);

    PlacesItemModel *model = new PlacesItemModel();

    int tempDirIndex = 3;
    if (m_hasDesktopFolder) {
        tempDirIndex++;
    }
    if (m_hasDownloadsFolder) {
        tempDirIndex++;
    }

    // create a new place
    createPlaceItem(QStringLiteral("Temporary Dir"), tempUrl, QString());
    urls.insert(tempDirIndex, tempUrl.toLocalFile());

    // check if the new entry was created
    QTRY_COMPARE(itemsInsertedSpy.count(), 1);
    CHECK_PLACES_URLS(urls);
    QTRY_COMPARE(model->count(), m_model->count());

    // delete item
    m_model->deleteItem(tempDirIndex);

    // make sure that the new item is removed
    QTRY_COMPARE(itemsRemovedSpy.count(), 1);
    QTRY_COMPARE(m_model->count(), m_expectedModelCount);
    CHECK_PLACES_URLS(initialUrls());
    QTRY_COMPARE(model->count(), m_model->count());
}

void PlacesItemModelTest::testTearDownDevice()
{
    const QUrl mediaUrl = QUrl::fromLocalFile(QStringLiteral("/media/XO-Y4"));
    int index = indexOf(mediaUrl);
    QVERIFY(index != -1);

    auto ejectAction = m_model->ejectAction(index);
    QVERIFY(!ejectAction);

    auto teardownAction = m_model->teardownAction(index);
    QVERIFY(teardownAction);

    QCOMPARE(m_model->count(), m_expectedModelCount);

    QSignalSpy spyItemsRemoved(m_model, &PlacesItemModel::itemsRemoved);
    fakeManager()->call(QStringLiteral("unplug"), "/org/kde/solid/fakehw/volume_part1_size_993284096");
    QTRY_COMPARE(m_model->count(), m_expectedModelCount - 1);
    QCOMPARE(spyItemsRemoved.count(), 1);
    const QList<QVariant> spyItemsRemovedArgs = spyItemsRemoved.takeFirst();
    const KItemRangeList removedRange = spyItemsRemovedArgs.at(0).value<KItemRangeList>();
    QCOMPARE(removedRange.size(), 1);
    QCOMPARE(removedRange.first().index, index);
    QCOMPARE(removedRange.first().count, 1);

    QCOMPARE(indexOf(mediaUrl), -1);

    QSignalSpy spyItemsInserted(m_model, &PlacesItemModel::itemsInserted);
    fakeManager()->call(QStringLiteral("plug"), "/org/kde/solid/fakehw/volume_part1_size_993284096");
    QTRY_COMPARE(m_model->count(), m_expectedModelCount);
    QCOMPARE(spyItemsInserted.count(), 1);
    index = indexOf(mediaUrl);

    const QList<QVariant> args = spyItemsInserted.takeFirst();
    const KItemRangeList insertedRange = args.at(0).value<KItemRangeList>();
    QCOMPARE(insertedRange.size(), 1);
    QCOMPARE(insertedRange.first().index, index);
    QCOMPARE(insertedRange.first().count, 1);
}

void PlacesItemModelTest::testDefaultViewProperties_data()
{
    QTest::addColumn<QUrl>("url");
    QTest::addColumn<DolphinView::Mode>("expectedViewMode");
    QTest::addColumn<bool>("expectedPreviewShow");
    QTest::addColumn<QList<QByteArray> >("expectedVisibleRole");

    // places
    QTest::newRow("Places - Home") << QUrl::fromLocalFile(QDir::homePath()) << DolphinView::IconsView << true << QList<QByteArray>({"text"});

    // baloo -search
    QTest::newRow("Baloo - Documents") << QUrl("search:/documents") << DolphinView::DetailsView << false << QList<QByteArray>({"text", "path"});

    // audio files
    QTest::newRow("Places - Audio") << QUrl("search:/audio") << DolphinView::DetailsView << false << QList<QByteArray>({"text", "artist", "album"});

    // baloo - timeline
    QTest::newRow("Baloo - Today") << QUrl("timeline:/today") << DolphinView::DetailsView << true << QList<QByteArray>({"text", "modificationtime"});

    // devices
    QTest::newRow("Devices - Floppy") << QUrl("file:///media/floppy0") << DolphinView::IconsView << true << QList<QByteArray>({"text"});

}

void PlacesItemModelTest::testDefaultViewProperties()
{
    QFETCH(QUrl, url);
    QFETCH(DolphinView::Mode, expectedViewMode);
    QFETCH(bool, expectedPreviewShow);
    QFETCH(QList<QByteArray>, expectedVisibleRole);

    ViewProperties properties(KFilePlacesModel::convertedUrl(url));
    QCOMPARE(properties.viewMode(), expectedViewMode);
    QCOMPARE(properties.previewsShown(), expectedPreviewShow);
    QCOMPARE(properties.visibleRoles(), expectedVisibleRole);
}

void PlacesItemModelTest::testClear()
{
    QCOMPARE(m_model->count(), m_expectedModelCount);
    m_model->clear();
    QCOMPARE(m_model->count(), 0);
    QCOMPARE(m_model->hiddenCount(), 0);
    m_model->refresh();
    QTRY_COMPARE(m_model->count(), m_expectedModelCount);
}

void PlacesItemModelTest::testHideItem()
{
    const QUrl mediaUrl = QUrl::fromLocalFile(QStringLiteral("/media/XO-Y4"));
    const int index = indexOf(mediaUrl);

    PlacesItem *item = m_model->placesItem(index);

    QSignalSpy spyItemsRemoved(m_model, &PlacesItemModel::itemsRemoved);
    QList<QVariant> spyItemsRemovedArgs;
    KItemRangeList removedRange;

    QSignalSpy spyItemsInserted(m_model, &PlacesItemModel::itemsInserted);
    QList<QVariant> spyItemsInsertedArgs;
    KItemRangeList insertedRange;
    QVERIFY(item);

    // hide an item
    item->setHidden(true);

    // check if items removed was fired
    QTRY_COMPARE(m_model->count(), m_expectedModelCount - 1);
    QCOMPARE(spyItemsRemoved.count(), 1);
    spyItemsRemovedArgs = spyItemsRemoved.takeFirst();
    removedRange = spyItemsRemovedArgs.at(0).value<KItemRangeList>();
    QCOMPARE(removedRange.size(), 1);
    QCOMPARE(removedRange.first().index, index);
    QCOMPARE(removedRange.first().count, 1);

    // allow model to show hidden items
    m_model->setHiddenItemsShown(true);

    // check if the items inserted was fired
    spyItemsInsertedArgs = spyItemsInserted.takeFirst();
    insertedRange = spyItemsInsertedArgs.at(0).value<KItemRangeList>();
    QCOMPARE(insertedRange.size(), 1);
    QCOMPARE(insertedRange.first().index, index);
    QCOMPARE(insertedRange.first().count, 1);

    // mark item as visible
    item = m_model->placesItem(index);
    item->setHidden(false);

     // mark model to hide invisible items
    m_model->setHiddenItemsShown(true);

    QTRY_COMPARE(m_model->count(), m_expectedModelCount);
}

void PlacesItemModelTest::testSystemItems()
{
    int tempDirIndex = 3;
    if (m_hasDesktopFolder) {
        tempDirIndex++;
    }
    if (m_hasDownloadsFolder) {
        tempDirIndex++;
    }

    QCOMPARE(m_model->count(), m_expectedModelCount);
    for (int r = 0; r < m_model->count(); r++) {
        QCOMPARE(m_model->placesItem(r)->isSystemItem(), !m_model->placesItem(r)->device().isValid());
    }

    QSignalSpy itemsInsertedSpy(m_model, &PlacesItemModel::itemsInserted);

    // create a new entry (non system item)
    createPlaceItem(QStringLiteral("Temporary Dir"),  QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::TempLocation)), QString());

    // check if the new entry was created
    QTRY_COMPARE(itemsInsertedSpy.count(), 1);

    // make sure the new place get removed
    removePlaceAfter(tempDirIndex);

    QList<QVariant> args = itemsInsertedSpy.takeFirst();
    KItemRangeList range = args.at(0).value<KItemRangeList>();
    QCOMPARE(range.first().index, tempDirIndex);
    QCOMPARE(range.first().count, 1);
    QVERIFY(!m_model->placesItem(tempDirIndex)->isSystemItem());
    QCOMPARE(m_model->count(), m_expectedModelCount + 1);

    QTest::qWait(300);
    // check if the removal signal is correct
    QSignalSpy itemsRemovedSpy(m_model, &PlacesItemModel::itemsRemoved);
    m_model->deleteItem(tempDirIndex);
    QTRY_COMPARE(itemsRemovedSpy.count(), 1);
    args = itemsRemovedSpy.takeFirst();
    range = args.at(0).value<KItemRangeList>();
    QCOMPARE(range.first().index, tempDirIndex);
    QCOMPARE(range.first().count, 1);
    QTRY_COMPARE(m_model->count(), m_expectedModelCount);

    //cancel removal (it was removed above)
    cancelPlaceRemoval(tempDirIndex);
}

void PlacesItemModelTest::testEditBookmark()
{
    QScopedPointer<PlacesItemModel> other(new PlacesItemModel());

    createPlaceItem(QStringLiteral("Temporary Dir"), QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::TempLocation)), QString());

    // make sure that the new item will be removed later
    removePlaceAfter(5);

    QSignalSpy itemsChangedSply(m_model, &PlacesItemModel::itemsChanged);

    // modify place text
    m_model->item(3)->setText(QStringLiteral("Renamed place"));
    m_model->refresh();

    // check if the correct signal was fired
    QTRY_COMPARE(itemsChangedSply.count(), 1);
    QList<QVariant> args = itemsChangedSply.takeFirst();
    KItemRangeList range = args.at(0).value<KItemRangeList>();
    QCOMPARE(range.first().index, 3);
    QCOMPARE(range.first().count, 1);
    QSet<QByteArray> roles = args.at(1).value<QSet<QByteArray> >();
    QCOMPARE(roles.size(), 1);
    QCOMPARE(*roles.begin(), QByteArrayLiteral("text"));
    QCOMPARE(m_model->item(3)->text(), QStringLiteral("Renamed place"));

    // check if the item was updated in the other model
    QTRY_COMPARE(other->item(3)->text(), QStringLiteral("Renamed place"));
}

void PlacesItemModelTest::testEditAfterCreation()
{
    const QUrl tempUrl = QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::TempLocation));
    QSignalSpy itemsInsertedSpy(m_model, &PlacesItemModel::itemsInserted);

    // create a new place
    createPlaceItem(QStringLiteral("Temporary Dir"), tempUrl, QString());
    QTRY_COMPARE(itemsInsertedSpy.count(), 1);

    PlacesItemModel *model = new PlacesItemModel();
    QTRY_COMPARE(model->count(), m_model->count());

    // make sure that the new item will be removed later
    removePlaceAfter(5);

    // modify place text
    PlacesItem *item = m_model->placesItem(3);
    item->setText(QStringLiteral("Renamed place"));
    m_model->refresh();

    // check if the second model got the changes
    QTRY_COMPARE(model->count(), m_model->count());
    QTRY_COMPARE(model->placesItem(3)->text(), m_model->placesItem(3)->text());
    QTRY_COMPARE(model->placesItem(3)->bookmark().metaDataItem(QStringLiteral("OnlyInApp")),
                 m_model->placesItem(3)->bookmark().metaDataItem(QStringLiteral("OnlyInApp")));
    QTRY_COMPARE(model->placesItem(3)->icon(), m_model->placesItem(3)->icon());
    QTRY_COMPARE(model->placesItem(3)->url(), m_model->placesItem(3)->url());
}

void PlacesItemModelTest::testEditMetadata()
{
    const QUrl tempUrl = QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::TempLocation));
    QSignalSpy itemsInsertedSpy(m_model, &PlacesItemModel::itemsInserted);

    // create a new place
    createPlaceItem(QStringLiteral("Temporary Dir"), tempUrl, QString());
    QTRY_COMPARE(itemsInsertedSpy.count(), 1);

    // check if the new entry was created
    PlacesItemModel *model = new PlacesItemModel();
    QTRY_COMPARE(model->count(), m_model->count());

    // make sure that the new item will be removed later
    removePlaceAfter(5);

    // modify place metadata
    PlacesItem *item = m_model->placesItem(3);
    item->bookmark().setMetaDataItem(QStringLiteral("OnlyInApp"), KAboutData::applicationData().componentName());
    m_model->refresh();

    // check if the place was modified in both models
    QTRY_COMPARE(model->placesItem(3)->bookmark().metaDataItem(QStringLiteral("OnlyInApp")),
                 KAboutData::applicationData().componentName());
    QTRY_COMPARE(model->placesItem(3)->text(), m_model->placesItem(3)->text());
    QTRY_COMPARE(model->placesItem(3)->bookmark().metaDataItem(QStringLiteral("OnlyInApp")),
                 m_model->placesItem(3)->bookmark().metaDataItem(QStringLiteral("OnlyInApp")));
    QTRY_COMPARE(model->placesItem(3)->icon(), m_model->placesItem(3)->icon());
    QTRY_COMPARE(model->placesItem(3)->url(), m_model->placesItem(3)->url());
}

void PlacesItemModelTest::testRefresh()
{
    const QUrl tempUrl = QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::TempLocation));
    QSignalSpy itemsInsertedSpy(m_model, &PlacesItemModel::itemsInserted);

    // create a new place
    createPlaceItem(QStringLiteral("Temporary Dir"), tempUrl, QString());
    QTRY_COMPARE(itemsInsertedSpy.count(), 1);

    PlacesItemModel *model = new PlacesItemModel();
    QTRY_COMPARE(model->count(), m_model->count());

    // make sure that the new item will be removed later
    removePlaceAfter(5);

    PlacesItem *item = m_model->placesItem(5);
    PlacesItem *sameItem = model->placesItem(5);
    QCOMPARE(item->text(), sameItem->text());

    // modify place text
    item->setText(QStringLiteral("Renamed place"));

    // item from another model is not affected at the moment
    QVERIFY(item->text() != sameItem->text());

    // propagate change
    m_model->refresh();

    // item must be equal
    QTRY_COMPARE(item->text(), sameItem->text());
}

void PlacesItemModelTest::testIcons_data()
{
    QTest::addColumn<QUrl>("url");
    QTest::addColumn<QString>("expectedIconName");

    // places
    QTest::newRow("Places - Home") << QUrl::fromLocalFile(QDir::homePath()) << QStringLiteral("user-home");

    // baloo -search
    QTest::newRow("Baloo - Documents") << QUrl("search:/documents") << QStringLiteral("folder-text");

    // baloo - timeline
    QTest::newRow("Baloo - Today") << QUrl("timeline:/today") << QStringLiteral("go-jump-today");

    // devices
    QTest::newRow("Devices - Floppy") << QUrl("file:///media/floppy0") << QStringLiteral("blockdevice");
}

void PlacesItemModelTest::testIcons()
{
    QFETCH(QUrl, url);
    QFETCH(QString, expectedIconName);

    PlacesItem *item = m_model->placesItem(indexOf(url));
    QCOMPARE(item->icon(), expectedIconName);

    for (int r = 0; r < m_model->count(); r++) {
        QVERIFY(!m_model->placesItem(r)->icon().isEmpty());
    }
}

void PlacesItemModelTest::testDragAndDrop()
{
    QList<QVariant> args;
    KItemRangeList range;
    QStringList urls = initialUrls();
    QSignalSpy itemsInsertedSpy(m_model, &PlacesItemModel::itemsInserted);
    QSignalSpy itemsRemovedSpy(m_model, &PlacesItemModel::itemsRemoved);

    CHECK_PLACES_URLS(initialUrls());
    // Move the home directory to the end of the places group
    QMimeData *dropData = createMimeData(QList<int>() << 0);
    m_model->dropMimeDataBefore(m_model->count() - 1, dropData);
    urls.move(0, 4);
    delete dropData;

    QTRY_COMPARE(itemsInsertedSpy.count(), 1);
    QTRY_COMPARE(itemsRemovedSpy.count(), 1);

    // remove item from actual position
    args = itemsRemovedSpy.takeFirst();
    range = args.at(0).value<KItemRangeList>();
    QCOMPARE(range.size(), 1);
    QCOMPARE(range.at(0).count, 1);
    QCOMPARE(range.at(0).index, 0);

    // insert intem in his group
    args = itemsInsertedSpy.takeFirst();
    range = args.at(0).value<KItemRangeList>();
    QCOMPARE(range.size(), 1);
    QCOMPARE(range.at(0).count, 1);
    QCOMPARE(range.at(0).index, 4);

    CHECK_PLACES_URLS(urls);

    itemsInsertedSpy.clear();
    itemsRemovedSpy.clear();

    // Move home directory item back to its original position
    dropData = createMimeData(QList<int>() << 4);
    m_model->dropMimeDataBefore(0, dropData);
    urls.move(4, 0);
    delete dropData;

    QTRY_COMPARE(itemsInsertedSpy.count(), 1);
    QTRY_COMPARE(itemsRemovedSpy.count(), 1);

    // remove item from actual position
    args = itemsRemovedSpy.takeFirst();
    range = args.at(0).value<KItemRangeList>();
    QCOMPARE(range.size(), 1);
    QCOMPARE(range.at(0).count, 1);
    QCOMPARE(range.at(0).index, 4);

    // insert intem in the requested position
    args = itemsInsertedSpy.takeFirst();
    range = args.at(0).value<KItemRangeList>();
    QCOMPARE(range.size(), 1);
    QCOMPARE(range.at(0).count, 1);
    QCOMPARE(range.at(0).index, 0);

    CHECK_PLACES_URLS(urls);
}

void PlacesItemModelTest::testHideDevices()
{
    QSignalSpy itemsRemoved(m_model, &PlacesItemModel::itemsRemoved);
    QStringList urls = initialUrls();

    m_model->setGroupHidden(KFilePlacesModel::RemovableDevicesType, true);
    QTRY_VERIFY(m_model->isGroupHidden(KFilePlacesModel::RemovableDevicesType));
    QTRY_COMPARE(itemsRemoved.count(), 3);

    // remove removable-devices
    urls.removeOne(QStringLiteral("/media/floppy0"));
    urls.removeOne(QStringLiteral("/media/XO-Y4"));
    urls.removeOne(QStringLiteral("/media/cdrom"));

    // check if the correct urls was removed
    CHECK_PLACES_URLS(urls);

    delete m_model;
    m_model = new PlacesItemModel();
    QTRY_COMPARE(m_model->count(), urls.count());
    CHECK_PLACES_URLS(urls);

    // revert changes
    m_model->setGroupHidden(KFilePlacesModel::RemovableDevicesType, false);
    urls = initialUrls();
    QTRY_COMPARE(m_model->count(), urls.count());
    CHECK_PLACES_URLS(urls);
}

void PlacesItemModelTest::testDuplicatedEntries()
{
    QStringList urls = initialUrls();
    // create a duplicated entry on bookmark
    KBookmarkManager *bookmarkManager = KBookmarkManager::managerForFile(bookmarksFile(), QStringLiteral("kfilePlaces"));
    KBookmarkGroup root = bookmarkManager->root();
    KBookmark bookmark = root.addBookmark(QStringLiteral("Duplicated Search Videos"), QUrl("search:/videos"), {});

    const QString id = QUuid::createUuid().toString();
    bookmark.setMetaDataItem(QStringLiteral("ID"), id);
    bookmark.setMetaDataItem(QStringLiteral("OnlyInApp"), KAboutData::applicationData().componentName());
    bookmarkManager->emitChanged(bookmarkManager->root());

    PlacesItemModel *newModel = new PlacesItemModel();
    QTRY_COMPARE(placesUrls(newModel).count(QStringLiteral("search:/videos")), 1);
    QTRY_COMPARE(urls, placesUrls(newModel));
    delete newModel;
}

void PlacesItemModelTest::renameAfterCreation()
{
    const QUrl tempUrl = QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::TempLocation));
    QStringList urls = initialUrls();
    PlacesItemModel *model = new PlacesItemModel();

    CHECK_PLACES_URLS(urls);
    QTRY_COMPARE(model->count(), m_model->count());

    // create a new place
    createPlaceItem(QStringLiteral("Temporary Dir"), tempUrl, QString());
    urls.insert(5, tempUrl.toLocalFile());

    // make sure that the new item will be removed later
    removePlaceAfter(5);

    CHECK_PLACES_URLS(urls);
    QCOMPARE(model->count(), m_model->count());


    // modify place text
    QSignalSpy changedSpy(m_model, &PlacesItemModel::itemsChanged);

    PlacesItem *item = m_model->placesItem(3);
    item->setText(QStringLiteral("New Temporary Dir"));
    item->setUrl(item->url());
    item->setIcon(item->icon());
    m_model->refresh();

    QTRY_COMPARE(changedSpy.count(), 1);

    // check if the place was modified in both models
    QTRY_COMPARE(m_model->placesItem(3)->text(), QStringLiteral("New Temporary Dir"));
    QTRY_COMPARE(model->placesItem(3)->text(), QStringLiteral("New Temporary Dir"));
}

QTEST_MAIN(PlacesItemModelTest)

#include "placesitemmodeltest.moc"
