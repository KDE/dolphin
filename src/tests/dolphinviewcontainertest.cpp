/*
 * SPDX-FileCopyrightText: 2026 Sebastian Englbrecht
 * SPDX-FileCopyrightText: 2026 Méven Car <meven@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "dolphinviewcontainer.h"
#include "dolphin_generalsettings.h"
#include "dolphinurlnavigator.h"
#include "kitemviews/kitemlistcontroller.h"
#include "kitemviews/kitemliststyleoption.h"
#include "kitemviews/kitemlistview.h"
#include "testdir.h"
#include "views/dolphincolumnpane.h"
#include "views/dolphincolumnsview.h"
#include "views/dolphinview.h"
#include "views/viewproperties.h"

#include <QCoreApplication>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTest>

class DolphinViewContainerTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void init();
    void cleanup();

    void testSetViewMode_iconsToDetails();
    void testSetViewMode_iconsToColumns();
    void testSetViewMode_columnsToIcons();
    void testSetViewMode_fullCycle();
    void testSetViewMode_columnsToDetailsThenBackToColumns();
    void testSetViewMode_sameModeTwice();
    void testColumnsIconSizeStaysNonZeroOnPreviewToggle();
    void testSwapDoesNotPersistOutgoingMode();
    void testNavigatorFollowsViewAfterModeSwap();
    void testSwapAdoptsContainerActiveState();

private:
    void waitForViewReady();

    DolphinViewContainer *m_container = nullptr;
    TestDir *m_testDir = nullptr;
};

void DolphinViewContainerTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
    // Use per-folder view properties so each test's fresh TestDir isolates
    // persistence (the global store would leak modes between tests).
    GeneralSettings::setGlobalViewProps(false);
}

void DolphinViewContainerTest::init()
{
    m_testDir = new TestDir();
    m_testDir->createDir("subdir");
    m_testDir->createFile("file1.txt");
    m_testDir->createFile("file2.txt");

    m_container = new DolphinViewContainer(m_testDir->url(), nullptr);
    m_container->resize(800, 600);
    m_container->show();
    QVERIFY(QTest::qWaitForWindowExposed(m_container));

    waitForViewReady();
}

void DolphinViewContainerTest::cleanup()
{
    delete m_container;
    m_container = nullptr;

    delete m_testDir;
    m_testDir = nullptr;
}

void DolphinViewContainerTest::waitForViewReady()
{
    QTRY_VERIFY_WITH_TIMEOUT(m_container->view()->itemsCount() > 0, 5000);
}

void DolphinViewContainerTest::testSetViewMode_iconsToDetails()
{
    // Start in Icons mode
    m_container->setViewMode(DolphinView::IconsView);
    waitForViewReady();
    QCOMPARE(m_container->view()->viewMode(), DolphinView::IconsView);
    QVERIFY(qobject_cast<DolphinColumnsView *>(m_container->view()) == nullptr);

    // Switch to Details
    m_container->setViewMode(DolphinView::DetailsView);
    waitForViewReady();
    QCOMPARE(m_container->view()->viewMode(), DolphinView::DetailsView);
    QVERIFY(qobject_cast<DolphinColumnsView *>(m_container->view()) == nullptr);
}

void DolphinViewContainerTest::testSetViewMode_iconsToColumns()
{
    m_container->setViewMode(DolphinView::IconsView);
    waitForViewReady();
    QVERIFY(qobject_cast<DolphinColumnsView *>(m_container->view()) == nullptr);

    // Switch to Columns — should swap the view
    QSignalSpy spy(m_container, &DolphinViewContainer::viewReplaced);
    m_container->setViewMode(DolphinView::ColumnsView);
    waitForViewReady();

    QCOMPARE(spy.count(), 1);
    QCOMPARE(m_container->view()->viewMode(), DolphinView::ColumnsView);
    QVERIFY(qobject_cast<DolphinColumnsView *>(m_container->view()) != nullptr);
}

void DolphinViewContainerTest::testSetViewMode_columnsToIcons()
{
    // Start in Columns
    m_container->setViewMode(DolphinView::ColumnsView);
    waitForViewReady();
    QVERIFY(qobject_cast<DolphinColumnsView *>(m_container->view()) != nullptr);

    // Switch to Icons — should swap back
    QSignalSpy spy(m_container, &DolphinViewContainer::viewReplaced);
    m_container->setViewMode(DolphinView::IconsView);
    waitForViewReady();

    QCOMPARE(spy.count(), 1);
    QCOMPARE(m_container->view()->viewMode(), DolphinView::IconsView);
    QVERIFY(qobject_cast<DolphinColumnsView *>(m_container->view()) == nullptr);
}

void DolphinViewContainerTest::testSetViewMode_fullCycle()
{
    // Icons → Compact → Details → Columns → Icons → Compact → Details → Columns
    const DolphinView::Mode modes[] = {
        DolphinView::IconsView,
        DolphinView::CompactView,
        DolphinView::DetailsView,
        DolphinView::ColumnsView,
        DolphinView::IconsView,
        DolphinView::CompactView,
        DolphinView::DetailsView,
        DolphinView::ColumnsView,
    };

    for (DolphinView::Mode mode : modes) {
        m_container->setViewMode(mode);
        waitForViewReady();

        QCOMPARE(m_container->view()->viewMode(), mode);

        const bool shouldBeColumns = (mode == DolphinView::ColumnsView);
        const bool isColumns = qobject_cast<DolphinColumnsView *>(m_container->view()) != nullptr;
        QCOMPARE(isColumns, shouldBeColumns);
    }
}

void DolphinViewContainerTest::testSetViewMode_columnsToDetailsThenBackToColumns()
{
    // Columns → Details → Columns
    m_container->setViewMode(DolphinView::ColumnsView);
    waitForViewReady();
    QVERIFY(qobject_cast<DolphinColumnsView *>(m_container->view()) != nullptr);

    m_container->setViewMode(DolphinView::DetailsView);
    waitForViewReady();
    QCOMPARE(m_container->view()->viewMode(), DolphinView::DetailsView);
    QVERIFY(qobject_cast<DolphinColumnsView *>(m_container->view()) == nullptr);

    m_container->setViewMode(DolphinView::ColumnsView);
    waitForViewReady();
    QCOMPARE(m_container->view()->viewMode(), DolphinView::ColumnsView);
    QVERIFY(qobject_cast<DolphinColumnsView *>(m_container->view()) != nullptr);
}

void DolphinViewContainerTest::testSetViewMode_sameModeTwice()
{
    m_container->setViewMode(DolphinView::ColumnsView);
    waitForViewReady();

    DolphinView *viewBefore = m_container->view();
    QSignalSpy spy(m_container, &DolphinViewContainer::viewReplaced);

    // Setting same mode again should NOT swap the view
    m_container->setViewMode(DolphinView::ColumnsView);
    waitForViewReady();

    QCOMPARE(spy.count(), 0);
    QCOMPARE(m_container->view(), viewBefore);
}

void DolphinViewContainerTest::testColumnsIconSizeStaysNonZeroOnPreviewToggle()
{
    m_container->setViewMode(DolphinView::ColumnsView);
    waitForViewReady();

    auto *columnsView = qobject_cast<DolphinColumnsView *>(m_container->view());
    QVERIFY(columnsView);
    QVERIFY(columnsView->columnCount() > 0);

    DolphinColumnPane *pane = columnsView->columnAt(0);
    QVERIFY(pane);

    // Toggling previews must never collapse the column icon size to 0. A zero
    // icon size made the column icons vanish and aborted in
    // KStandardItemListWidget::addOverlays() via std::clamp (lo > hi).
    m_container->view()->setPreviewsShown(true);
    QVERIFY(pane->controller()->view()->styleOption().iconSize > 0);

    m_container->view()->setPreviewsShown(false);
    QVERIFY(pane->controller()->view()->styleOption().iconSize > 0);

    m_container->view()->setPreviewsShown(true);
    QVERIFY(pane->controller()->view()->styleOption().iconSize > 0);
}

void DolphinViewContainerTest::testSwapDoesNotPersistOutgoingMode()
{
    const QUrl url = m_container->view()->url();

    // Switch into Columns and back out. Each switch swaps (destroys) the
    // outgoing view; the outgoing view must not persist its now-stale mode and
    // clobber the surviving view's mode on disk.
    m_container->setViewMode(DolphinView::ColumnsView);
    waitForViewReady();
    m_container->setViewMode(DolphinView::IconsView);
    waitForViewReady();

    // Synchronously run the deleteLater() of the swapped-out views so their
    // destructors (which persist the view mode) fire before we read it back.
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);

    // We ended in Icons, so the destroyed Columns view must not have written
    // ColumnsView back to disk.
    ViewProperties props(url);
    QVERIFY(props.viewMode() != DolphinView::ColumnsView);
}

void DolphinViewContainerTest::testNavigatorFollowsViewAfterModeSwap()
{
    // Attach a url navigator the way the toolbar does.
    auto *navigator = new DolphinUrlNavigator(m_testDir->url(), nullptr);
    m_container->connectUrlNavigator(navigator);

    // Switching to Columns swaps (recreates) the view. The navigator must keep
    // following the new view, not the destroyed old one.
    m_container->setViewMode(DolphinView::ColumnsView);
    waitForViewReady();

    // "subdir" is empty, so navigating there opens no auto-cascaded child column
    // and the URL settles deterministically.
    const QUrl subUrl = QUrl::fromLocalFile(m_testDir->path() + QStringLiteral("/subdir"));
    m_container->setUrl(subUrl);

    QTRY_COMPARE_WITH_TIMEOUT(navigator->locationUrl().adjusted(QUrl::StripTrailingSlash), subUrl.adjusted(QUrl::StripTrailingSlash), 5000);

    m_container->disconnectUrlNavigator();
    delete navigator;
}

void DolphinViewContainerTest::testSwapAdoptsContainerActiveState()
{
    // A freshly constructed DolphinView defaults to active (m_active is
    // initialized true). swapView() must sync the incoming view to the
    // container's real active state. Otherwise a view swapped in on an inactive
    // container (e.g. an inactive split pane switching to Columns) stays wrongly
    // "active", so the window - which only wires active-view signals (context
    // menu, ...) to the active view - never connects them to it.

    // Inactive container: the swapped-in columns view must be inactive.
    m_container->setActive(false);
    QVERIFY(!m_container->view()->isActive());

    m_container->setViewMode(DolphinView::ColumnsView);
    waitForViewReady();
    QVERIFY(qobject_cast<DolphinColumnsView *>(m_container->view()) != nullptr);
    QVERIFY(!m_container->view()->isActive());

    // Active container: the swapped-in view must be active.
    m_container->setActive(true);
    QVERIFY(m_container->view()->isActive());

    m_container->setViewMode(DolphinView::IconsView);
    waitForViewReady();
    QVERIFY(qobject_cast<DolphinColumnsView *>(m_container->view()) == nullptr);
    QVERIFY(m_container->view()->isActive());
}

QTEST_MAIN(DolphinViewContainerTest)

#include "dolphinviewcontainertest.moc"
