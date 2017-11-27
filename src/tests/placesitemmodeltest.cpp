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
#include <QDebug>
#include <QList>
#include <QByteArray>
#include <QStandardPaths>
#include <QAction>
#include <QDBusInterface>

#include <KBookmarkManager>
#include <KConfig>
#include <KConfigGroup>
#include <KAboutData>

#include "panels/places/placesitemmodel.h"
#include "panels/places/placesitem.h"
#include "views/viewproperties.h"
#include "kitemviews/kitemrange.h"

Q_DECLARE_METATYPE(KItemRangeList)
Q_DECLARE_METATYPE(PlacesItem::GroupType)

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
    void testModelMove();
    void testGroups();
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

private:
    PlacesItemModel* m_model;
    QMap<QString, QDBusInterface *> m_interfacesMap;

    void setBalooEnabled(bool enabled);
    int indexOf(const QUrl &url);
    QDBusInterface *fakeManager();
    QDBusInterface *fakeDevice(const QString &udi);
    QStringList placesUrls() const;
    QStringList initialUrls() const;
    void createPlaceItem(const QString &text, const QUrl &url, const QString &icon);
};

#define CHECK_PLACES_URLS(urls)                                              \
    QStringList tmp(urls);                                                   \
    QStringList places = placesUrls();                                       \
    while(!places.isEmpty()) {                                               \
        tmp.removeOne(places.takeFirst());                                   \
    }                                                                        \
    if (!tmp.isEmpty()) {                                                    \
        qWarning() << "Expected:" << urls;                                   \
        qWarning() << "Got:" << places;                                      \
        QCOMPARE(places, urls);                                              \
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

QStringList PlacesItemModelTest::placesUrls() const
{
    QStringList urls;
    for (int row = 0; row < m_model->count(); ++row) {
        urls << m_model->placesItem(row)->url().toDisplayString(QUrl::PreferLocalFile);
    }
    return urls;
}

void PlacesItemModelTest::createPlaceItem(const QString &text, const QUrl &url, const QString &icon)
{
    PlacesItem *item = m_model->createPlacesItem(text,
                                                 url,
                                                 icon);
    QSignalSpy itemsInsertedSpy(m_model, &PlacesItemModel::itemsInserted);
    m_model->appendItemToGroup(item);
    QTRY_COMPARE(itemsInsertedSpy.count(), 1);
}

void PlacesItemModelTest::init()
{
    m_model = new PlacesItemModel();
    // WORKAROUND: need to wait for bookmark to load, check: PlacesItemModel::updateBookmarks
    QTest::qWait(300);
    QCOMPARE(m_model->count(), 17);
}

void PlacesItemModelTest::cleanup()
{
    delete m_model;
    m_model = nullptr;
}

void PlacesItemModelTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);

    const QString fakeHw = QFINDTESTDATA("data/fakecomputer.xml");
    QVERIFY(!fakeHw.isEmpty());
    qputenv("SOLID_FAKEHW", QFile::encodeName(fakeHw));

    setBalooEnabled(true);
    const QString bookmarsFileName = bookmarksFile();
    if (QFileInfo::exists(bookmarsFileName)) {
        // Ensure we'll have a clean bookmark file to start
        QVERIFY(QFile::remove(bookmarsFileName));
    }

    qRegisterMetaType<KItemRangeList>();
}

void PlacesItemModelTest::cleanupTestCase()
{
    qDeleteAll(m_interfacesMap);
    QFile::remove(bookmarksFile());
}

QStringList PlacesItemModelTest::initialUrls() const
{
    QStringList urls;

    urls << QDir::homePath() << QStringLiteral("remote:/") << QStringLiteral(KDE_ROOT_PATH) << QStringLiteral("trash:/")
            << QStringLiteral("timeline:/today") << QStringLiteral("timeline:/yesterday") << QStringLiteral("timeline:/thismonth") << QStringLiteral("timeline:/lastmonth")
            << QStringLiteral("search:/documents") << QStringLiteral("search:/images") << QStringLiteral("search:/audio") << QStringLiteral("search:/videos")
            << QStringLiteral("/media/cdrom") << QStringLiteral("/foreign") << QStringLiteral("/media/XO-Y4") << QStringLiteral("/media/nfs") << QStringLiteral("/media/floppy0");

    return urls;
}

