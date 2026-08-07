/*
 * SPDX-FileCopyrightText: 2026 Iyán Méndez Veiga <me@iyanmv.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "views/dolphinitemlistview.h"
#include "dolphin_compactmodesettings.h"
#include "dolphin_detailsmodesettings.h"
#include "dolphin_generalsettings.h"
#include "dolphin_iconsmodesettings.h"
#include "kitemviews/kfileitemmodel.h"
#include "kitemviews/kitemlistcontroller.h"
#include "views/zoomlevelinfo.h"

#include <QStandardPaths>
#include <QTest>

Q_DECLARE_METATYPE(KStandardItemListView::ItemLayout)

class DolphinItemListViewTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void init();
    void cleanup();

    void testFreshViewFallsBackToTheConfiguredIconSize();

    void testFreshViewFallsBackToTheConfiguredPreviewSize_data();
    void testFreshViewFallsBackToTheConfiguredPreviewSize();

    void testApplyingTheCurrentZoomLevelAppliesItsIconSize_data();
    void testApplyingTheCurrentZoomLevelAppliesItsIconSize();

    void testIconAndPreviewSizesAreCachedSeparately();

    void testZoomLevelChangesAreApplied_data();
    void testZoomLevelChangesAreApplied();

    void testZoomLevelIsClamped();

private:
    /** The configured icon or preview size of @p layout, i.e. the size a view falls back to. */
    static int configuredSize(KStandardItemListView::ItemLayout layout, bool previewsShown);

    int iconSize() const
    {
        return m_view->styleOption().iconSize;
    }

    KFileItemModel *m_model = nullptr;
    DolphinItemListView *m_view = nullptr;
    KItemListController *m_controller = nullptr;
    bool m_globalViewProps = true;
};

void DolphinItemListViewTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
}

void DolphinItemListViewTest::init()
{
    m_globalViewProps = GeneralSettings::globalViewProps();
    // With global view properties the sizes are always read back from the settings, so the cached
    // m_iconSize/m_previewSize members are never used. Per-folder view properties are therefore the
    // interesting case here, and also the one in which BUG 523228 showed up.
    GeneralSettings::setGlobalViewProps(false);

    m_model = new KFileItemModel();
    m_view = new DolphinItemListView();
    // The controller attaches the model to the view, which is what makes previews toggleable:
    // without a model KFileItemListView::setPreviewsShown() is a no-op. It also takes ownership
    // of both the view and the model.
    m_controller = new KItemListController(m_model, m_view, nullptr);

    // A view starts out in details layout without previews.
    QCOMPARE(m_view->itemLayout(), KStandardItemListView::DetailsLayout);
    QVERIFY(!m_view->previewsShown());
}

void DolphinItemListViewTest::cleanup()
{
    // The controller owns both the view and the model.
    delete m_controller;
    m_controller = nullptr;
    m_view = nullptr;
    m_model = nullptr;

    GeneralSettings::setGlobalViewProps(m_globalViewProps);
}

int DolphinItemListViewTest::configuredSize(KStandardItemListView::ItemLayout layout, bool previewsShown)
{
    switch (layout) {
    case KStandardItemListView::IconsLayout:
        return previewsShown ? IconsModeSettings::previewSize() : IconsModeSettings::iconSize();
    case KStandardItemListView::CompactLayout:
        return previewsShown ? CompactModeSettings::previewSize() : CompactModeSettings::iconSize();
    case KStandardItemListView::DetailsLayout:
        return previewsShown ? DetailsModeSettings::previewSize() : DetailsModeSettings::iconSize();
    }
    Q_UNREACHABLE();
}

static void addLayoutRows()
{
    QTest::addColumn<KStandardItemListView::ItemLayout>("layout");
    QTest::newRow("icons") << KStandardItemListView::IconsLayout;
    QTest::newRow("compact") << KStandardItemListView::CompactLayout;
    QTest::newRow("details") << KStandardItemListView::DetailsLayout;
}

/**
 * A view that was just constructed must already be usable: an icon size of 0 means that no icons
 * are rendered at all. Since the cached icon size starts out as 0, the configured size has to be
 * used until a zoom level is applied.
 */
void DolphinItemListViewTest::testFreshViewFallsBackToTheConfiguredIconSize()
{
    QCOMPARE(iconSize(), configuredSize(KStandardItemListView::DetailsLayout, false));
    QVERIFY(iconSize() > 0);
}

