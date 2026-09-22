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
#include "kitemviews/kitemlistcontainer.h"
#include "kitemviews/kitemlistcontroller.h"
#include "kitemviews/kitemlistselectionmanager.h"
#include "testdir.h"
#include "views/zoomlevelinfo.h"

#include <QGuiApplication>
#include <QScopeGuard>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTest>

#include <memory>

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

    void testTheFilesOnScreenStayOnScreenWhenTheViewGetsNarrower();
    void testTheViewComesBackToWhereItWasWhenTheWidthDoes();
    void testTheLastRowStaysClearOfTheStatusBarWhenTheViewGetsWider();

    void testTheViewStaysAtTheStartOrTheEndWhenResized_data();
    void testTheViewStaysAtTheStartOrTheEndWhenResized();

    void testASelectedItemOnScreenStaysOnScreenWhenResized_data();
    void testASelectedItemOnScreenStaysOnScreenWhenResized();

    void testASelectionOffScreenOrACurrentItemAloneDoesNotMoveTheView_data();
    void testASelectionOffScreenOrACurrentItemAloneDoesNotMoveTheView();

    void testTheViewKeepsUpWithChangesBetweenResizes_data();
    void testTheViewKeepsUpWithChangesBetweenResizes();

private:
    /** The configured icon or preview size of @p layout, i.e. the size a view falls back to. */
    static int configuredSize(KStandardItemListView::ItemLayout layout, bool previewsShown);

    /** Start and end of item @p index along the scroll axis, from the side the list starts at. */
    std::pair<qreal, qreal> itemSpan(int index, KStandardItemListView::ItemLayout layout) const;

    /** @return Whether the view could be shown at @p size with 500 files loaded. */
    bool showViewWithFiles(KStandardItemListView::ItemLayout layout, const QSize &size);

    /** @return Whether the container got resized to @p size. */
    bool resizeContainer(const QSize &size);

    int iconSize() const
    {
        return m_view->styleOption().iconSize;
    }

    KFileItemModel *m_model = nullptr;
    DolphinItemListView *m_view = nullptr;
    KItemListController *m_controller = nullptr;
    std::unique_ptr<KItemListContainer> m_container;
    std::unique_ptr<TestDir> m_testDir;
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
    // The controller owns the view and the model, the container owns the controller.
    m_container.reset();
    delete m_controller;
    m_controller = nullptr;
    m_view = nullptr;
    m_model = nullptr;
    m_testDir.reset();

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

bool DolphinItemListViewTest::showViewWithFiles(KStandardItemListView::ItemLayout layout, const QSize &size)
{
    // The container provides the scene and takes over the controller.
    m_container = std::make_unique<KItemListContainer>(m_controller);
    m_controller = nullptr;
#ifndef QT_NO_ACCESSIBILITY
    m_view->setAccessibleParentsObject(m_container.get());
#endif
    m_view->setItemLayout(layout);
    m_container->resize(size);
    m_container->show();
    if (!QTest::qWaitForWindowExposed(m_container.get())) {
        return false;
    }

    m_testDir = std::make_unique<TestDir>();
    QStringList files;
    for (int i = 0; i < 500; ++i) {
        files.append(QStringLiteral("file_%1").arg(i, 3, 10, QLatin1Char('0')));
    }
    m_testDir->createFiles(files);

    QSignalSpy itemsInsertedSpy(m_model, &KFileItemModel::itemsInserted);
    m_model->loadDirectory(m_testDir->url());
    return itemsInsertedSpy.wait() && m_model->count() == files.count();
}