void PlacesItemModelTest::testModelSort()
{
    CHECK_PLACES_URLS(initialUrls());
}

void PlacesItemModelTest::testModelMove()
{
    QStringList urls = initialUrls();
    KBookmarkManager *bookmarkManager = KBookmarkManager::managerForFile(bookmarksFile(), QStringLiteral("kfilePlaces"));
    KBookmarkGroup root = bookmarkManager->root();
    KBookmark systemRoot = m_model->placesItem(1)->bookmark();
    KBookmark last = m_model->placesItem(m_model->count() - 1)->bookmark();

    // try to move the "root" path to the end of the list
    root.moveBookmark(systemRoot, last);
    bookmarkManager->emitChanged(root);

    // make sure that the items still grouped and the "root" item was moved to the end of places group instead
    urls.move(1, 2);
    CHECK_PLACES_URLS(urls);
}

void PlacesItemModelTest::testGroups()
{
    const auto groups = m_model->groups();

    QCOMPARE(groups.size(), 4);
    QCOMPARE(groups.at(0).first, 0);
    QCOMPARE(groups.at(0).second.toString(), QStringLiteral("Places"));
    QCOMPARE(groups.at(1).first, 4);
    QCOMPARE(groups.at(1).second.toString(), QStringLiteral("Recently Saved"));
    QCOMPARE(groups.at(2).first, 8);
    QCOMPARE(groups.at(2).second.toString(), QStringLiteral("Search For"));
    QCOMPARE(groups.at(3).first, 12);
    QCOMPARE(groups.at(3).second.toString(), QStringLiteral("Devices"));
}

void PlacesItemModelTest::testPlaceItem_data()
{
    QTest::addColumn<QUrl>("url");
    QTest::addColumn<bool>("expectedIsHidden");
    QTest::addColumn<bool>("expectedIsSystemItem");
    QTest::addColumn<PlacesItem::GroupType>("expectedGroupType");
    QTest::addColumn<bool>("expectedStorageSetupNeeded");

    // places
    QTest::newRow("Places - Home") << QUrl::fromLocalFile(QDir::homePath()) << false << true << PlacesItem::PlacesType << false;

    // baloo -search
    QTest::newRow("Baloo - Documents") << QUrl("search:/documents") << false << true << PlacesItem::SearchForType << false;

    // baloo - timeline
    QTest::newRow("Baloo - Last Month") << QUrl("timeline:/lastmonth") << false << true << PlacesItem::RecentlySavedType << false;

    // devices
    QTest::newRow("Devices - Floppy") << QUrl("file:///media/floppy0") << false << false << PlacesItem::DevicesType << false;
}

void PlacesItemModelTest::testPlaceItem()
{
    QFETCH(QUrl, url);
    QFETCH(bool, expectedIsHidden);
    QFETCH(bool, expectedIsSystemItem);
    QFETCH(PlacesItem::GroupType, expectedGroupType);
    QFETCH(bool, expectedStorageSetupNeeded);

    const int index = indexOf(url);
    PlacesItem *item = m_model->placesItem(index);
    QCOMPARE(item->url(), url);
    QCOMPARE(item->isHidden(), expectedIsHidden);
    QCOMPARE(item->isSystemItem(), expectedIsSystemItem);
    QCOMPARE(item->groupType(), expectedGroupType);
    QCOMPARE(item->storageSetupNeeded(), expectedStorageSetupNeeded);
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

    QCOMPARE(m_model->count(), 17);

    QSignalSpy spyItemsRemoved(m_model, &PlacesItemModel::itemsRemoved);
    fakeManager()->call(QStringLiteral("unplug"), "/org/kde/solid/fakehw/volume_part1_size_993284096");
    QTRY_COMPARE(m_model->count(), 16);
    QCOMPARE(spyItemsRemoved.count(), 1);
    const QList<QVariant> spyItemsRemovedArgs = spyItemsRemoved.takeFirst();
    const KItemRangeList removedRange = spyItemsRemovedArgs.at(0).value<KItemRangeList>();
    QCOMPARE(removedRange.size(), 1);
    QCOMPARE(removedRange.first().index, index);
    QCOMPARE(removedRange.first().count, 1);

    QCOMPARE(indexOf(mediaUrl), -1);

    QSignalSpy spyItemsInserted(m_model, &PlacesItemModel::itemsInserted);
    fakeManager()->call(QStringLiteral("plug"), "/org/kde/solid/fakehw/volume_part1_size_993284096");
    QTRY_COMPARE(m_model->count(), 17);
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
    QTest::newRow("Baloo - Last Month") << QUrl("timeline:/lastmonth") << DolphinView::DetailsView << true << QList<QByteArray>({"text", "modificationtime"});

    // devices
    QTest::newRow("Devices - Floppy") << QUrl("file:///media/floppy0") << DolphinView::IconsView << true << QList<QByteArray>({"text"});

}