void DolphinItemListViewTest::testFreshViewFallsBackToTheConfiguredPreviewSize_data()
{
    addLayoutRows();
}

/** The same for the separately cached preview size, which is unset until previews are turned on. */
void DolphinItemListViewTest::testFreshViewFallsBackToTheConfiguredPreviewSize()
{
    QFETCH(KStandardItemListView::ItemLayout, layout);

    m_view->setItemLayout(layout);
    m_view->setPreviewsShown(true);
    QVERIFY(m_view->previewsShown());

    QCOMPARE(iconSize(), configuredSize(layout, true));
    QVERIFY(iconSize() > 0);
}

void DolphinItemListViewTest::testApplyingTheCurrentZoomLevelAppliesItsIconSize_data()
{
    addLayoutRows();
}

/**
 * Regression test for BUG 523228: when a folder is opened, DolphinView applies the zoom level from
 * its view properties. That zoom level often is the one the view already reports, in which case
 * setZoomLevel() used to return early and left the cached size at 0, so the view rendered null
 * pixmaps for every item until the view mode was changed.
 */
void DolphinItemListViewTest::testApplyingTheCurrentZoomLevelAppliesItsIconSize()
{
    QFETCH(KStandardItemListView::ItemLayout, layout);

    m_view->setItemLayout(layout);

    // Applying the zoom level the view already is at must not be a no-op.
    const int level = m_view->zoomLevel();
    m_view->setZoomLevel(level);

    QCOMPARE(m_view->zoomLevel(), level);
    QCOMPARE(iconSize(), ZoomLevelInfo::iconSizeForZoomLevel(level));
    QVERIFY(iconSize() > 0);
}

/**
 * Icon size and preview size are cached separately, so turning previews on after only the icon size
 * has been populated must not leave the view with the still unset preview size of 0.
 */
void DolphinItemListViewTest::testIconAndPreviewSizesAreCachedSeparately()
{
    const int zoomLevel = 2;
    m_view->setZoomLevel(zoomLevel);
    QCOMPARE(iconSize(), ZoomLevelInfo::iconSizeForZoomLevel(zoomLevel));

    // Only the icon size has been set so far, so the preview size falls back to the configured one.
    m_view->setPreviewsShown(true);
    QCOMPARE(iconSize(), configuredSize(KStandardItemListView::DetailsLayout, true));

    // Once a zoom level is applied while previews are shown, that one wins again.
    m_view->setZoomLevel(zoomLevel);
    QCOMPARE(iconSize(), ZoomLevelInfo::iconSizeForZoomLevel(zoomLevel));

    // Switching previews back off restores the cached icon size.
    m_view->setPreviewsShown(false);
    QCOMPARE(iconSize(), ZoomLevelInfo::iconSizeForZoomLevel(zoomLevel));
}

void DolphinItemListViewTest::testZoomLevelChangesAreApplied_data()
{
    addLayoutRows();
}

/** Makes sure that dropping the early return in setZoomLevel() did not break ordinary zooming. */
void DolphinItemListViewTest::testZoomLevelChangesAreApplied()
{
    QFETCH(KStandardItemListView::ItemLayout, layout);

    m_view->setItemLayout(layout);

    for (int level = ZoomLevelInfo::minimumLevel(); level <= ZoomLevelInfo::maximumLevel(); ++level) {
        m_view->setZoomLevel(level);
        QCOMPARE(m_view->zoomLevel(), level);
        QCOMPARE(iconSize(), ZoomLevelInfo::iconSizeForZoomLevel(level));
    }
}

void DolphinItemListViewTest::testZoomLevelIsClamped()
{
    m_view->setZoomLevel(ZoomLevelInfo::minimumLevel() - 1);
    QCOMPARE(m_view->zoomLevel(), ZoomLevelInfo::minimumLevel());
    QCOMPARE(iconSize(), ZoomLevelInfo::iconSizeForZoomLevel(ZoomLevelInfo::minimumLevel()));

    m_view->setZoomLevel(ZoomLevelInfo::maximumLevel() + 1);
    QCOMPARE(m_view->zoomLevel(), ZoomLevelInfo::maximumLevel());
    QCOMPARE(iconSize(), ZoomLevelInfo::iconSizeForZoomLevel(ZoomLevelInfo::maximumLevel()));
}

QTEST_MAIN(DolphinItemListViewTest)

#include "dolphinitemlistviewtest.moc"
