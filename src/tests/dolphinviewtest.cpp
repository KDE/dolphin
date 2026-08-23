/*
 * SPDX-FileCopyrightText: 2026 Meven Car <meven@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "views/dolphinview.h"
#include "dolphin_compactmodesettings.h"
#include "dolphin_detailsmodesettings.h"
#include "dolphin_generalsettings.h"
#include "dolphin_iconsmodesettings.h"
#include "kitemviews/kitemlistcontainer.h"
#include "testdir.h"
#include "views/viewproperties.h"
#include "views/zoomlevelinfo.h"

#include <KIconLoader>

#include <QApplication>
#include <QContextMenuEvent>
#include <QDir>
#include <QScopeGuard>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTest>

class DolphinViewTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void init();
    void cleanup();

    void selectionIsAnnouncedBeforeTheBackgroundContextMenu();
    void defaultZoomLevelComesFromThePreviewSizeWhenPreviewsAreShown();
    void switchingViewModeAdoptsTheNewModesConfiguredSize_data();
    void switchingViewModeAdoptsTheNewModesConfiguredSize();
    void aFreshViewUsesTheConfiguredSizeOfItsOwnViewMode_data();
    void aFreshViewUsesTheConfiguredSizeOfItsOwnViewMode();

private:
    void requestBackgroundContextMenu();
    static void writeViewProperties(const QUrl &folder, DolphinView::Mode mode, bool previewsShown);

    TestDir *m_testDir = nullptr;
    DolphinView *m_view = nullptr;
};

void DolphinViewTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
}

void DolphinViewTest::init()
{
    m_testDir = new TestDir();
    m_testDir->createFile(QStringLiteral("a.txt"));
    m_testDir->createFile(QStringLiteral("b.txt"));

    m_view = new DolphinView(m_testDir->url(), nullptr);
    m_view->resize(400, 400);
    m_view->show();
    QVERIFY(QTest::qWaitForWindowExposed(m_view));
    QTRY_COMPARE(m_view->itemsCount(), 2);
}

void DolphinViewTest::cleanup()
{
    delete m_view;
    m_view = nullptr;
    delete m_testDir;
    m_testDir = nullptr;
}

/**
 * A context menu asked for on the background of the view comes with the selection emptied by the
 * same click, and whoever fills that menu learns about the empty selection before the menu itself
 * is asked for. See Bug 522372.
 */
void DolphinViewTest::selectionIsAnnouncedBeforeTheBackgroundContextMenu()
{
    QSignalSpy selectionChanged(m_view, &DolphinView::selectionChanged);
    m_view->selectAll();
    QTRY_COMPARE(selectionChanged.count(), 1);
    QCOMPARE(selectionChanged.first().first().value<KFileItemList>().count(), 2);

    QStringList signalsInOrder;
    connect(m_view, &DolphinView::selectionChanged, this, [&signalsInOrder] {
        signalsInOrder << QStringLiteral("selectionChanged");
    });
    connect(m_view, &DolphinView::requestContextMenu, this, [&signalsInOrder] {
        signalsInOrder << QStringLiteral("requestContextMenu");
    });

    m_view->clearSelection();
    requestBackgroundContextMenu();

    QCOMPARE(signalsInOrder, QStringList({QStringLiteral("selectionChanged"), QStringLiteral("requestContextMenu")}));
    QCOMPARE(selectionChanged.last().first().value<KFileItemList>().count(), 0);
}

/**
 * A folder that carries no zoom level of its own falls back to the default one, and while previews
 * are shown that default has to come from the configured preview size rather than the icon size.
 * Otherwise the preview size never reaches such a folder and its slider looks like it does nothing.
 * See Bug 524532.
 */
void DolphinViewTest::defaultZoomLevelComesFromThePreviewSizeWhenPreviewsAreShown()
{
    const bool globalViewProps = GeneralSettings::globalViewProps();
    const int iconSize = IconsModeSettings::iconSize();
    const int previewSize = IconsModeSettings::previewSize();
    auto restoreSettings = qScopeGuard([globalViewProps, iconSize, previewSize] {
        GeneralSettings::setGlobalViewProps(globalViewProps);
        IconsModeSettings::setIconSize(iconSize);
        IconsModeSettings::setPreviewSize(previewSize);
    });

    // Only per-folder view properties have a folder without a zoom level to fall back for.
    GeneralSettings::setGlobalViewProps(false);
    IconsModeSettings::setIconSize(KIconLoader::SizeMedium);
    IconsModeSettings::setPreviewSize(KIconLoader::SizeEnormous);

    const int iconZoomLevel = ZoomLevelInfo::zoomLevelForIconSize(QSize(KIconLoader::SizeMedium, KIconLoader::SizeMedium));
    const int previewZoomLevel = ZoomLevelInfo::zoomLevelForIconSize(QSize(KIconLoader::SizeEnormous, KIconLoader::SizeEnormous));
    QVERIFY(iconZoomLevel != previewZoomLevel);

    // A folder of its own, so that nothing has ever stored a zoom level for it.
    m_testDir->createDir(QStringLiteral("fresh"));
    const QUrl freshFolder = QUrl::fromLocalFile(QDir(m_testDir->path()).filePath(QStringLiteral("fresh")));
    writeViewProperties(freshFolder, DolphinView::IconsView, true);
    QCOMPARE(ViewProperties(freshFolder).zoomLevel(), -1);

    {
        DolphinView view(freshFolder, nullptr);
        QVERIFY(view.previewsShown());
        QCOMPARE(view.zoomLevel(), previewZoomLevel);
    }

    // The same folder with previews turned off has to fall back to the icon size instead.
    {
        ViewProperties props(freshFolder);
        props.setPreviewsShown(false);
        props.save();
        QCOMPARE(props.zoomLevel(), -1);
    }

    DolphinView view(freshFolder, nullptr);
    QVERIFY(!view.previewsShown());
    QCOMPARE(view.zoomLevel(), iconZoomLevel);
}