bool DolphinItemListViewTest::resizeContainer(const QSize &size)
{
    m_container->resize(size);
    return QTest::qWaitFor([this, size]() {
        return m_container->size() == size;
    });
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

/**
 * Opening the split view halves the width of the view the user was looking at, and in the icons
 * layout that reflows the items into fewer columns. The files that were on screen have to still be
 * on screen afterwards. See bug 524143.
 */
void DolphinItemListViewTest::testTheFilesOnScreenStayOnScreenWhenTheViewGetsNarrower()
{
    QVERIFY(showViewWithFiles(KStandardItemListView::IconsLayout, QSize(800, 600)));

    // Somewhere in the middle of the folder, so that there is content above and below.
    m_view->setScrollOffset(m_view->maximumScrollOffset() / 2);
    const int topItem = m_view->firstVisibleIndex();
    QVERIFY(topItem > 0);
    QVERIFY(m_view->lastVisibleIndex() < m_model->count() - 1);

    // Opening the split view leaves this view with half of the width it had.
    m_container->resize(400, 600);
    QVERIFY(QTest::qWaitFor([this]() {
        return m_view->size().width() < 500;
    }));

    QVERIFY2(topItem >= m_view->firstVisibleIndex() && topItem <= m_view->lastVisibleIndex(),
             qPrintable(QStringLiteral("item %1 was at the top and is now outside the visible range %2 to %3")
                            .arg(topItem)
                            .arg(m_view->firstVisibleIndex())
                            .arg(m_view->lastVisibleIndex())));
}

/**
 * Opening and closing the split view has to leave the view where it started. Every reflow moves
 * where the top row begins, so a view that took the topmost item afresh on each resize would hold
 * on to a slightly earlier one every time and creep towards the start of the folder, a row per
 * toggle. See bug 524143.
 */
void DolphinItemListViewTest::testTheViewComesBackToWhereItWasWhenTheWidthDoes()
{
    QVERIFY(showViewWithFiles(KStandardItemListView::IconsLayout, QSize(800, 600)));

    m_view->setScrollOffset(m_view->maximumScrollOffset() / 2);
    const int topItem = m_view->firstVisibleIndex();
    const qreal offset = m_view->scrollOffset();
    QVERIFY(topItem > 0);

    // The width is walked down and back up the way the split view animation walks it.
    for (int round = 0; round < 3; ++round) {
        for (int width = 800; width >= 400; width -= 50) {
            m_container->resize(width, 600);
            QCoreApplication::processEvents();
        }
        for (int width = 400; width <= 800; width += 50) {
            m_container->resize(width, 600);
            QCoreApplication::processEvents();
        }
        QVERIFY(QTest::qWaitFor([this]() {
            return m_view->size().width() > 700;
        }));

        QCOMPARE(m_view->firstVisibleIndex(), topItem);
        QCOMPARE(m_view->scrollOffset(), offset);
    }
}

/** A resize back to the end of the list leaves the last row clear of the small statusbar. */
void DolphinItemListViewTest::testTheLastRowStaysClearOfTheStatusBarWhenTheViewGetsWider()
{
    const int statusBarOffset = 30;
    m_view->setStatusBarOffset(statusBarOffset);
    QVERIFY(showViewWithFiles(KStandardItemListView::IconsLayout, QSize(400, 600)));

    // At the very end of the folder.
    m_view->setScrollOffset(m_view->maximumScrollOffset() - m_view->size().height());
    const int lastItem = m_model->count() - 1;
    QVERIFY(m_view->itemRect(lastItem).bottom() <= m_view->size().height() - statusBarOffset);

    // Closing the split view: the list gets shorter and the view follows its end.
    QVERIFY(resizeContainer(QSize(800, 600)));

    QCOMPARE(m_view->scrollOffset(), m_view->maximumScrollOffset() - m_view->size().height());
    QVERIFY2(m_view->itemRect(lastItem).bottom() <= m_view->size().height() - statusBarOffset,
             qPrintable(QStringLiteral("the last item ends at %1, below the statusbar at %2")
                            .arg(m_view->itemRect(lastItem).bottom())
                            .arg(m_view->size().height() - statusBarOffset)));
}

std::pair<qreal, qreal> DolphinItemListViewTest::itemSpan(int index, KStandardItemListView::ItemLayout layout) const
{
    const QRectF rect = m_view->itemRect(index);
    if (layout != KStandardItemListView::CompactLayout) {
        return {rect.top(), rect.bottom()};
    }
    if (QGuiApplication::isRightToLeft()) {
        return {m_view->size().width() - rect.right(), m_view->size().width() - rect.left()};
    }
    return {rect.left(), rect.right()};
}

/** Per kind of resize: name, container size before, container size after. */
static QList<std::tuple<const char *, QSize, QSize>> resizes()
{
    return {
        {"narrower", QSize(800, 600), QSize(400, 600)},
        {"wider", QSize(400, 600), QSize(800, 600)},
        {"shorter", QSize(800, 600), QSize(800, 300)},
        {"taller", QSize(800, 300), QSize(800, 600)},
    };
}

void DolphinItemListViewTest::testTheViewStaysAtTheStartOrTheEndWhenResized_data()
{
    QTest::addColumn<KStandardItemListView::ItemLayout>("layout");
    QTest::addColumn<bool>("atEnd");
    QTest::addColumn<QSize>("from");
    QTest::addColumn<QSize>("to");

    const QList<std::pair<const char *, KStandardItemListView::ItemLayout>> layouts = {
        {"icons", KStandardItemListView::IconsLayout},
        {"compact", KStandardItemListView::CompactLayout},
        {"details", KStandardItemListView::DetailsLayout},
    };

    for (const auto &[layoutName, layout] : layouts) {
        for (const bool atEnd : {false, true}) {
            for (const auto &[resizeName, from, to] : resizes()) {
                QTest::addRow("%s, at the %s, %s", layoutName, atEnd ? "end" : "start", resizeName) << layout << atEnd << from << to;
            }
        }
    }
}

/** A view at the start or the end of the list stays there when resized. */
void DolphinItemListViewTest::testTheViewStaysAtTheStartOrTheEndWhenResized()
{
    QFETCH(KStandardItemListView::ItemLayout, layout);
    QFETCH(bool, atEnd);
    QFETCH(QSize, from);
    QFETCH(QSize, to);

    QVERIFY(showViewWithFiles(layout, from));

    // Offset showing the end of the list; compact scrolls sideways.
    const auto lastOffset = [this, layout]() {
        const qreal visibleLength = (layout == KStandardItemListView::CompactLayout) ? m_view->size().width() : m_view->size().height();
        return m_view->maximumScrollOffset() - visibleLength;
    };
    QVERIFY(lastOffset() > 0);
    m_view->setScrollOffset(atEnd ? lastOffset() : 0);

    QVERIFY(resizeContainer(to));

    if (atEnd) {
        QCOMPARE(m_view->scrollOffset(), lastOffset());
        QCOMPARE(m_view->lastVisibleIndex(), m_model->count() - 1);
    } else {
        QCOMPARE(m_view->scrollOffset(), qreal(0));
        QCOMPARE(m_view->firstVisibleIndex(), 0);
    }
}

void DolphinItemListViewTest::testASelectedItemOnScreenStaysOnScreenWhenResized_data()
{
    QTest::addColumn<KStandardItemListView::ItemLayout>("layout");
    QTest::addColumn<Qt::LayoutDirection>("layoutDirection");
    QTest::addColumn<QByteArray>("position");
    QTest::addColumn<QSize>("from");
    QTest::addColumn<QSize>("to");

    const QList<std::tuple<const char *, KStandardItemListView::ItemLayout, Qt::LayoutDirection>> layouts = {
        {"icons", KStandardItemListView::IconsLayout, Qt::LeftToRight},
        {"compact", KStandardItemListView::CompactLayout, Qt::LeftToRight},
        {"compact right to left", KStandardItemListView::CompactLayout, Qt::RightToLeft},
        {"details", KStandardItemListView::DetailsLayout, Qt::LeftToRight},
    };
    const QList<std::pair<const char *, const char *>> positions = {
        {"at the start", "start"},
        {"in the middle", "middle"},
        {"at the end", "end"},
    };

    for (const auto &[layoutName, layout, layoutDirection] : layouts) {
        for (const auto &[positionName, position] : positions) {
            for (const auto &[resizeName, from, to] : resizes()) {
                QTest::addRow("%s, %s, %s", layoutName, positionName, resizeName) << layout << layoutDirection << QByteArray(position) << from << to;
            }
        }
    }
}

/**
 * A visible selected item stays fully visible through a resize. Within that, the view keeps to the
 * start, the end or the item's old position.
 */
void DolphinItemListViewTest::testASelectedItemOnScreenStaysOnScreenWhenResized()
{
    QFETCH(KStandardItemListView::ItemLayout, layout);
    QFETCH(Qt::LayoutDirection, layoutDirection);
    QFETCH(QByteArray, position);
    QFETCH(QSize, from);
    QFETCH(QSize, to);

    // The layouter follows the application's direction.
    auto restoreLayoutDirection = qScopeGuard([] {
        QGuiApplication::setLayoutDirection(Qt::LeftToRight);
    });
    QGuiApplication::setLayoutDirection(layoutDirection);

    KItemListSelectionManager *selectionManager = m_controller->selectionManager();
    m_view->setLayoutDirection(layoutDirection);
    QVERIFY(showViewWithFiles(layout, from));

    const auto visibleLength = [this, layout]() {
        return (layout == KStandardItemListView::CompactLayout) ? m_view->size().width() : m_view->size().height();
    };
    const auto lastOffset = [this, &visibleLength]() {
        return m_view->maximumScrollOffset() - visibleLength();
    };
    // At offset 0 the first item marks where the visible area begins, below any header.
    const qreal areaStart = itemSpan(0, layout).first;
    const auto isFullyVisible = [this, layout, areaStart, &visibleLength](int index) {
        const auto [start, end] = itemSpan(index, layout);
        return start >= areaStart - 1 && end <= visibleLength() + 1;
    };

    QVERIFY(lastOffset() > 0);
    if (position == "middle") {
        m_view->setScrollOffset(lastOffset() / 2);
    } else if (position == "end") {
        m_view->setScrollOffset(lastOffset());
    }

    // Select the item most likely pushed off: the last fully visible one, or the first at the end.
    int selected = -1;
    if (position == "end") {
        for (int index = m_view->firstVisibleIndex(); selected < 0 && index <= m_view->lastVisibleIndex(); ++index) {
            if (isFullyVisible(index)) {
                selected = index;
            }
        }
    } else {
        for (int index = m_view->lastVisibleIndex(); selected < 0 && index >= m_view->firstVisibleIndex(); --index) {
            if (isFullyVisible(index)) {
                selected = index;
            }
        }
    }
    QVERIFY(selected >= 0);
    const qreal offset = m_view->scrollOffset();
    selectionManager->setCurrentItem(selected);
    selectionManager->setSelected(selected);
    QCOMPARE(m_view->scrollOffset(), offset);
    const qreal distance = itemSpan(selected, layout).first;

    QVERIFY(resizeContainer(to));

    const auto [start, end] = itemSpan(selected, layout);
    QVERIFY2(isFullyVisible(selected),
             qPrintable(QStringLiteral("item %1 spans %2 to %3, outside of the visible area from %4 to %5")
                            .arg(selected)
                            .arg(start)
                            .arg(end)
                            .arg(areaStart)
                            .arg(visibleLength())));

    // Moving only as far as needed leaves the item at the opposite side of the view.
    const qreal length = end - start;
    const bool atTheNearSide = start < areaStart + length;
    const bool atTheFarSide = end > visibleLength() - length;
    if (position == "start") {
        QVERIFY2(m_view->scrollOffset() < 1 || atTheFarSide,
                 qPrintable(QStringLiteral("scrolled to %1 with item %2 at %3").arg(m_view->scrollOffset()).arg(selected).arg(start)));
    } else if (position == "end") {
        QVERIFY2(m_view->scrollOffset() > lastOffset() - 1 || atTheNearSide,
                 qPrintable(QStringLiteral("scrolled to %1 of %2 with item %3 at %4").arg(m_view->scrollOffset()).arg(lastOffset()).arg(selected).arg(start)));
    } else {
        QVERIFY2(qAbs(start - distance) < 1 || atTheNearSide || atTheFarSide,
                 qPrintable(QStringLiteral("item %1 moved from %2 to %3").arg(selected).arg(distance).arg(start)));
    }
}

void DolphinItemListViewTest::testASelectionOffScreenOrACurrentItemAloneDoesNotMoveTheView_data()
{
    QTest::addColumn<KStandardItemListView::ItemLayout>("layout");
    QTest::addColumn<bool>("currentItemOnly");

    const QList<std::pair<const char *, KStandardItemListView::ItemLayout>> layouts = {
        {"icons", KStandardItemListView::IconsLayout},
        {"compact", KStandardItemListView::CompactLayout},
        {"details", KStandardItemListView::DetailsLayout},
    };

    for (const auto &[layoutName, layout] : layouts) {
        QTest::addRow("%s, selection off screen", layoutName) << layout << false;
        QTest::addRow("%s, current item only", layoutName) << layout << true;
    }
}

/** An off-screen selection or an unselected current item does not move the view. */
void DolphinItemListViewTest::testASelectionOffScreenOrACurrentItemAloneDoesNotMoveTheView()
{
    QFETCH(KStandardItemListView::ItemLayout, layout);
    QFETCH(bool, currentItemOnly);

    KItemListSelectionManager *selectionManager = m_controller->selectionManager();
    QVERIFY(showViewWithFiles(layout, QSize(800, 600)));

    m_view->setScrollOffset(m_view->maximumScrollOffset() / 2);
    const int topItem = m_view->firstVisibleIndex();
    QVERIFY(topItem > 0);

    if (currentItemOnly) {
        // No longer fits in a shorter view.
        selectionManager->setCurrentItem(m_view->lastVisibleIndex());
        QVERIFY(!selectionManager->hasSelection());
    } else {
        // Above the visible area.
        selectionManager->setCurrentItem(0);
        selectionManager->setSelected(0);
    }
    const qreal topDistance = itemSpan(topItem, layout).first;

    QVERIFY(resizeContainer(QSize(800, 300)));

    QCOMPARE(itemSpan(topItem, layout).first, topDistance);
}

void DolphinItemListViewTest::testTheViewKeepsUpWithChangesBetweenResizes_data()
{
    QTest::addColumn<bool>("zoom");

    QTest::newRow("selecting an item") << false;
    QTest::newRow("zooming in") << true;
}

/** Selecting or zooming between two resizes invalidates the held anchor. */
void DolphinItemListViewTest::testTheViewKeepsUpWithChangesBetweenResizes()
{
    QFETCH(bool, zoom);

    KItemListSelectionManager *selectionManager = m_controller->selectionManager();
    QVERIFY(showViewWithFiles(KStandardItemListView::IconsLayout, QSize(800, 600)));

    m_view->setScrollOffset(m_view->maximumScrollOffset() / 2);

    // Leaves a held anchor.
    QVERIFY(resizeContainer(QSize(400, 600)));

    int selected = -1;
    if (zoom) {
        const qreal maximumOffset = m_view->maximumScrollOffset();
        m_view->setZoomLevel(m_view->zoomLevel() + 1);
        QVERIFY(m_view->maximumScrollOffset() != maximumOffset);
    } else {
        // Last fully visible item; no longer fits in a shorter view.
        for (int index = m_view->lastVisibleIndex(); selected < 0 && index >= m_view->firstVisibleIndex(); --index) {
            if (m_view->itemRect(index).bottom() <= m_view->size().height()) {
                selected = index;
            }
        }
        QVERIFY(selected >= 0);
        selectionManager->setCurrentItem(selected);
        selectionManager->setSelected(selected);
    }
    const qreal offset = m_view->scrollOffset();

    QVERIFY(resizeContainer(QSize(400, 300)));

    if (zoom) {
        // A shorter view does not reflow, so nothing should move.
        QCOMPARE(m_view->scrollOffset(), offset);
    } else {
        const QRectF rect = m_view->itemRect(selected);
        QVERIFY2(rect.top() >= 0 && rect.bottom() <= m_view->size().height(),
                 qPrintable(
                     QStringLiteral("item %1 spans %2 to %3 in a view %4 high").arg(selected).arg(rect.top()).arg(rect.bottom()).arg(m_view->size().height())));
    }
}

QTEST_MAIN(DolphinItemListViewTest)

#include "dolphinitemlistviewtest.moc"