void PlacesItemModelTest::testDefaultViewProperties()
{
    QFETCH(QUrl, url);
    QFETCH(DolphinView::Mode, expectedViewMode);
    QFETCH(bool, expectedPreviewShow);
    QFETCH(QList<QByteArray>, expectedVisibleRole);

    ViewProperties properties(m_model->convertedUrl(url));
    QCOMPARE(properties.viewMode(), expectedViewMode);
    QCOMPARE(properties.previewsShown(), expectedPreviewShow);
    QCOMPARE(properties.visibleRoles(), expectedVisibleRole);
}

void PlacesItemModelTest::testClear()
{
    QCOMPARE(m_model->count(), 17);
    m_model->clear();
    QCOMPARE(m_model->count(), 0);
    QCOMPARE(m_model->hiddenCount(), 0);
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
    QTRY_COMPARE(m_model->count(), 16);
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

    QTRY_COMPARE(m_model->count(), 17);
}

void PlacesItemModelTest::testSystemItems()
{
    QCOMPARE(m_model->count(), 17);
    for (int r = 0; r < m_model->count(); r++) {
        QCOMPARE(m_model->placesItem(r)->isSystemItem(), !m_model->placesItem(r)->device().isValid());
    }

    // create a new entry (non system item)
    PlacesItem *item = m_model->createPlacesItem(QStringLiteral("Temporary Dir"),
                                                 QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::TempLocation)),
                                                 QString());

    QSignalSpy itemsInsertedSpy(m_model, &PlacesItemModel::itemsInserted);
    m_model->appendItemToGroup(item);

    // check if the new entry was created
    QTRY_COMPARE(itemsInsertedSpy.count(), 1);
    QList<QVariant> args = itemsInsertedSpy.takeFirst();
    KItemRangeList range = args.at(0).value<KItemRangeList>();
    QCOMPARE(range.first().index, 4);
    QCOMPARE(range.first().count, 1);
    QVERIFY(!m_model->placesItem(4)->isSystemItem());
    QCOMPARE(m_model->count(), 18);

    // remove new entry
    QSignalSpy itemsRemovedSpy(m_model, &PlacesItemModel::itemsRemoved);
    m_model->removeItem(4);
    m_model->saveBookmarks();
    QTRY_COMPARE(itemsRemovedSpy.count(), 1);
    args = itemsRemovedSpy.takeFirst();
    range = args.at(0).value<KItemRangeList>();
    QCOMPARE(range.first().index, 4);
    QCOMPARE(range.first().count, 1);
    QTRY_COMPARE(m_model->count(), 17);
}