void DolphinViewTest::switchingViewModeAdoptsTheNewModesConfiguredSize_data()
{
    QTest::addColumn<bool>("previews");
    QTest::newRow("previews") << true;
    QTest::newRow("no previews") << false;
}

/**
 * Every view mode carries its own icon and preview size, so a folder that stores no zoom level of
 * its own has to take the size of whichever mode it is shown in, and switching modes has to move to
 * the new mode's size. Per-folder view properties are the interesting case: there the size a view
 * renders at is cached on the view rather than read back from the settings, and that cache is not
 * per mode. See Bug 524532.
 */
void DolphinViewTest::switchingViewModeAdoptsTheNewModesConfiguredSize()
{
    QFETCH(bool, previews);

    const bool globalBefore = GeneralSettings::globalViewProps();
    const int iconsIconBefore = IconsModeSettings::iconSize();
    const int iconsPreviewBefore = IconsModeSettings::previewSize();
    const int compactIconBefore = CompactModeSettings::iconSize();
    const int compactPreviewBefore = CompactModeSettings::previewSize();
    auto restoreSettings = qScopeGuard([=] {
        GeneralSettings::setGlobalViewProps(globalBefore);
        IconsModeSettings::setIconSize(iconsIconBefore);
        IconsModeSettings::setPreviewSize(iconsPreviewBefore);
        CompactModeSettings::setIconSize(compactIconBefore);
        CompactModeSettings::setPreviewSize(compactPreviewBefore);
    });

    // Per-folder view properties are the only case that caches the size on the view rather than
    // reading it back from the settings, and writing to them stays inside the test folder.
    GeneralSettings::setGlobalViewProps(false);
    // Four distinct sizes, so the mode and the preview state a size came from are both visible in
    // the resulting zoom level.
    IconsModeSettings::setIconSize(KIconLoader::SizeMedium);
    IconsModeSettings::setPreviewSize(KIconLoader::SizeEnormous);
    CompactModeSettings::setIconSize(KIconLoader::SizeSmall);
    CompactModeSettings::setPreviewSize(KIconLoader::SizeLarge);

    const auto levelFor = [](int size) {
        return ZoomLevelInfo::zoomLevelForIconSize(QSize(size, size));
    };
    const int iconsLevel = previews ? levelFor(KIconLoader::SizeEnormous) : levelFor(KIconLoader::SizeMedium);
    const int compactLevel = previews ? levelFor(KIconLoader::SizeLarge) : levelFor(KIconLoader::SizeSmall);
    QVERIFY(iconsLevel != compactLevel);

    // A folder of its own, so that nothing has ever stored a zoom level for it.
    m_testDir->createDir(QStringLiteral("fresh"));
    m_testDir->createFile(QStringLiteral("fresh/one.txt"));
    m_testDir->createFile(QStringLiteral("fresh/two.txt"));
    const QUrl folder = QUrl::fromLocalFile(QDir(m_testDir->path()).filePath(QStringLiteral("fresh")));
    // Details is a mode no assertion below is about, so every switch that follows is a real change.
    writeViewProperties(folder, DolphinView::DetailsView, previews);
    QCOMPARE(ViewProperties(folder).zoomLevel(), -1);

    delete m_view;
    m_view = new DolphinView(folder, nullptr);
    m_view->resize(400, 400);
    m_view->show();
    QVERIFY(QTest::qWaitForWindowExposed(m_view));
    QTRY_COMPARE(m_view->itemsCount(), 2);

    QCOMPARE(m_view->previewsShown(), previews);

    // Every step below is an actual mode change, which is what has to land on the new mode's size.
    m_view->setViewMode(DolphinView::CompactView);
    QCOMPARE(m_view->zoomLevel(), compactLevel);

    m_view->setViewMode(DolphinView::IconsView);
    QCOMPARE(m_view->zoomLevel(), iconsLevel);

    m_view->setViewMode(DolphinView::CompactView);
    QCOMPARE(m_view->zoomLevel(), compactLevel);

    // The folder still stores no zoom level, so this exercised the fallback throughout.
    QCOMPARE(ViewProperties(folder).zoomLevel(), -1);
}

/**
 * Stores the view properties a test needs, so that it depends on nothing inherited. A folder with no
 * properties of its own takes the global ones as its defaults, and those persist between runs in the
 * QStandardPaths test location, so trusting them makes a test depend on what ran before it.
 */
void DolphinViewTest::aFreshViewUsesTheConfiguredSizeOfItsOwnViewMode_data()
{
    QTest::addColumn<bool>("previews");
    QTest::newRow("previews") << true;
    QTest::newRow("no previews") << false;
}

/**
 * A folder with no zoom level of its own has to be shown at the configured size of the view mode it
 * opens in. The per-folder zoom is one value for the folder and so is deliberately shared by every
 * view mode, but a view passes through the details layout on its way to the mode it will actually
 * show, and the size that layout falls back to must not be mistaken for a zoom the folder carries.
 * See Bug 524532.
 */
void DolphinViewTest::aFreshViewUsesTheConfiguredSizeOfItsOwnViewMode()
{
    QFETCH(bool, previews);

    const bool globalBefore = GeneralSettings::globalViewProps();
    const int iconsIconBefore = IconsModeSettings::iconSize();
    const int iconsPreviewBefore = IconsModeSettings::previewSize();
    const int detailsIconBefore = DetailsModeSettings::iconSize();
    const int detailsPreviewBefore = DetailsModeSettings::previewSize();
    auto restoreSettings = qScopeGuard([=] {
        GeneralSettings::setGlobalViewProps(globalBefore);
        IconsModeSettings::setIconSize(iconsIconBefore);
        IconsModeSettings::setPreviewSize(iconsPreviewBefore);
        DetailsModeSettings::setIconSize(detailsIconBefore);
        DetailsModeSettings::setPreviewSize(detailsPreviewBefore);
    });

    // Only per-folder view properties read the size back from the view rather than the settings.
    GeneralSettings::setGlobalViewProps(false);
    IconsModeSettings::setIconSize(KIconLoader::SizeMedium);
    IconsModeSettings::setPreviewSize(KIconLoader::SizeEnormous);
    DetailsModeSettings::setIconSize(KIconLoader::SizeSmall);
    DetailsModeSettings::setPreviewSize(KIconLoader::SizeLarge);

    const auto levelFor = [](int size) {
        return ZoomLevelInfo::zoomLevelForIconSize(QSize(size, size));
    };
    const int iconsLevel = previews ? levelFor(KIconLoader::SizeEnormous) : levelFor(KIconLoader::SizeMedium);
    const int detailsLevel = previews ? levelFor(KIconLoader::SizeLarge) : levelFor(KIconLoader::SizeSmall);
    QVERIFY(iconsLevel != detailsLevel);

    m_testDir->createDir(QStringLiteral("brandnew"));
    m_testDir->createFile(QStringLiteral("brandnew/one.txt"));
    m_testDir->createFile(QStringLiteral("brandnew/two.txt"));
    const QUrl folder = QUrl::fromLocalFile(QDir(m_testDir->path()).filePath(QStringLiteral("brandnew")));
    // Opened with the opposite preview state, so that turning previews to the state under test is
    // what selects the cached size, with no mode change to recompute the default along the way.
    writeViewProperties(folder, DolphinView::IconsView, !previews);
    QCOMPARE(ViewProperties(folder).zoomLevel(), -1);

    delete m_view;
    m_view = new DolphinView(folder, nullptr);
    m_view->resize(600, 400);
    m_view->show();
    QVERIFY(QTest::qWaitForWindowExposed(m_view));
    QTRY_COMPARE(m_view->itemsCount(), 2);

    m_view->setPreviewsShown(previews);
    QCOMPARE(m_view->previewsShown(), previews);
    QCOMPARE(m_view->zoomLevel(), iconsLevel);
    // Nothing was stored along the way, so this really was the fallback.
    QCOMPARE(ViewProperties(folder).zoomLevel(), -1);
}

void DolphinViewTest::writeViewProperties(const QUrl &folder, DolphinView::Mode mode, bool previewsShown)
{
    ViewProperties props(folder);
    props.setViewMode(mode);
    props.setPreviewsShown(previewsShown);
    // No zoom level of its own, which is the fallback the tests below are about.
    props.setZoomLevel(-1);
    props.save();
}

void DolphinViewTest::requestBackgroundContextMenu()
{
    KItemListContainer *container = m_view->findChild<KItemListContainer *>();
    QVERIFY(container);

    QContextMenuEvent event(QContextMenuEvent::Keyboard, QPoint(), QPoint());
    QApplication::sendEvent(container, &event);
}

QTEST_MAIN(DolphinViewTest)

#include "dolphinviewtest.moc"