void PlacesItemModelTest::testEditBookmark()
{
    QScopedPointer<PlacesItemModel> other(new PlacesItemModel());

    createPlaceItem(QStringLiteral("Temporary Dir"), QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::TempLocation)), QString());

    QSignalSpy itemsChangedSply(m_model, &PlacesItemModel::itemsChanged);
    m_model->item(4)->setText(QStringLiteral("Renamed place"));
    m_model->saveBookmarks();
    QTRY_COMPARE(itemsChangedSply.count(), 1);
    QList<QVariant> args = itemsChangedSply.takeFirst();
    KItemRangeList range = args.at(0).value<KItemRangeList>();
    QCOMPARE(range.first().index, 4);
    QCOMPARE(range.first().count, 1);
    QSet<QByteArray> roles = args.at(1).value<QSet<QByteArray> >();
    QCOMPARE(roles.size(), 1);
    QCOMPARE(*roles.begin(), QByteArrayLiteral("text"));
    QCOMPARE(m_model->item(4)->text(), QStringLiteral("Renamed place"));

    // check if the item was updated in the other model
    QTRY_COMPARE(other->item(4)->text(), QStringLiteral("Renamed place"));

    // remove new entry
    QSignalSpy itemsRemovedSpy(m_model, &PlacesItemModel::itemsRemoved);
    m_model->removeItem(4);
    m_model->saveBookmarks();
    QTRY_COMPARE(itemsRemovedSpy.count(), 1);
    args = itemsRemovedSpy.takeFirst();
    range = args.at(0).value<KItemRangeList>();
    QCOMPARE(range.first().index, 4);
    QCOMPARE(range.first().count, 1);
    QTRY_COMPARE(m_model->count(), 17);
}

void PlacesItemModelTest::testEditAfterCreation()
{
    createPlaceItem(QStringLiteral("Temporary Dir"), QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::TempLocation)), QString());

    PlacesItemModel *model = new PlacesItemModel();
    QTRY_COMPARE(model->count(), m_model->count());

    PlacesItem *item = m_model->placesItem(4);
    item->setText(QStringLiteral("Renamed place"));
    m_model->saveBookmarks();

    QTRY_COMPARE(model->count(), m_model->count());
    QTRY_COMPARE(model->placesItem(4)->text(), m_model->placesItem(4)->text());
    QTRY_COMPARE(model->placesItem(4)->bookmark().metaDataItem(QStringLiteral("OnlyInApp")),
                 m_model->placesItem(4)->bookmark().metaDataItem(QStringLiteral("OnlyInApp")));
    QTRY_COMPARE(model->placesItem(4)->icon(), m_model->placesItem(4)->icon());
    QTRY_COMPARE(model->placesItem(4)->url(), m_model->placesItem(4)->url());

    m_model->removeItem(4);
    m_model->saveBookmarks();
    QTRY_COMPARE(model->count(), m_model->count());
}

void PlacesItemModelTest::testEditMetadata()
{
    createPlaceItem(QStringLiteral("Temporary Dir"), QUrl::fromLocalFile(QStandardPaths::writableLocation(QStandardPaths::TempLocation)), QString());

    PlacesItemModel *model = new PlacesItemModel();
    QTRY_COMPARE(model->count(), m_model->count());

    PlacesItem *item = m_model->placesItem(4);
    item->bookmark().setMetaDataItem(QStringLiteral("OnlyInApp"), KAboutData::applicationData().componentName());
    m_model->saveBookmarks();

    QTRY_COMPARE(model->count(), m_model->count());
    QTRY_COMPARE(model->placesItem(4)->bookmark().metaDataItem(QStringLiteral("OnlyInApp")),
                 KAboutData::applicationData().componentName());
    QTRY_COMPARE(model->placesItem(4)->text(), m_model->placesItem(4)->text());
    QTRY_COMPARE(model->placesItem(4)->bookmark().metaDataItem(QStringLiteral("OnlyInApp")),
                 m_model->placesItem(4)->bookmark().metaDataItem(QStringLiteral("OnlyInApp")));
    QTRY_COMPARE(model->placesItem(4)->icon(), m_model->placesItem(4)->icon());
    QTRY_COMPARE(model->placesItem(4)->url(), m_model->placesItem(4)->url());

    m_model->removeItem(4);
    m_model->saveBookmarks();
    QTRY_COMPARE(model->count(), m_model->count());
}

QTEST_MAIN(PlacesItemModelTest)

#include "placesitemmodeltest.moc"
