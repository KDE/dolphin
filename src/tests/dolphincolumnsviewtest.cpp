/*
 * SPDX-FileCopyrightText: 2026 Sebastian Englbrecht
 * SPDX-FileCopyrightText: 2026 Méven Car <meven@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "views/dolphincolumnsview.h"
#include "dolphin_columnsmodesettings.h"
#include "dolphin_detailsmodesettings.h"
#include "dolphin_generalsettings.h"
#include "dolphin_iconsmodesettings.h"
#include "kitemviews/kfileitemmodel.h"
#include "kitemviews/kitemlistcontainer.h"
#include "kitemviews/kitemlistcontroller.h"
#include "kitemviews/kitemlistselectionmanager.h"
#include "testdir.h"
#include "views/dolphincolumnpane.h"
#include "views/dolphinitemlistview.h"
#include "views/viewproperties.h"
#include "views/zoomlevelinfo.h"

#include <KProtocolManager>

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QGraphicsView>
#include <QKeyEvent>
#include <QLineEdit>
#include <QScrollArea>
#include <QScrollBar>
#include <QSignalSpy>
#include <QSplitter>
#include <QStandardPaths>
#include <QTest>
#include <QVBoxLayout>

/**
 * @brief Unit tests for DolphinColumnsView (Miller Columns).
 *
 * Test directory structure:
 *   root/
 *     alpha/
 *       alpha-child/
 *         deep-file.txt
 *       file1.txt
 *       file2.txt
 *     beta/
 *       beta-file.txt
 *     gamma/           (empty directory)
 *     single-file.txt
 *
 * DolphinColumnsView auto-selects the first item when a column loads.
 * If the first item is a directory, a child column is opened automatically.
 * This means the initial column count after construction may be > 1.
 */
class DolphinColumnsViewTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void init();
    void cleanup();

    void testInitialState();
    void testColumnCountAfterNavigation();
    void testPopAfterNavigation();

    void testKeyRight_opensChild();
    void testKeyRight_fileDoesNotOpen();
    void testKeyLeft_activatesParent();
    void testKeyLeft_atFirstColumn();

    void testUrlUpdatesOnNavigation();
    void testSetUrl_rebuildsColumns();

    void testDefaultWidths();
    void testCustomWidthPreserved();
    void testCustomWidthCleanupOnPop();
    void testColumnWidthMode();
    void testAutoAdjustColumns();
    void testDoubleClickOnAHandleFitsTheColumns();
    void testIconSizeFollowsSettings();
    void testZoomingLeavesTheOtherViewModesAlone();
    void testRenameRefitsColumnWhenAdjustingToContent();
    void testNoJumpWhenSiblingSelectionReplacesWideColumn();
    void testAColumnKeepsItsWidthWhenItIsEntered();
    void testShownHiddenFileWidensColumn();
    void testColumnIsNeverWiderThanTheViewport();

    void testSelectionMatchesActiveColumn();
    void testModelMatchesActiveColumn();

    void testEmptyDirectory();
    void testReload();

    void testKeyReturn_fileEmitsItemActivated();
    void testKeyReturn_directoryOpensChild();
    void testKeyReturn_opensEverySelectedItem();
    void testKeyReturn_doesNotReloadAlreadyOpenChild();

    void testShiftRight_navigatesBetweenColumns();
    void testCtrlRight_navigatesBetweenColumns();

    void testSelectionPreservedAcrossNavigation();
    void testLeftKeyClearsChildSelection();
    void testDirectorySelectionOpensChildOnce();
    void testMouseClickOnDirectoryOpensChildOnce();
    void testActivatingADirectoryStaysInItsColumn();
    void testMouseClickOnNotCurrentDirectoryOpensChild();
    void testRightClickKeepsTheChildColumns();
    void testMouseClickOnAFileDropsTheColumnsAfterIt();
    void testDraggingAFileLeavesTheColumnsOpen();
    void testPressingAFolderLeavesTheColumnsOpen();
    void testSetUrlActivatesAnOpenColumn();
    void testSetUrlOpensTheColumnsDownToADescendant();
    void testNameFilterAppliesToEveryColumn();
    void testAColumnOpenedLaterInheritsTheFilter();
    void testFilterCaseSensitivityAppliesToEveryColumn();
    void testFilterModeAppliesToEveryColumn();
    void testAColumnOpenedLaterInheritsTheFilterModeAndCase();
    void testFilteringOutAFolderClosesItsColumn();
    void testAListedColumnOpensNothing();
    void testDeletingAnOpenFolderOpensNoOther();
    void testOpeningAFolderDoesNotOpenAFurtherColumn();
    void testEveryFolderOnThePathIsMarkedInItsParent();
    void testSpaceIsAShortcutWhenAColumnHasTheFocus();
    void testActivatingAnotherColumnKeepsWhatTheUserWasDoing();
    void testClosingColumnsLeavesTheScrollPositionAlone();
    void testTheColumnsStartAtTheRootTheResolverNames();
    void testAColumnWithNoWidthYetTakesUpNothing();
    void testOpeningAColumnLeavesTheOnesBeforeItAlone();
    void testTheFirstVisibleColumnIsShownWhole();
    void testWideningTheWindowShowsTheParentColumns();
    void testALongNameDoesNotPushOtherColumnsOut();
    void testChangingTheUrlLeavesTheFocusOutsideTheView();
    void testAColumnThatWaitedForItsWidthKeepsItsMinimum();
    void testReadingSettingsKeepsTheColumnsMode();
    void testTheDetailsSettingsDoNotReachTheColumns();

    void testAnArchiveOpensAsAFolder();
    void testEscape_clearsSelection();
    void testHomeEnd_withinColumn();

    void testBackButton_emitsGoBack();
    void testForwardButton_emitsGoForward();

private:
    void waitForStableState();
    void resetSettings();
    static int indexOfName(const DolphinColumnPane *pane, const QString &name);
    /// A left press and its release on the item at @p index, as a click delivers them.
    void clickItem(DolphinColumnPane *pane, int index);
    QUrl urlOf(const QString &relativePath) const;
    void navigateRight();
    void navigateLeft();
    void selectItemInColumn(int columnIndex, const QString &name);
    void activateColumn(int index);
    void sendKeyToActivePane(Qt::Key key, Qt::KeyboardModifiers mod = Qt::NoModifier);

    DolphinColumnsView *m_view = nullptr;
    TestDir *m_testDir = nullptr;
};

void DolphinColumnsViewTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
}

void DolphinColumnsViewTest::init()
{
    resetSettings();
    m_testDir = new TestDir();

    m_testDir->createDir("alpha");
    m_testDir->createDir("alpha/alpha-child");
    m_testDir->createFile("alpha/alpha-child/deep-file.txt");
    m_testDir->createFile("alpha/file1.txt");
    m_testDir->createFile("alpha/file2.txt");
    m_testDir->createDir("beta");
    m_testDir->createFile("beta/beta-file.txt");
    m_testDir->createDir("gamma");
    m_testDir->createFile("single-file.txt");

    m_view = new DolphinColumnsView(m_testDir->url(), nullptr, DolphinView::ColumnsView);
    m_view->resize(800, 400);

    m_view->show();
    QVERIFY(QTest::qWaitForWindowExposed(m_view));

    waitForStableState();
}

void DolphinColumnsViewTest::cleanup()
{
    delete m_view;
    m_view = nullptr;

    delete m_testDir;
    m_testDir = nullptr;

    resetSettings();
}

void DolphinColumnsViewTest::resetSettings()
{
    // Every test starts from the default settings, whatever the one before it changed.
    for (KCoreConfigSkeleton *settings : std::initializer_list<KCoreConfigSkeleton *>{ColumnsModeSettings::self(),
                                                                                      IconsModeSettings::self(),
                                                                                      DetailsModeSettings::self(),
                                                                                      GeneralSettings::self()}) {
        settings->setDefaults();
        settings->save();
    }
}

int DolphinColumnsViewTest::indexOfName(const DolphinColumnPane *pane, const QString &name)
{
    for (int i = 0; i < pane->model()->count(); ++i) {
        if (pane->model()->fileItem(i).name() == name) {
            return i;
        }
    }
    return -1;
}

QUrl DolphinColumnsViewTest::urlOf(const QString &relativePath) const
{
    return QUrl::fromLocalFile(m_testDir->path() + QLatin1Char('/') + relativePath);
}

void DolphinColumnsViewTest::clickItem(DolphinColumnPane *pane, int index)
{
    m_view->handleMouseButtonPressed(pane, index, Qt::LeftButton);
    m_view->handleMouseButtonReleased(pane, index);
}

void DolphinColumnsViewTest::waitForStableState()
{
    QTRY_VERIFY_WITH_TIMEOUT(m_view->columnCount() >= 1, 5000);
    QTRY_VERIFY_WITH_TIMEOUT(m_view->columnAt(0)->model()->count() > 0, 5000);

    // Wait for any pending directory loads to complete
    QSignalSpy loadSpy(m_view, &DolphinView::directoryLoadingCompleted);
    QTRY_VERIFY_WITH_TIMEOUT(loadSpy.count() > 0 || m_view->columnAt(0)->model()->count() > 0, 5000);
}

void DolphinColumnsViewTest::navigateRight()
{
    const int activeBefore = m_view->activeColumnIndex();
    QSignalSpy loadSpy(m_view, &DolphinView::directoryLoadingCompleted);

    sendKeyToActivePane(Qt::Key_Right);

    // Wait for active column to change
    QTRY_VERIFY_WITH_TIMEOUT(m_view->activeColumnIndex() > activeBefore, 5000);

    // Wait for the child column's directory to finish loading
    auto *childPane = m_view->columnAt(m_view->activeColumnIndex());
    if (childPane && childPane->model()->count() == 0) {
        QTRY_VERIFY_WITH_TIMEOUT(loadSpy.count() > 0 || childPane->model()->count() > 0, 5000);
    }
}

void DolphinColumnsViewTest::navigateLeft()
{
    const int activeBefore = m_view->activeColumnIndex();
    sendKeyToActivePane(Qt::Key_Left);
    QTRY_VERIFY_WITH_TIMEOUT(m_view->activeColumnIndex() < activeBefore || activeBefore == 0, 5000);
}

void DolphinColumnsViewTest::selectItemInColumn(int columnIndex, const QString &name)
{
    auto *pane = m_view->columnAt(columnIndex);
    QVERIFY2(pane, qPrintable(QStringLiteral("Column %1 does not exist").arg(columnIndex)));

    QTRY_VERIFY2_WITH_TIMEOUT(indexOfName(pane, name) >= 0, qPrintable(QStringLiteral("Item '%1' not found in column %2").arg(name).arg(columnIndex)), 5000);
    const int index = indexOfName(pane, name);
    auto *selectionManager = pane->controller()->selectionManager();
    // The listing made the first item current without opening it, so picking that one opens it here.
    const bool alreadyCurrent = selectionManager->currentItem() == index;
    selectionManager->setCurrentItem(index);
    selectionManager->setSelected(index, 1, KItemListSelectionManager::Select);
    if (alreadyCurrent) {
        m_view->followItem(columnIndex, pane->model()->fileItem(index));
    }
    QTRY_VERIFY_WITH_TIMEOUT(selectionManager->isSelected(index), 5000);
}

void DolphinColumnsViewTest::activateColumn(int index)
{
    QVERIFY(index >= 0 && index < m_view->columnCount());
    m_view->setActiveColumn(index);
    QTRY_COMPARE_WITH_TIMEOUT(m_view->activeColumnIndex(), index, 5000);
}

void DolphinColumnsViewTest::sendKeyToActivePane(Qt::Key key, Qt::KeyboardModifiers mod)
{
    auto *pane = m_view->columnAt(m_view->activeColumnIndex());
    QVERIFY(pane);
    QKeyEvent press(QEvent::KeyPress, key, mod);
    QCoreApplication::sendEvent(pane->container()->viewport(), &press);
    QCoreApplication::processEvents();
}

void DolphinColumnsViewTest::testInitialState()
{
    QVERIFY(m_view->columnCount() >= 1);
    QVERIFY(m_view->activeColumnIndex() >= 0);
    QVERIFY(m_view->activeColumnIndex() < m_view->columnCount());

    auto *rootPane = m_view->columnAt(0);
    QVERIFY(rootPane);
    QVERIFY(rootPane->model()->count() == 4); // alpha, beta, gamma, single-file.txt
}

void DolphinColumnsViewTest::testColumnCountAfterNavigation()
{
    activateColumn(0);
    selectItemInColumn(0, "beta");

    navigateRight();

    QVERIFY(m_view->columnCount() > 1);
    QVERIFY(m_view->activeColumnIndex() >= 1);
}

void DolphinColumnsViewTest::testPopAfterNavigation()
{
    activateColumn(0);
    selectItemInColumn(0, "alpha");
    navigateRight();

    QVERIFY(m_view->columnCount() >= 2);
    QVERIFY(m_view->activeColumnIndex() >= 1);

    navigateLeft();
    QCOMPARE(m_view->activeColumnIndex(), 0);
}

void DolphinColumnsViewTest::testKeyRight_opensChild()
{
    activateColumn(0);
    selectItemInColumn(0, "beta");
    navigateRight();

    QVERIFY(m_view->activeColumnIndex() >= 1);
    auto *childPane = m_view->columnAt(m_view->activeColumnIndex());
    QVERIFY(childPane);
    QVERIFY(childPane->dirUrl().path().contains("beta"));
}

void DolphinColumnsViewTest::testKeyRight_fileDoesNotOpen()
{
    activateColumn(0);
    selectItemInColumn(0, "single-file.txt");

    const int before = m_view->columnCount();
    sendKeyToActivePane(Qt::Key_Right);
    // Pressing Right on a file opens no child column. The handler runs
    // synchronously, so flushing pending events is enough to confirm that the
    // column count does not change.
    QCoreApplication::processEvents();

    QCOMPARE(m_view->columnCount(), before);
}

void DolphinColumnsViewTest::testKeyLeft_activatesParent()
{
    activateColumn(0);
    selectItemInColumn(0, "alpha");
    navigateRight();
    QVERIFY(m_view->activeColumnIndex() >= 1);

    navigateLeft();
    QCOMPARE(m_view->activeColumnIndex(), 0);
}

void DolphinColumnsViewTest::testKeyLeft_atFirstColumn()
{
    activateColumn(0);
    navigateLeft();

    QCOMPARE(m_view->activeColumnIndex(), 0);
    QVERIFY(m_view->columnCount() >= 1);
}

void DolphinColumnsViewTest::testUrlUpdatesOnNavigation()
{
    activateColumn(0);

    selectItemInColumn(0, "beta");
    navigateRight();

    QVERIFY(m_view->url().path().contains("beta"));
}

void DolphinColumnsViewTest::testSetUrl_rebuildsColumns()
{
    // A url that sits under none of the open columns has no column to keep, so the view starts
    // again from that url. A url under an open column extends the columns instead, which
    // testSetUrlOpensTheColumnsDownToADescendant covers.
    TestDir otherDir;
    otherDir.createDir("other-child");

    m_view->setUrl(otherDir.url());
    waitForStableState();

    QVERIFY(m_view->columnCount() >= 1);
    QCOMPARE(m_view->columnAt(0)->dirUrl().adjusted(QUrl::StripTrailingSlash), otherDir.url().adjusted(QUrl::StripTrailingSlash));
}

void DolphinColumnsViewTest::testDefaultWidths()
{
    for (int i = 0; i < m_view->columnCount(); ++i) {
        auto *pane = m_view->columnAt(i);
        QVERIFY(pane);
        QVERIFY(pane->width() > 0);
    }
}

void DolphinColumnsViewTest::testCustomWidthPreserved()
{
    activateColumn(0);
    selectItemInColumn(0, "alpha");
    navigateRight();
    QVERIFY(m_view->columnCount() >= 2);

    auto *splitter = m_view->findChild<QSplitter *>();
    QVERIFY(splitter);

    QList<int> sizes = splitter->sizes();
    QVERIFY(sizes.size() >= 2);
    const int customWidth = 250;
    sizes[0] = customWidth;
    splitter->setSizes(sizes);

    Q_EMIT splitter->splitterMoved(customWidth, 1);

    const int widthAfterCustom = splitter->sizes().at(0);
    QVERIFY(qAbs(widthAfterCustom - customWidth) <= 10);
}

void DolphinColumnsViewTest::testCustomWidthCleanupOnPop()
{
    activateColumn(0);
    selectItemInColumn(0, "alpha");
    navigateRight();

    const int colCount = m_view->columnCount();
    QVERIFY(colCount >= 2);

    auto *splitter = m_view->findChild<QSplitter *>();
    QVERIFY(splitter);

    Q_EMIT splitter->splitterMoved(300, colCount - 1);

    navigateLeft();

    QVERIFY(m_view->columnCount() >= 1);
}

void DolphinColumnsViewTest::testColumnWidthMode()
{
    // With "adjust to content" a column shrinks to fit its items; with "fixed" it
    // takes an equal share of the viewport. A small minimum keeps the two modes
    // clearly apart regardless of the viewport size.
    auto *settings = ColumnsModeSettings::self();
    settings->setMinColumnWidth(10);

    activateColumn(0);
    auto *splitter = m_view->findChild<QSplitter *>();
    QVERIFY(splitter);

    // Changing the width mode is one of the few things that may leave a column narrower than
    // it was, so it refits. readSettings() passes the same policy when the dialog applies.
    settings->setDynamicColumnWidth(false);
    m_view->recalculateColumnWidths(DolphinColumnsView::WidthPolicy::Refit);
    const int fixedWidth = splitter->sizes().at(0);

    settings->setDynamicColumnWidth(true);
    m_view->recalculateColumnWidths(DolphinColumnsView::WidthPolicy::Refit);
    const int contentWidth = splitter->sizes().at(0);

    QVERIFY(fixedWidth > 0);
    QVERIFY(contentWidth >= 10); // never below the configured minimum
    QVERIFY2(contentWidth < fixedWidth,
             qPrintable(QStringLiteral("content-sized column (%1) should be narrower than the fixed one (%2)").arg(contentWidth).arg(fixedWidth)));
}

void DolphinColumnsViewTest::testAutoAdjustColumns()
{
    // A double-click on a splitter handle (which calls autoAdjustColumns) fits
    // every column to its content and drops the widths the user dragged. How the
    // fit is kept afterwards depends on the width mode.
    auto *settings = ColumnsModeSettings::self();
    settings->setMinColumnWidth(10);

    activateColumn(0);
    auto *splitter = m_view->findChild<QSplitter *>();
    QVERIFY(splitter);

    // Adjust-to-content mode: fitting drops the manual widths so the columns keep
    // tracking their content, and nothing is pinned.
    settings->setDynamicColumnWidth(true);
    QList<int> sizes = splitter->sizes();
    const int manualWidth = 600;
    sizes[0] = manualWidth;
    splitter->setSizes(sizes);
    Q_EMIT splitter->splitterMoved(manualWidth, 1);
    QVERIFY(!m_view->m_customColumnWidths.isEmpty());

    m_view->autoAdjustColumns();
    const int fittedDynamic = splitter->sizes().at(0);
    QVERIFY2(fittedDynamic < manualWidth,
             qPrintable(QStringLiteral("fitted column (%1) should be narrower than the manual width (%2)").arg(fittedDynamic).arg(manualWidth)));
    QVERIFY(m_view->m_customColumnWidths.isEmpty());

    // Fixed-width mode: the fixed layout would otherwise snap columns back to an
    // equal share of the viewport, so the fit is pinned as a custom width and
    // survives a relayout.
    settings->setDynamicColumnWidth(false);
    m_view->recalculateColumnWidths();
    const int fixedWidth = splitter->sizes().at(0);

    m_view->autoAdjustColumns();
    const int fittedFixed = splitter->sizes().at(0);
    QVERIFY2(fittedFixed < fixedWidth,
             qPrintable(QStringLiteral("fitted column (%1) should be narrower than the fixed width (%2)").arg(fittedFixed).arg(fixedWidth)));
    QVERIFY(!m_view->m_customColumnWidths.isEmpty());

    // A later relayout keeps the fitted width instead of restoring the fixed one.
    m_view->recalculateColumnWidths();
    QCOMPARE(splitter->sizes().at(0), fittedFixed);
}

void DolphinColumnsViewTest::testDoubleClickOnAHandleFitsTheColumns()
{
    // The handle before a column only reacts once the view filters its events.
    auto *settings = ColumnsModeSettings::self();
    settings->setDynamicColumnWidth(true);

    m_view->openChild(0, urlOf(QStringLiteral("alpha")));
    waitForStableState();
    QCOMPARE(m_view->columnCount(), 2);

    auto *splitter = m_view->findChild<QSplitter *>();
    QVERIFY(splitter);
    Q_EMIT splitter->splitterMoved(600, 1);
    QVERIFY(!m_view->m_customColumnWidths.isEmpty());

    QTest::mouseDClick(splitter->handle(1), Qt::LeftButton);
    // mouseDClick() leaves the button held, and every later test would navigate with it down.
    QTest::mouseRelease(splitter->handle(1), Qt::LeftButton);
    QCOMPARE(QGuiApplication::mouseButtons(), Qt::NoButton);
    QTRY_VERIFY(m_view->m_customColumnWidths.isEmpty());
}

void DolphinColumnsViewTest::testIconSizeFollowsSettings()
{
    // With global view properties (the default) the icons are the size that is configured for
    // the columns view mode, so changing that size resizes them.
    auto *settings = ColumnsModeSettings::self();

    const int levelA = ZoomLevelInfo::minimumLevel();
    const int levelB = ZoomLevelInfo::minimumLevel() + 2;
    QVERIFY(levelB <= ZoomLevelInfo::maximumLevel());

    // Set both sizes so the result does not depend on whether previews are shown.
    settings->setIconSize(ZoomLevelInfo::iconSizeForZoomLevel(levelA));
    settings->setPreviewSize(ZoomLevelInfo::iconSizeForZoomLevel(levelA));
    settings->save();
    m_view->readSettings();
    QCOMPARE(m_view->zoomLevel(), levelA);

    settings->setIconSize(ZoomLevelInfo::iconSizeForZoomLevel(levelB));
    settings->setPreviewSize(ZoomLevelInfo::iconSizeForZoomLevel(levelB));
    settings->save();
    m_view->readSettings();
    QCOMPARE(m_view->zoomLevel(), levelB);
}

void DolphinColumnsViewTest::testZoomingLeavesTheOtherViewModesAlone()
{
    // Zooming keeps the new icon size for the columns view mode. The other view modes keep the
    // size each of them is configured with.
    auto *columns = ColumnsModeSettings::self();
    auto *icons = IconsModeSettings::self();
    auto *details = DetailsModeSettings::self();

    const int otherModesLevel = ZoomLevelInfo::minimumLevel();
    const int columnsLevel = ZoomLevelInfo::minimumLevel() + 2;
    QVERIFY(columnsLevel <= ZoomLevelInfo::maximumLevel());
    const int otherModesSize = ZoomLevelInfo::iconSizeForZoomLevel(otherModesLevel);
    icons->setIconSize(otherModesSize);
    details->setIconSize(otherModesSize);

    m_view->setZoomLevel(columnsLevel);

    QCOMPARE(m_view->zoomLevel(), columnsLevel);
    const int columnsSize = ZoomLevelInfo::iconSizeForZoomLevel(columnsLevel);
    QCOMPARE(m_view->previewsShown() ? columns->previewSize() : columns->iconSize(), columnsSize);
    QCOMPARE(icons->iconSize(), otherModesSize);
    QCOMPARE(details->iconSize(), otherModesSize);
}

void DolphinColumnsViewTest::testRenameRefitsColumnWhenAdjustingToContent()
{
    auto *settings = ColumnsModeSettings::self();
    // A small minimum keeps the content width from being clamped, so the change is visible.
    settings->setDynamicColumnWidth(true);
    settings->setMinColumnWidth(10);
    // Up to the whole viewport, the width of the content.
    settings->setMaxVisibleColumns(1);

    activateColumn(0);
    m_view->recalculateColumnWidths();
    auto *splitter = m_view->findChild<QSplitter *>();
    QVERIFY(splitter);
    const int widthBefore = splitter->sizes().at(0);

    // Renaming a file to a much longer name widens the column it lives in.
    const QString dir = m_testDir->url().toLocalFile();
    const QString longName = QStringLiteral("single-file-") + QString(80, QLatin1Char('x')) + QStringLiteral(".txt");
    QVERIFY(QFile::rename(dir + QStringLiteral("/single-file.txt"), dir + QLatin1Char('/') + longName));

    QTRY_VERIFY_WITH_TIMEOUT(splitter->sizes().at(0) > widthBefore, 5000);
}

void DolphinColumnsViewTest::testShownHiddenFileWidensColumn()
{
    auto *settings = ColumnsModeSettings::self();
    // A small minimum keeps the content width from being clamped, so the change is visible.
    settings->setDynamicColumnWidth(true);
    settings->setMinColumnWidth(10);
    // Up to the whole viewport, the width of the content.
    settings->setMaxVisibleColumns(1);
    m_view->setHiddenFilesShown(false);

    // A hidden file with a name far longer than any visible one.
    const QString longHiddenName = QStringLiteral(".hidden-") + QString(80, QLatin1Char('x')) + QStringLiteral(".txt");
    m_testDir->createFile(longHiddenName);

    activateColumn(0);
    m_view->recalculateColumnWidths();
    auto *splitter = m_view->findChild<QSplitter *>();
    QVERIFY(splitter);
    const int widthWithoutTheFile = splitter->sizes().at(0);

    // Showing the file makes the column wide enough for its name.
    m_view->setHiddenFilesShown(true);
    QTRY_VERIFY_WITH_TIMEOUT(splitter->sizes().at(0) > widthWithoutTheFile, 5000);
}

void DolphinColumnsViewTest::testNoJumpWhenSiblingSelectionReplacesWideColumn()
{
    auto *settings = ColumnsModeSettings::self();
    settings->setDynamicColumnWidth(true);
    settings->setMinColumnWidth(10);
    // Up to the whole viewport, the width of the content.
    settings->setMaxVisibleColumns(1);

    // alpha/alpha-child holds a name far wider than the window, and alpha has a second folder
    // to move to.
    m_testDir->createFile(QStringLiteral("alpha/alpha-child/") + QString(200, QLatin1Char('w')) + QStringLiteral(".txt"));
    m_testDir->createDir(QStringLiteral("alpha/alpha-child2"));

    // A window too narrow to hold every column at once.
    m_view->resize(350, 400);
    QTRY_VERIFY_WITH_TIMEOUT(m_view->m_scrollArea->viewport()->width() > 0, 5000);

    auto activeScreenX = [this]() {
        const int index = m_view->activeColumnIndex();
        return m_view->columnAt(index)->mapTo(m_view->m_scrollArea->viewport(), QPoint(0, 0)).x();
    };

    // root -> alpha -> alpha-child, so the wide column is the third one.
    activateColumn(0);
    selectItemInColumn(0, QStringLiteral("alpha"));
    QTRY_VERIFY_WITH_TIMEOUT(m_view->columnCount() > 1, 5000);
    navigateRight();
    selectItemInColumn(1, QStringLiteral("alpha-child"));
    QTRY_VERIFY_WITH_TIMEOUT(m_view->columnCount() > 2, 5000);
    navigateRight();

    // The folder is listed after its column was added, so the column only takes its full width
    // then. The view has to follow it, rather than being pulled there by a later keystroke.
    QTRY_COMPARE_WITH_TIMEOUT(activeScreenX(), 0, 5000);

    navigateLeft();
    QTRY_COMPARE_WITH_TIMEOUT(activeScreenX(), 0, 5000);
    const int screenXBefore = activeScreenX();
    const int scrollBefore = m_view->m_scrollArea->horizontalScrollBar()->value();

    const int childWidthBefore = m_view->m_splitter->sizes().at(2);

    // Moving to the sibling folder replaces the wide column with one holding shorter names.
    // The replacement keeps the width, so nothing shifts under the folder being looked at.
    // alpha was listed before this test added the second folder, so wait for the watcher.
    auto *alphaPane = m_view->columnAt(1);
    QTRY_VERIFY_WITH_TIMEOUT(indexOfName(alphaPane, QStringLiteral("alpha-child2")) >= 0, 10000);

    selectItemInColumn(1, QStringLiteral("alpha-child2"));
    QTRY_VERIFY_WITH_TIMEOUT(m_view->columnCount() > 2
                                 && m_view->columnAt(2)->dirUrl().adjusted(QUrl::StripTrailingSlash).fileName() == QStringLiteral("alpha-child2"),
                             5000);
    // UNAVOIDABLE: the relayout is posted with a zero timer and emits no signal, and the checks below are that nothing moved
    QTest::qWait(200); // UNAVOIDABLE: see above

    QCOMPARE(m_view->m_splitter->sizes().at(2), childWidthBefore);
    QCOMPARE(activeScreenX(), screenXBefore);
    QCOMPARE(m_view->m_scrollArea->horizontalScrollBar()->value(), scrollBefore);
}

void DolphinColumnsViewTest::testAColumnKeepsItsWidthWhenItIsEntered()
{
    auto *settings = ColumnsModeSettings::self();
    settings->setDynamicColumnWidth(true);
    settings->setMinColumnWidth(10);
    // Up to the whole viewport, the width of the content.
    settings->setMaxVisibleColumns(1);

    // One folder holds a name far wider than the other, so the width of the column to the right
    // differs a great deal between the two.
    m_testDir->createFile(QStringLiteral("wide-names/") + QString(120, QLatin1Char('w')) + QStringLiteral(".txt"));
    m_testDir->createFile(QStringLiteral("short-names/s.txt"));

    activateColumn(0);

    // The root was listed before this test created the two folders, so wait for the watcher.
    auto *rootPane = m_view->columnAt(0);
    QTRY_VERIFY_WITH_TIMEOUT(indexOfName(rootPane, QStringLiteral("wide-names")) >= 0 && indexOfName(rootPane, QStringLiteral("short-names")) >= 0, 10000);

    selectItemInColumn(0, QStringLiteral("wide-names"));
    QTRY_VERIFY_WITH_TIMEOUT(m_view->columnCount() > 1 && m_view->columnAt(1)->model()->count() > 0, 5000);
    QTRY_VERIFY_WITH_TIMEOUT(m_view->m_splitter->sizes().at(1) > 200, 5000);
    const int wideWidth = m_view->m_splitter->sizes().at(1);

    // The folder with the shorter name needs less room, and the column keeps the width it has.
    selectItemInColumn(0, QStringLiteral("short-names"));
    QTRY_VERIFY_WITH_TIMEOUT(m_view->columnCount() > 1
                                 && m_view->columnAt(1)->dirUrl().adjusted(QUrl::StripTrailingSlash).fileName() == QStringLiteral("short-names"),
                             5000);
    QTRY_VERIFY_WITH_TIMEOUT(m_view->columnAt(1)->model()->count() > 0, 5000);
    QTest::qWait(200); // UNAVOIDABLE: the relayout is posted with a zero timer and emits no signal
    QCOMPARE(m_view->m_splitter->sizes().at(1), wideWidth);

    // Entering the column is not the user asking for a narrower one either.
    navigateRight();
    QCOMPARE(m_view->activeColumnIndex(), 1);
    QTest::qWait(200); // UNAVOIDABLE: see above
    QCOMPARE(m_view->m_splitter->sizes().at(1), wideWidth);
}

void DolphinColumnsViewTest::testSelectionMatchesActiveColumn()
{
    auto *pane = m_view->columnAt(m_view->activeColumnIndex());
    QVERIFY(pane);
    auto *selectionManager = pane->controller()->selectionManager();
    QVERIFY(selectionManager);

    selectionManager->clearSelection();
    selectionManager->setCurrentItem(0);
    selectionManager->setSelected(0, 1, KItemListSelectionManager::Select);
    QVERIFY(selectionManager->hasSelection());

    KFileItemList selected = m_view->selectedItems();
    QCOMPARE(selected.count(), 1);
}

void DolphinColumnsViewTest::testModelMatchesActiveColumn()
{
    auto *pane = m_view->columnAt(0);
    QVERIFY(pane);

    auto *model = pane->model();
    QVERIFY(model);
    QVERIFY(model->count() > 0);

    QStringList names;
    for (int i = 0; i < model->count(); ++i) {
        names.append(model->fileItem(i).name());
    }
    QVERIFY(names.contains("alpha"));
    QVERIFY(names.contains("beta"));
    QVERIFY(names.contains("gamma"));
    QVERIFY(names.contains("single-file.txt"));
}

void DolphinColumnsViewTest::testEmptyDirectory()
{
    activateColumn(0);
    selectItemInColumn(0, "gamma");

    const int before = m_view->columnCount();
    navigateRight();

    if (m_view->columnCount() > before) {
        const int afterGamma = m_view->columnCount();
        navigateRight();
        QCOMPARE(m_view->columnCount(), afterGamma);
    }
}

void DolphinColumnsViewTest::testReload()
{
    const QUrl urlBefore = m_view->url();

    m_view->reload();
    waitForStableState();

    QCOMPARE(m_view->url().adjusted(QUrl::StripTrailingSlash), urlBefore.adjusted(QUrl::StripTrailingSlash));
    QVERIFY(m_view->columnAt(0)->model()->count() > 0);
}

void DolphinColumnsViewTest::testKeyReturn_fileEmitsItemActivated()
{
    activateColumn(0);
    selectItemInColumn(0, "single-file.txt");

    QSignalSpy spy(m_view, &DolphinView::itemActivated);
    sendKeyToActivePane(Qt::Key_Return);

    QCOMPARE(spy.count(), 1);
    QCOMPARE(spy.at(0).at(0).value<KFileItem>().name(), QStringLiteral("single-file.txt"));
}

void DolphinColumnsViewTest::testKeyReturn_directoryOpensChild()
{
    activateColumn(0);
    selectItemInColumn(0, "beta");

    QSignalSpy spy(m_view, &DolphinView::itemActivated);
    QSignalSpy loadSpy(m_view, &DolphinView::directoryLoadingCompleted);
    sendKeyToActivePane(Qt::Key_Return);
    QTRY_VERIFY_WITH_TIMEOUT(loadSpy.count() > 0 || m_view->activeColumnIndex() >= 1, 5000);

    QCOMPARE(spy.count(), 0);
    QVERIFY(m_view->activeColumnIndex() >= 1);
}

void DolphinColumnsViewTest::testKeyReturn_opensEverySelectedItem()
{
    // As in the other view modes, Return opens the whole selection, not only the current item.
    selectItemInColumn(0, QStringLiteral("alpha"));
    QTRY_COMPARE_WITH_TIMEOUT(m_view->columnCount(), 2, 5000);
    activateColumn(1);
    selectItemInColumn(1, QStringLiteral("file1.txt"));
    // Added to the selection without moving the current item, as a Ctrl+click does.
    auto *selectionManager = m_view->columnAt(1)->controller()->selectionManager();
    selectionManager->setSelected(indexOfName(m_view->columnAt(1), QStringLiteral("file2.txt")), 1, KItemListSelectionManager::Select);
    QCOMPARE(selectionManager->selectedItems().count(), 2);

    QSignalSpy itemActivated(m_view, &DolphinView::itemActivated);
    QSignalSpy itemsActivated(m_view, &DolphinView::itemsActivated);
    sendKeyToActivePane(Qt::Key_Return);

    QCOMPARE(itemActivated.count(), 0);
    QCOMPARE(itemsActivated.count(), 1);
    QStringList names;
    const auto activatedItems = itemsActivated.first().first().value<KFileItemList>();
    for (const KFileItem &item : activatedItems) {
        names << item.name();
    }
    names.sort();
    QCOMPARE(names, QStringList({QStringLiteral("file1.txt"), QStringLiteral("file2.txt")}));
}

void DolphinColumnsViewTest::testKeyReturn_doesNotReloadAlreadyOpenChild()
{
    activateColumn(0);
    selectItemInColumn(0, "beta");

    // First Return opens beta's child column.
    sendKeyToActivePane(Qt::Key_Return);
    QTRY_VERIFY_WITH_TIMEOUT(m_view->activeColumnIndex() >= 1, 5000);

    const int childCol = m_view->activeColumnIndex();
    DolphinColumnPane *childBefore = m_view->columnAt(childCol);
    QVERIFY(childBefore);
    const QUrl childUrl = childBefore->dirUrl();

    // Pressing Return again on the same folder should step into the existing
    // child column, not tear it down and reload it.
    activateColumn(0);
    selectItemInColumn(0, "beta");
    sendKeyToActivePane(Qt::Key_Return);
    QTRY_COMPARE_WITH_TIMEOUT(m_view->activeColumnIndex(), childCol, 5000);

    QCOMPARE(m_view->columnAt(childCol), childBefore);
    QCOMPARE(m_view->columnAt(childCol)->dirUrl(), childUrl);
}

void DolphinColumnsViewTest::testShiftRight_navigatesBetweenColumns()
{
    activateColumn(0);
    selectItemInColumn(0, "beta");

    QSignalSpy loadSpy(m_view, &DolphinView::directoryLoadingCompleted);
    sendKeyToActivePane(Qt::Key_Right, Qt::ShiftModifier);
    QTRY_VERIFY_WITH_TIMEOUT(loadSpy.count() > 0 || m_view->activeColumnIndex() >= 1, 5000);

    QVERIFY(m_view->activeColumnIndex() >= 1);
}

void DolphinColumnsViewTest::testCtrlRight_navigatesBetweenColumns()
{
    activateColumn(0);
    selectItemInColumn(0, "beta");

    QSignalSpy loadSpy(m_view, &DolphinView::directoryLoadingCompleted);
    sendKeyToActivePane(Qt::Key_Right, Qt::ControlModifier);
    QTRY_VERIFY_WITH_TIMEOUT(loadSpy.count() > 0 || m_view->activeColumnIndex() >= 1, 5000);

    QVERIFY(m_view->activeColumnIndex() >= 1);
}

void DolphinColumnsViewTest::testSelectionPreservedAcrossNavigation()
{
    activateColumn(0);
    selectItemInColumn(0, "beta");
    navigateRight();

    const int childIdx = m_view->activeColumnIndex();
    QVERIFY(childIdx >= 1);
    auto *childSelectionManager = m_view->columnAt(childIdx)->controller()->selectionManager();
    childSelectionManager->setSelected(0, 1, KItemListSelectionManager::Select);
    QVERIFY(childSelectionManager->hasSelection());

    navigateLeft();
    QCOMPARE(m_view->activeColumnIndex(), 0);

    // Navigate forward again, child selection should still be intact
    navigateRight();
    QVERIFY(childSelectionManager->hasSelection());
}

void DolphinColumnsViewTest::testLeftKeyClearsChildSelection()
{
    activateColumn(0);
    selectItemInColumn(0, "alpha");
    navigateRight();

    const int childIdx = m_view->activeColumnIndex();
    QVERIFY(childIdx >= 1);

    auto *childSelectionManager = m_view->columnAt(childIdx)->controller()->selectionManager();
    childSelectionManager->setSelected(0, 1, KItemListSelectionManager::Select);
    QVERIFY(childSelectionManager->hasSelection());

    // Left key moves focus to parent column but preserves child selection
    // (consistent with macOS Finder behaviour)
    navigateLeft();
    QCOMPARE(m_view->activeColumnIndex(), 0);
    QVERIFY(childSelectionManager->hasSelection());
}

void DolphinColumnsViewTest::testAnArchiveOpensAsAFolder()
{
    // The mime type of a listed item is determined lazily, and an archive whose type is not known
    // yet reads as a plain file.
    if (KProtocolManager::protocolForArchiveMimetype(QStringLiteral("application/zip")).isEmpty()) {
        QSKIP("No KIO worker opens zip archives here");
    }
    GeneralSettings::setBrowseThroughArchives(true);

    // An empty zip archive: the end of central directory record alone.
    QByteArray emptyZip("PK\x05\x06", 4);
    emptyZip.append(18, '\0');
    m_testDir->createFile(QStringLiteral("archive.zip"), emptyZip);

    const KFileItem archive(urlOf(QStringLiteral("archive.zip")));
    QVERIFY(archive.isFile());
    QVERIFY(!archive.isMimeTypeKnown());

    QCOMPARE(DolphinColumnPane::folderUrlFor(archive).scheme(), QStringLiteral("zip"));

    GeneralSettings::setBrowseThroughArchives(false);
    QVERIFY(DolphinColumnPane::folderUrlFor(archive).isEmpty());
}

void DolphinColumnsViewTest::testEscape_clearsSelection()
{
    auto *pane = m_view->columnAt(m_view->activeColumnIndex());
    auto *selectionManager = pane->controller()->selectionManager();

    selectionManager->setSelected(0, 1, KItemListSelectionManager::Select);
    QVERIFY(selectionManager->hasSelection());

    sendKeyToActivePane(Qt::Key_Escape);
    QVERIFY(!selectionManager->hasSelection());
}

void DolphinColumnsViewTest::testHomeEnd_withinColumn()
{
    activateColumn(0);

    auto *pane = m_view->columnAt(0);
    auto *selectionManager = pane->controller()->selectionManager();
    QVERIFY(pane->model()->count() >= 3);

    selectionManager->setCurrentItem(2);
    QCOMPARE(selectionManager->currentItem(), 2);

    sendKeyToActivePane(Qt::Key_Home);
    QCOMPARE(selectionManager->currentItem(), 0);

    sendKeyToActivePane(Qt::Key_End);
    QCOMPARE(selectionManager->currentItem(), pane->model()->count() - 1);
}

void DolphinColumnsViewTest::testBackButton_emitsGoBack()
{
    QSignalSpy spy(m_view, &DolphinView::goBackRequested);

    // Emit mouseButtonPressed directly because QGraphicsView does not
    // forward back/forward mouse buttons to the graphics scene.
    auto *pane = m_view->columnAt(m_view->activeColumnIndex());
    Q_EMIT pane->controller()->mouseButtonPressed(0, Qt::BackButton);
    QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, 5000);
}

void DolphinColumnsViewTest::testForwardButton_emitsGoForward()
{
    QSignalSpy spy(m_view, &DolphinView::goForwardRequested);

    auto *pane = m_view->columnAt(m_view->activeColumnIndex());
    Q_EMIT pane->controller()->mouseButtonPressed(0, Qt::ForwardButton);
    QTRY_COMPARE_WITH_TIMEOUT(spy.count(), 1, 5000);
}

void DolphinColumnsViewTest::testDirectorySelectionOpensChildOnce()
{
    // Selecting a directory in the active, focused column changes both the
    // current item and the selection, which drives two independent handlers
    // (slotColumnsCurrentItemChanged and the selectionChanged lambda). Verify
    // that the child column is opened once, shows the right folder, does not
    // reload, and is not torn down and rebuilt.
    activateColumn(0);
    auto *rootContainer = m_view->columnAt(0)->container();
    rootContainer->setFocus();
    if (!QTest::qWaitFor(
            [&]() {
                return rootContainer->hasFocus();
            },
            1000)) {
        QSKIP("Could not give keyboard focus to the column in this environment");
    }

    // "beta" is a directory and not the auto-selected first item.
    QSignalSpy loadSpy(m_view, &DolphinView::directoryLoadingCompleted);
    selectItemInColumn(0, "beta");

    QTRY_COMPARE_WITH_TIMEOUT(m_view->columnCount(), 2, 5000);
    QCOMPARE(m_view->columnAt(1)->dirUrl().fileName(), QStringLiteral("beta"));

    // Let any queued re-open/auto-select settle, then confirm the child column
    // is the same object (not rebuilt) and beta was loaded only once.
    auto *childPane = m_view->columnAt(1);
    QTRY_VERIFY_WITH_TIMEOUT(loadSpy.count() >= 1, 5000);
    // Settle briefly to confirm no second reload is queued; there is no signal for an event not happening.
    QTest::qWait(300); // UNAVOIDABLE: no signal for the absence of a reload
    QCOMPARE(m_view->columnCount(), 2);
    QCOMPARE(m_view->columnAt(1), childPane);
    // beta is loaded exactly once: no double pop-and-rebuild of the child column.
    QCOMPARE(loadSpy.count(), 1);
}

void DolphinColumnsViewTest::testMouseClickOnDirectoryOpensChildOnce()
{
    // A left-click opens the child of the *current* item (handleMouseButtonPressed).
    // openChild() then calls setActiveChildUrl() on the parent, which changes the
    // parent's selection and re-emits selectionChanged. Without the m_blockNavigation
    // guard on that handler this re-enters openChild() before the first call has
    // appended its column, so both calls see no child and build one each - two
    // columns for the same folder. Verify exactly one child column is created.
    activateColumn(0);
    auto *pane = m_view->columnAt(0);
    auto *selectionManager = pane->controller()->selectionManager();

    const int betaIndex = indexOfName(pane, QStringLiteral("beta"));
    QVERIFY(betaIndex >= 0);

    // Make "beta" the current item without opening its child yet. During a real
    // click the mouse button is held, so slotColumnsCurrentItemChanged()'s
    // "mouseButtons() == NoButton" guard suppresses it and only
    // handleMouseButtonPressed() opens the child. Offscreen there is no held
    // button, so block signals while seeding the current item to reproduce that
    // precondition (current == beta, no child column, selection differs from it).
    selectionManager->blockSignals(true);
    selectionManager->clearSelection();
    selectionManager->setCurrentItem(betaIndex);
    selectionManager->blockSignals(false);

    // Simulate a left-click on the already-current "beta". This opens exactly one
    // child column showing "beta"; the re-entrancy bug would open a second.
    clickItem(pane, betaIndex);

    QTRY_COMPARE_WITH_TIMEOUT(m_view->columnCount(), 2, 5000);
    QCOMPARE(m_view->columnAt(1)->dirUrl().fileName(), QStringLiteral("beta"));

    // Let any queued re-entrant open settle, then confirm still exactly one child.
    QTest::qWait(100); // UNAVOIDABLE: no signal for the absence of a re-entrant open
    QCOMPARE(m_view->columnCount(), 2);
    QCOMPARE(m_view->columnAt(1)->dirUrl().fileName(), QStringLiteral("beta"));
}

// Activating a folder shows it in the next column but leaves the selection on the folder itself,
// the way the Finder column view does. Moving into that column is the Right arrow's job, see
// testKeyRight_opensChild().
void DolphinColumnsViewTest::testActivatingADirectoryStaysInItsColumn()
{
    activateColumn(0);
    selectItemInColumn(0, "beta");

    // Selecting the folder already opens the column that previews it.
    QTRY_COMPARE_WITH_TIMEOUT(m_view->columnCount(), 2, 5000);
    QCOMPARE(m_view->columnAt(1)->dirUrl().fileName(), QStringLiteral("beta"));
    QTRY_VERIFY(m_view->columnAt(1)->model()->count() > 0);

    auto *pane = m_view->columnAt(0);
    const int betaIndex = indexOfName(pane, QStringLiteral("beta"));
    QVERIFY(betaIndex >= 0);

    // What a click or a double click reaches, depending on the single click setting.
    QVERIFY(QMetaObject::invokeMethod(pane, "slotItemActivated", Qt::DirectConnection, Q_ARG(int, betaIndex)));
    QCoreApplication::processEvents();

    QCOMPARE(m_view->activeColumnIndex(), 0);
    QVERIFY(!m_view->columnAt(1)->controller()->selectionManager()->hasSelection());

    // The column the selection opened is reused rather than torn down and built again.
    QCOMPARE(m_view->columnCount(), 2);
    QCOMPARE(m_view->columnAt(1)->dirUrl().fileName(), QStringLiteral("beta"));
}

void DolphinColumnsViewTest::testMouseClickOnNotCurrentDirectoryOpensChild()
{
    // The first click on an item that is not the current one has to open its child column.
    // KItemListController::onPress() emits mouseButtonPressed before it moves the current item,
    // so handleMouseButtonPressed() sees a clicked index that differs from the current one, and
    // slotColumnsCurrentItemChanged() is suppressed while the button is held.
    activateColumn(0);
    auto *pane = m_view->columnAt(0);
    auto *selectionManager = pane->controller()->selectionManager();

    int betaIndex = -1;
    int otherIndex = -1;
    for (int i = 0; i < pane->model()->count(); ++i) {
        const QString name = pane->model()->fileItem(i).name();
        if (name == QStringLiteral("beta")) {
            betaIndex = i;
        } else if (otherIndex < 0) {
            otherIndex = i;
        }
    }
    QVERIFY(betaIndex >= 0);
    QVERIFY(otherIndex >= 0);

    // Current is some other item, which is what a first click on "beta" starts from.
    selectionManager->blockSignals(true);
    selectionManager->clearSelection();
    selectionManager->setCurrentItem(otherIndex);
    selectionManager->blockSignals(false);
    QVERIFY(selectionManager->currentItem() != betaIndex);

    clickItem(pane, betaIndex);

    QTRY_COMPARE_WITH_TIMEOUT(m_view->columnCount(), 2, 5000);
    QCOMPARE(m_view->columnAt(1)->dirUrl().fileName(), QStringLiteral("beta"));
}

void DolphinColumnsViewTest::testRightClickKeepsTheChildColumns()
{
    // A right click opens the context menu, whose actions act on the folder of the column that
    // was clicked. Opening the child of the item under the cursor would take that folder away
    // before the menu is even shown.
    selectItemInColumn(0, QStringLiteral("alpha"));
    QTRY_COMPARE_WITH_TIMEOUT(m_view->columnCount(), 2, 5000);
    activateColumn(1);

    auto *pane = m_view->columnAt(0);
    const int betaIndex = indexOfName(pane, QStringLiteral("beta"));
    QVERIFY(betaIndex >= 0);

    m_view->handleMouseButtonPressed(pane, betaIndex, Qt::RightButton);

    // The column that was clicked becomes the active one, so the menu acts on the right folder.
    QCOMPARE(m_view->activeColumnIndex(), 0);
    QCOMPARE(m_view->columnCount(), 2);
    QCOMPARE(m_view->columnAt(1)->dirUrl().adjusted(QUrl::StripTrailingSlash).fileName(), QStringLiteral("alpha"));
}

void DolphinColumnsViewTest::testMouseClickOnAFileDropsTheColumnsAfterIt()
{
    // The columns to the right of a click belong to a folder that is no longer the selected item,
    // so clicking a file closes them and the url goes back to the folder holding the file.
    selectItemInColumn(0, QStringLiteral("alpha"));
    QTRY_COMPARE_WITH_TIMEOUT(m_view->columnCount(), 2, 5000);

    auto *pane = m_view->columnAt(0);
    const int fileIndex = indexOfName(pane, QStringLiteral("single-file.txt"));
    QVERIFY(fileIndex >= 0);

    QSignalSpy urlSpy(m_view, &DolphinView::urlChanged);
    clickItem(pane, fileIndex);

    QTRY_COMPARE_WITH_TIMEOUT(m_view->columnCount(), 1, 5000);
    QCOMPARE(m_view->url().adjusted(QUrl::StripTrailingSlash), m_testDir->url().adjusted(QUrl::StripTrailingSlash));
    QCOMPARE(urlSpy.count(), 1);
}

void DolphinColumnsViewTest::testDraggingAFileLeavesTheColumnsOpen()
{
    // Dragging a file to a folder shown further right needs that column to still be there when
    // the drag arrives, so a press that becomes a drag closes nothing.
    selectItemInColumn(0, QStringLiteral("alpha"));
    QTRY_COMPARE_WITH_TIMEOUT(m_view->columnCount(), 2, 5000);

    auto *pane = m_view->columnAt(0);
    const int fileIndex = indexOfName(pane, QStringLiteral("single-file.txt"));
    QVERIFY(fileIndex >= 0);

    QSignalSpy urlSpy(m_view, &DolphinView::urlChanged);
    m_view->handleMouseButtonPressed(pane, fileIndex, Qt::LeftButton);
    QCOMPARE(m_view->columnCount(), 2);

    Q_EMIT pane->controller()->draggingStarted();
    QTest::qWait(100); // UNAVOIDABLE: proving that nothing closes needs time for it to happen
    QCOMPARE(m_view->columnCount(), 2);

    // The release that ends the drag is not a click on the file either.
    m_view->handleMouseButtonReleased(pane, fileIndex);
    QTest::qWait(100); // UNAVOIDABLE: see above
    QCOMPARE(m_view->columnCount(), 2);
    QCOMPARE(urlSpy.count(), 0);
}

void DolphinColumnsViewTest::testPressingAFolderLeavesTheColumnsOpen()
{
    // A folder dragged to a folder in the next column needs that column to still be there, so a
    // press on a folder opens nothing until its release. The press goes through the controller,
    // which selects the pressed item.
    selectItemInColumn(0, QStringLiteral("alpha"));
    QTRY_COMPARE_WITH_TIMEOUT(m_view->columnCount(), 2, 5000);
    QCOMPARE(m_view->activeColumnIndex(), 0);

    auto *pane = m_view->columnAt(0);
    const int betaIndex = indexOfName(pane, QStringLiteral("beta"));
    QVERIFY(betaIndex >= 0);
    auto *graphicsView = qobject_cast<QGraphicsView *>(pane->container()->viewport());
    QVERIFY(graphicsView);
    const QPoint betaPos = graphicsView->mapFromScene(pane->itemListView()->itemRect(betaIndex).center());

    QTest::mousePress(graphicsView->viewport(), Qt::LeftButton, Qt::NoModifier, betaPos);
    QVERIFY(pane->controller()->selectionManager()->isSelected(betaIndex));
    QTest::qWait(100); // UNAVOIDABLE: proving that nothing opens needs time for it to happen
    QCOMPARE(m_view->columnCount(), 2);
    QCOMPARE(m_view->columnAt(1)->dirUrl(), urlOf(QStringLiteral("alpha")));

    QTest::mouseRelease(graphicsView->viewport(), Qt::LeftButton, Qt::NoModifier, betaPos);
    QTRY_COMPARE_WITH_TIMEOUT(m_view->columnAt(1)->dirUrl(), urlOf(QStringLiteral("beta")), 5000);
    QCOMPARE(m_view->columnCount(), 2);
}

void DolphinColumnsViewTest::testSetUrlActivatesAnOpenColumn()
{
    // Going back to a folder one of the columns already shows keeps the columns below it, which
    // is what Back does. Rebuilding would leave that folder as the only column.
    selectItemInColumn(0, QStringLiteral("alpha"));
    QTRY_COMPARE_WITH_TIMEOUT(m_view->columnCount(), 2, 5000);
    activateColumn(1);
    selectItemInColumn(1, QStringLiteral("alpha-child"));
    QTRY_COMPARE_WITH_TIMEOUT(m_view->columnCount(), 3, 5000);

    m_view->setUrl(urlOf(QStringLiteral("alpha")));
    waitForStableState();

    QCOMPARE(m_view->columnCount(), 3);
    QCOMPARE(m_view->activeColumnIndex(), 1);
    QCOMPARE(m_view->columnAt(2)->dirUrl().adjusted(QUrl::StripTrailingSlash).fileName(), QStringLiteral("alpha-child"));
}

void DolphinColumnsViewTest::testSetUrlOpensTheColumnsDownToADescendant()
{
    // A url below the column that is open opens the folders in between rather than becoming the
    // only column, so the breadcrumb and the Places panel land where the columns already are.
    // The fixture auto-selects the first item, so a preview column may be open already.
    QVERIFY(m_view->columnCount() >= 1);
    QCOMPARE(m_view->columnAt(0)->dirUrl().adjusted(QUrl::StripTrailingSlash), m_testDir->url().adjusted(QUrl::StripTrailingSlash));

    m_view->setUrl(urlOf(QStringLiteral("alpha/alpha-child")));
    waitForStableState();

    QTRY_COMPARE_WITH_TIMEOUT(m_view->columnCount(), 3, 5000);
    QCOMPARE(m_view->columnAt(1)->dirUrl().adjusted(QUrl::StripTrailingSlash).fileName(), QStringLiteral("alpha"));
    QCOMPARE(m_view->columnAt(2)->dirUrl().adjusted(QUrl::StripTrailingSlash).fileName(), QStringLiteral("alpha-child"));
}

void DolphinColumnsViewTest::testNameFilterAppliesToEveryColumn()
{
    // The filter belongs to the view, so it reaches every column and not only the active one.
    selectItemInColumn(0, QStringLiteral("alpha"));
    QTRY_COMPARE_WITH_TIMEOUT(m_view->columnCount(), 2, 5000);
    QTRY_VERIFY_WITH_TIMEOUT(m_view->columnAt(1)->model()->count() > 0, 5000);

    const int unfilteredRootCount = m_view->columnAt(0)->model()->count();
    const int unfilteredChildCount = m_view->columnAt(1)->model()->count();

    // "al" matches alpha in the root and alpha-child in alpha. It has to keep alpha itself
    // visible, because filtering the folder a column was opened from closes that column.
    m_view->setNameFilter(QStringLiteral("al"));

    QTRY_COMPARE_WITH_TIMEOUT(m_view->columnAt(0)->model()->count(), 1, 5000);
    QCOMPARE(m_view->columnCount(), 2);
    QTRY_COMPARE_WITH_TIMEOUT(m_view->columnAt(1)->model()->count(), 1, 5000);
    QCOMPARE(m_view->columnAt(1)->model()->fileItem(0).name(), QStringLiteral("alpha-child"));

    m_view->setNameFilter(QString());
    QTRY_COMPARE_WITH_TIMEOUT(m_view->columnAt(0)->model()->count(), unfilteredRootCount, 5000);
    QTRY_COMPARE_WITH_TIMEOUT(m_view->columnAt(1)->model()->count(), unfilteredChildCount, 5000);
}

void DolphinColumnsViewTest::testAColumnOpenedLaterInheritsTheFilter()
{
    // A column opened while a filter is set is filtered as well.
    m_view->setNameFilter(QStringLiteral("a"));
    // alpha, beta and gamma carry an "a", single-file.txt does not.
    QTRY_COMPARE_WITH_TIMEOUT(m_view->columnAt(0)->model()->count(), 3, 5000);

    selectItemInColumn(0, QStringLiteral("alpha"));
    QTRY_COMPARE_WITH_TIMEOUT(m_view->columnCount(), 2, 5000);

    // alpha holds alpha-child, file1.txt and file2.txt, and only the first carries an "a".
    QTRY_COMPARE_WITH_TIMEOUT(m_view->columnAt(1)->model()->count(), 1, 5000);
    QCOMPARE(m_view->columnAt(1)->model()->fileItem(0).name(), QStringLiteral("alpha-child"));

    m_view->setNameFilter(QString());
}

void DolphinColumnsViewTest::testFilterCaseSensitivityAppliesToEveryColumn()
{
    // Case sensitivity belongs to the view, so it reaches the columns that are already open.
    m_testDir->createFile("alpha/ALPHA-UPPER.txt");

    selectItemInColumn(0, QStringLiteral("alpha"));
    QTRY_COMPARE_WITH_TIMEOUT(m_view->columnCount(), 2, 5000);
    QTRY_COMPARE_WITH_TIMEOUT(m_view->columnAt(1)->model()->count(), 4, 5000);

    m_view->setFilterCaseSensitive(false);
    m_view->setNameFilter(QStringLiteral("alpha"));

    // alpha in the root, and alpha-child plus ALPHA-UPPER.txt in alpha.
    QTRY_COMPARE_WITH_TIMEOUT(m_view->columnAt(0)->model()->count(), 1, 5000);
    QTRY_COMPARE_WITH_TIMEOUT(m_view->columnAt(1)->model()->count(), 2, 5000);

    m_view->setFilterCaseSensitive(true);

    QCOMPARE(m_view->columnAt(0)->model()->isFilterCaseSensitive(), true);
    QCOMPARE(m_view->columnAt(1)->model()->isFilterCaseSensitive(), true);
    QTRY_COMPARE_WITH_TIMEOUT(m_view->columnAt(1)->model()->count(), 1, 5000);
    QCOMPARE(m_view->columnAt(1)->model()->fileItem(0).name(), QStringLiteral("alpha-child"));
}

void DolphinColumnsViewTest::testFilterModeAppliesToEveryColumn()
{
    // The filter mode reaches the columns that are already open. "^alpha" matches as a regular
    // expression and matches nothing as a glob, where both characters are literal.
    selectItemInColumn(0, QStringLiteral("alpha"));
    QTRY_COMPARE_WITH_TIMEOUT(m_view->columnCount(), 2, 5000);

    m_view->setFilterMode(KFileItemModelFilter::Regex);
    m_view->setNameFilter(QStringLiteral("^alpha"));

    QCOMPARE(m_view->columnAt(0)->model()->filterMode(), KFileItemModelFilter::Regex);
    QCOMPARE(m_view->columnAt(1)->model()->filterMode(), KFileItemModelFilter::Regex);

    QTRY_COMPARE_WITH_TIMEOUT(m_view->columnAt(0)->model()->count(), 1, 5000);
    QTRY_COMPARE_WITH_TIMEOUT(m_view->columnAt(1)->model()->count(), 1, 5000);
    QCOMPARE(m_view->columnAt(1)->model()->fileItem(0).name(), QStringLiteral("alpha-child"));
}

void DolphinColumnsViewTest::testAColumnOpenedLaterInheritsTheFilterModeAndCase()
{
    // appendPane() gives a new column the whole filter, and not only the name.
    m_view->setFilterMode(KFileItemModelFilter::Regex);
    m_view->setFilterCaseSensitive(true);
    m_view->setNameFilter(QStringLiteral("^alpha"));
    QTRY_COMPARE_WITH_TIMEOUT(m_view->columnAt(0)->model()->count(), 1, 5000);

    selectItemInColumn(0, QStringLiteral("alpha"));
    QTRY_COMPARE_WITH_TIMEOUT(m_view->columnCount(), 2, 5000);

    QCOMPARE(m_view->columnAt(1)->model()->filterMode(), KFileItemModelFilter::Regex);
    QCOMPARE(m_view->columnAt(1)->model()->isFilterCaseSensitive(), true);
    QCOMPARE(m_view->columnAt(1)->model()->nameFilter(), QStringLiteral("^alpha"));
    QTRY_COMPARE_WITH_TIMEOUT(m_view->columnAt(1)->model()->count(), 1, 5000);
}

void DolphinColumnsViewTest::testFilteringOutAFolderClosesItsColumn()
{
    // A column stands for an item of the column to its left. Filtering that item away leaves the
    // column with nothing to stand for, so it closes.
    selectItemInColumn(0, QStringLiteral("alpha"));
    QTRY_COMPARE_WITH_TIMEOUT(m_view->columnCount(), 2, 5000);
    waitForStableState();

    // "file" matches single-file.txt in the root and leaves out alpha.
    m_view->setNameFilter(QStringLiteral("file"));

    QTRY_COMPARE_WITH_TIMEOUT(m_view->columnCount(), 1, 5000);
    QCOMPARE(m_view->columnAt(0)->model()->count(), 1);
}

void DolphinColumnsViewTest::testOpeningAFolderDoesNotOpenAFurtherColumn()
{
    // alpha holds alpha-child, which is a folder. Opening alpha selects alpha-child in the new
    // column, and must stop there. Opening it as well puts a column on screen that nobody asked
    // for, and browsing then adds and drops one at every step.
    activateColumn(0);
    selectItemInColumn(0, QStringLiteral("alpha"));
    navigateRight();
    QTRY_COMPARE_WITH_TIMEOUT(m_view->columnCount(), 2, 5000);

    // Entering a column selects its first item, and that is what used to open the column after
    // it. Once the selection is on alpha-child, that code has had its turn.
    auto *selectionManager = m_view->columnAt(1)->controller()->selectionManager();
    QTRY_VERIFY_WITH_TIMEOUT(selectionManager->selectedItems().count() == 1, 5000);
    QCOMPARE(m_view->columnAt(1)->model()->fileItem(selectionManager->currentItem()).name(), QStringLiteral("alpha-child"));

    QCOMPARE(m_view->columnCount(), 2);
}

void DolphinColumnsViewTest::testEveryFolderOnThePathIsMarkedInItsParent()
{
    // Restoring a session opens the columns for a url that is already several folders deep, so
    // each of them is asked to mark its child before it has listed and the item does not exist
    // yet. The mark has to be put on once the listing arrives.
    m_view->setUrl(urlOf(QStringLiteral("alpha/alpha-child")));
    waitForStableState();
    QTRY_COMPARE_WITH_TIMEOUT(m_view->columnCount(), 3, 5000);

    auto markedNameIn = [this](int column) {
        auto *selectionManager = m_view->columnAt(column)->controller()->selectionManager();
        const int current = selectionManager->currentItem();
        if (current < 0 || !selectionManager->isSelected(current)) {
            return QString();
        }
        return m_view->columnAt(column)->model()->fileItem(current).name();
    };

    QTRY_COMPARE_WITH_TIMEOUT(markedNameIn(0), QStringLiteral("alpha"), 5000);
    QTRY_COMPARE_WITH_TIMEOUT(markedNameIn(1), QStringLiteral("alpha-child"), 5000);
}

void DolphinColumnsViewTest::testSpaceIsAShortcutWhenAColumnHasTheFocus()
{
    // DolphinMainWindow asks this before letting Space through as a shortcut, and the answer has
    // to be about the column that holds the focus. The base view asks its own container, which
    // no column ever is, so Space always arrived as a normal key and selection mode never came on.
    activateColumn(0);
    m_view->columnAt(0)->container()->setFocus();
    QTRY_VERIFY_WITH_TIMEOUT(m_view->columnAt(0)->container()->hasFocus(), 5000);

    QCOMPARE(m_view->handleSpaceAsNormalKey(), false);
}

void DolphinColumnsViewTest::testReadingSettingsKeepsTheColumnsMode()
{
    // Applying anything in the settings dialog has every view re-read the view properties of its
    // folder. Those hold the mode the folder was last shown in, which is not the columns mode,
    // and taking it left the view reporting that mode while it still drew columns.
    // Written out, because readSettings() loads GeneralSettings from disk again.
    GeneralSettings::setGlobalViewProps(false);
    QVERIFY(GeneralSettings::self()->save());
    {
        ViewProperties props(m_testDir->url());
        props.setViewMode(DolphinView::DetailsView);
        props.save();
    }
    QCOMPARE(ViewProperties(m_testDir->url()).viewMode(), DolphinView::DetailsView);

    m_view->readSettings();

    QCOMPARE(m_view->viewMode(), DolphinView::ColumnsView);
    QVERIFY(m_view->columnCount() >= 1);
}

void DolphinColumnsViewTest::testTheDetailsSettingsDoNotReachTheColumns()
{
    // Each column draws with the details layout, and DolphinItemListView::readSettings() reads
    // the settings that go with the layout it finds. The details view's own settings would then
    // decide how a column looks, so a change made for the details view showed up here.
    DetailsModeSettings::setHighlightEntireRow(false);
    DetailsModeSettings::setExpandableFolders(true);
    QVERIFY(DetailsModeSettings::self()->save());

    m_view->readSettings();

    auto *paneView = m_view->columnAt(0)->itemListView();
    QVERIFY(paneView);
    // A column is one row wide, and it never expands a folder in place.
    QCOMPARE(paneView->highlightEntireRow(), true);
    QCOMPARE(paneView->supportsItemExpanding(), false);
}

void DolphinColumnsViewTest::testActivatingAnotherColumnKeepsWhatTheUserWasDoing()
{
    // DolphinViewContainer leaves selection mode when the url changes, because what the user had
    // selected is gone. Activating another column changes the url too, and closing selection mode
    // then took it away from under them. The view answers for the url change it is emitting.
    selectItemInColumn(0, QStringLiteral("alpha"));
    QTRY_COMPARE_WITH_TIMEOUT(m_view->columnCount(), 2, 5000);
    activateColumn(1);

    bool leavesBehindOnColumnSwitch = true;
    auto columnSwitch = connect(m_view, &DolphinView::urlChanged, this, [this, &leavesBehindOnColumnSwitch]() {
        leavesBehindOnColumnSwitch = m_view->urlChangeLeavesTheSelectionBehind();
    });
    m_view->setActiveColumn(0);
    disconnect(columnSwitch);
    QCOMPARE(leavesBehindOnColumnSwitch, false);

    // A url that none of the columns shows is somewhere else, and there selection mode goes.
    TestDir otherDir;
    bool leavesBehindOnNavigation = false;
    auto navigation = connect(m_view, &DolphinView::urlChanged, this, [this, &leavesBehindOnNavigation]() {
        leavesBehindOnNavigation = m_view->urlChangeLeavesTheSelectionBehind();
    });
    m_view->setUrl(otherDir.url());
    disconnect(navigation);
    QCOMPARE(leavesBehindOnNavigation, true);
}

void DolphinColumnsViewTest::testClosingColumnsLeavesTheScrollPositionAlone()
{
    // Clicking a file closes the columns that stood for a folder, and the content is then
    // narrower than where the view is scrolled to. Pulling the scroll position back slides
    // everything sideways under the user, so the filler takes up what the position needs.
    auto *settings = ColumnsModeSettings::self();
    settings->setDynamicColumnWidth(true);
    settings->setMinColumnWidth(10);
    // Up to the whole viewport, the width of the content.
    settings->setMaxVisibleColumns(1);

    // A name far wider than the window, so the columns do not fit and the view has somewhere to
    // scroll to.
    m_testDir->createFile(QStringLiteral("alpha/alpha-child/") + QString(200, QLatin1Char('w')) + QStringLiteral(".txt"));
    m_testDir->createDir(QStringLiteral("alpha/alpha-child2"));

    m_view->resize(350, 400);
    QTRY_VERIFY_WITH_TIMEOUT(m_view->m_scrollArea->viewport()->width() > 0, 5000);

    activateColumn(0);
    selectItemInColumn(0, QStringLiteral("alpha"));
    QTRY_VERIFY_WITH_TIMEOUT(m_view->columnCount() > 1, 5000);
    navigateRight();
    selectItemInColumn(1, QStringLiteral("alpha-child"));
    QTRY_VERIFY_WITH_TIMEOUT(m_view->columnCount() > 2, 5000);
    navigateRight();

    auto *scrollBar = m_view->m_scrollArea->horizontalScrollBar();
    QTRY_VERIFY_WITH_TIMEOUT(scrollBar->maximum() > 0, 5000);

    // Put alpha at the left edge: the user is looking at it, which is why they can click in it.
    auto *viewport = m_view->m_scrollArea->viewport();
    scrollBar->setValue(scrollBar->value() + m_view->columnAt(1)->mapTo(viewport, QPoint(0, 0)).x());
    QVERIFY(scrollBar->value() > 0);
    const int scrollBefore = scrollBar->value();

    // A file in alpha, the column the user is looking at, closes the one to its right.
    auto *pane = m_view->columnAt(1);
    const int fileIndex = indexOfName(pane, QStringLiteral("file1.txt"));
    QVERIFY(fileIndex >= 0);
    clickItem(pane, fileIndex);
    QTRY_COMPARE_WITH_TIMEOUT(m_view->columnCount(), 2, 5000);

    QCOMPARE(scrollBar->value(), scrollBefore);
    // The filler holds the width open, so the position is not merely set back and then clamped
    // away once the layout settles.
    const int neededWidth = scrollBefore + viewport->width();
    QVERIFY2(
        m_view->m_splitter->minimumWidth() >= neededWidth,
        qPrintable(QStringLiteral("splitter keeps %1, which is below the %2 the position needs").arg(m_view->m_splitter->minimumWidth()).arg(neededWidth)));
}

void DolphinColumnsViewTest::testTheColumnsStartAtTheRootTheResolverNames()
{
    // Restoring a session opens a url that is several folders deep. Without a root to start from
    // that folder was the only column and everything above it was gone, so there was nothing to
    // scroll left to.
    const QUrl deepUrl = urlOf(QStringLiteral("alpha/alpha-child"));
    const QUrl rootUrl = m_testDir->url();

    DolphinColumnsView view(deepUrl, nullptr, DolphinView::ColumnsView, [rootUrl](const QUrl &) {
        return rootUrl;
    });
    view.resize(1200, 600);
    view.show();
    QVERIFY(QTest::qWaitForWindowExposed(&view));

    QTRY_COMPARE_WITH_TIMEOUT(view.columnCount(), 3, 5000);
    QCOMPARE(view.columnAt(0)->dirUrl().adjusted(QUrl::StripTrailingSlash), rootUrl.adjusted(QUrl::StripTrailingSlash));
    QCOMPARE(view.columnAt(1)->dirUrl().adjusted(QUrl::StripTrailingSlash).fileName(), QStringLiteral("alpha"));
    QCOMPARE(view.columnAt(2)->dirUrl().adjusted(QUrl::StripTrailingSlash), deepUrl.adjusted(QUrl::StripTrailingSlash));
    // The folder that was asked for is the one the user lands on.
    QCOMPARE(view.url().adjusted(QUrl::StripTrailingSlash), deepUrl.adjusted(QUrl::StripTrailingSlash));
}

void DolphinColumnsViewTest::testAColumnWithNoWidthYetTakesUpNothing()
{
    // A column sized before its folder has listed takes the width of an empty model and then
    // resizes to the width of its content, which the user sees as the column jumping as it
    // opens. A column whose width is still pending takes up nothing at all, handle included.
    auto *settings = ColumnsModeSettings::self();
    settings->setDynamicColumnWidth(true);
    settings->setMinColumnWidth(10);

    m_view->resize(900, 400);
    QTRY_VERIFY_WITH_TIMEOUT(m_view->m_scrollArea->viewport()->width() > 0, 5000);

    selectItemInColumn(0, QStringLiteral("alpha"));
    QTRY_COMPARE_WITH_TIMEOUT(m_view->columnCount(), 2, 5000);
    QTRY_VERIFY_WITH_TIMEOUT(m_view->m_splitter->sizes().at(1) > 0, 5000);
    const int rootWidth = m_view->m_splitter->sizes().at(0);

    m_view->columnAt(1)->setWidthPending(true);
    m_view->recalculateColumnWidths();

    QTRY_COMPARE_WITH_TIMEOUT(m_view->m_splitter->sizes().at(1), 0, 5000);
    QVERIFY(!m_view->m_splitter->handle(1)->isVisible());
    // The column it was opened from keeps its place.
    QCOMPARE(m_view->m_splitter->sizes().at(0), rootWidth);

    m_view->columnAt(1)->setWidthPending(false);
    m_view->recalculateColumnWidths();
    QTRY_VERIFY_WITH_TIMEOUT(m_view->m_splitter->sizes().at(1) > 0, 5000);
    QVERIFY(m_view->m_splitter->handle(1)->isVisible());
}

void DolphinColumnsViewTest::testAColumnThatWaitedForItsWidthKeepsItsMinimum()
{
    // A column has no width while its folder lists, and takes back the minimum of every column
    // after that, so a drag cannot make it narrower than the others.
    ColumnsModeSettings::self()->setDynamicColumnWidth(true);

    selectItemInColumn(0, QStringLiteral("alpha"));
    QTRY_COMPARE_WITH_TIMEOUT(m_view->columnCount(), 2, 5000);
    auto *child = m_view->columnAt(1);
    QTRY_VERIFY_WITH_TIMEOUT(!child->isWidthPending(), 5000);

    QCOMPARE(child->minimumWidth(), m_view->columnAt(0)->minimumWidth());
    QVERIFY(child->minimumWidth() > 0);
}

void DolphinColumnsViewTest::testAListedColumnOpensNothing()
{
    // A column that lists its folder makes its first item current. Nobody chose that item, so
    // nothing opens and nothing is selected, even with the focus in that column. A reload lists
    // the folder again.
    activateColumn(0);
    m_view->columnAt(0)->container()->setFocus();
    QTRY_VERIFY_WITH_TIMEOUT(m_view->columnAt(0)->container()->hasFocus(), 5000);
    QCOMPARE(m_view->columnCount(), 1);

    m_view->reload();
    waitForStableState();
    QTRY_VERIFY_WITH_TIMEOUT(m_view->columnAt(0)->model()->count() > 0, 5000);
    QTest::qWait(200); // UNAVOIDABLE: the check below is that nothing opens afterwards

    QCOMPARE(m_view->columnCount(), 1);
    QVERIFY(!m_view->columnAt(0)->controller()->selectionManager()->hasSelection());
}

void DolphinColumnsViewTest::testDeletingAnOpenFolderOpensNoOther()
{
    // The folder after the deleted one becomes the current item of the column. Nobody chose it,
    // so the column of the deleted folder closes and no other column opens.
    m_view->columnAt(0)->container()->setFocus();
    QTRY_VERIFY_WITH_TIMEOUT(m_view->columnAt(0)->container()->hasFocus(), 5000);
    selectItemInColumn(0, QStringLiteral("alpha"));
    QTRY_COMPARE_WITH_TIMEOUT(m_view->columnCount(), 2, 5000);

    QVERIFY(QDir(m_testDir->path() + QStringLiteral("/alpha")).removeRecursively());

    QTRY_COMPARE_WITH_TIMEOUT(m_view->columnCount(), 1, 10000);
    QTest::qWait(200); // UNAVOIDABLE: the check below is that no other column opens afterwards
    QCOMPARE(m_view->columnCount(), 1);
}

void DolphinColumnsViewTest::testOpeningAColumnLeavesTheOnesBeforeItAlone()
{
    // The width the splitter is held at has to cover every column, the filler and every handle
    // that is drawn. One pixel short and the splitter takes that pixel out of one of the columns,
    // which moves every column after it as the new one opens.
    auto *settings = ColumnsModeSettings::self();
    settings->setDynamicColumnWidth(true);
    settings->setMinColumnWidth(200);

    m_testDir->createDir("alpha/alpha-child/deep");
    m_testDir->createFile("alpha/alpha-child/deep/deep-file.txt");

    // Narrow enough that the columns overflow it, so the splitter is held at the width they take.
    m_view->resize(420, 400);
    QTRY_VERIFY_WITH_TIMEOUT(m_view->m_scrollArea->viewport()->width() > 0, 5000);

    m_view->openChild(0, urlOf(QStringLiteral("alpha")));
    QTRY_VERIFY_WITH_TIMEOUT(m_view->m_splitter->sizes().at(1) > 0, 5000);
    m_view->openChild(1, urlOf(QStringLiteral("alpha/alpha-child")));
    QTRY_VERIFY_WITH_TIMEOUT(m_view->m_splitter->sizes().at(2) > 0, 5000);
    QTRY_VERIFY_WITH_TIMEOUT(m_view->m_splitter->minimumWidth() > 0, 5000);

    const QList<int> before = m_view->m_splitter->sizes().mid(0, 3);

    m_view->openChild(2, urlOf(QStringLiteral("alpha/alpha-child/deep")));
    QTRY_VERIFY_WITH_TIMEOUT(m_view->m_splitter->sizes().at(3) > 0, 5000);

    QCOMPARE(m_view->m_splitter->sizes().mid(0, 3), before);

    // What the splitter is held at has to cover every column, the filler and every handle it
    // draws. A pixel short and it takes that pixel back out of a column, so no column ends up
    // with less than the width it asked for.
    auto *splitter = m_view->m_splitter;
    QVERIFY(splitter->minimumWidth() > 0); // The columns overflow, so it is held open.
    const int viewportWidth = m_view->m_scrollArea->viewport()->width();
    const QList<int> applied = splitter->sizes();
    for (int i = 0; i < m_view->columnCount(); ++i) {
        const int asked = qMin(viewportWidth, qMax(settings->minColumnWidth(), m_view->columnAt(i)->calculateOptimalWidth()));
        QVERIFY2(applied.at(i) == asked, qPrintable(QStringLiteral("column %1 asked for %2 and was given %3").arg(i).arg(asked).arg(applied.at(i))));
    }
}

void DolphinColumnsViewTest::testTheFirstVisibleColumnIsShownWhole()
{
    // When the columns do not fit, the view scrolls to the left edge of a column. A column cut on
    // its left shows its selection without the name of the folder it leads to.
    auto *settings = ColumnsModeSettings::self();
    settings->setDynamicColumnWidth(true);
    settings->setMinColumnWidth(200);
    m_testDir->createDir("alpha/alpha-child/deep");

    m_view->resize(500, 400);
    QTRY_VERIFY_WITH_TIMEOUT(m_view->m_scrollArea->viewport()->width() > 0, 5000);

    auto noColumnIsCut = [this]() {
        for (int i = 0; i < m_view->columnCount(); ++i) {
            const int left = m_view->columnAt(i)->mapTo(m_view->m_scrollArea->viewport(), QPoint(0, 0)).x();
            if (left < 0 && left + m_view->columnAt(i)->width() > 0) {
                return false;
            }
        }
        return true;
    };

    m_view->openChild(0, urlOf(QStringLiteral("alpha")));
    QTRY_VERIFY_WITH_TIMEOUT(m_view->m_splitter->sizes().at(1) > 0, 5000);
    m_view->openChild(1, urlOf(QStringLiteral("alpha/alpha-child")));
    QTRY_VERIFY_WITH_TIMEOUT(m_view->m_splitter->sizes().at(2) > 0, 5000);
    m_view->openChild(2, urlOf(QStringLiteral("alpha/alpha-child/deep")));
    QTRY_VERIFY_WITH_TIMEOUT(m_view->m_splitter->sizes().at(3) > 0, 5000);
    m_view->setActiveColumn(3);

    QTRY_VERIFY_WITH_TIMEOUT(m_view->m_scrollArea->horizontalScrollBar()->value() > 0, 5000);
    QTRY_VERIFY_WITH_TIMEOUT(noColumnIsCut(), 5000);
}

void DolphinColumnsViewTest::testWideningTheWindowShowsTheParentColumns()
{
    // A wider window shows the parent columns again instead of empty room after the last column.
    auto *settings = ColumnsModeSettings::self();
    settings->setDynamicColumnWidth(true);
    settings->setMinColumnWidth(200);
    m_testDir->createDir("alpha/alpha-child/deep");

    m_view->resize(500, 400);
    QTRY_VERIFY_WITH_TIMEOUT(m_view->m_scrollArea->viewport()->width() > 0, 5000);

    m_view->openChild(0, urlOf(QStringLiteral("alpha")));
    QTRY_VERIFY_WITH_TIMEOUT(m_view->m_splitter->sizes().at(1) > 0, 5000);
    m_view->openChild(1, urlOf(QStringLiteral("alpha/alpha-child")));
    QTRY_VERIFY_WITH_TIMEOUT(m_view->m_splitter->sizes().at(2) > 0, 5000);
    m_view->openChild(2, urlOf(QStringLiteral("alpha/alpha-child/deep")));
    QTRY_VERIFY_WITH_TIMEOUT(m_view->m_splitter->sizes().at(3) > 0, 5000);
    m_view->setActiveColumn(3);

    QScrollBar *scrollBar = m_view->m_scrollArea->horizontalScrollBar();
    QTRY_VERIFY_WITH_TIMEOUT(scrollBar->value() > 0, 5000);
    const int narrowScrollValue = scrollBar->value();

    // Dragging the window edge widens it a few pixels at a time.
    for (int width = 510; width <= 750; width += 10) {
        m_view->resize(width, 400);
        QCoreApplication::processEvents();
    }

    QWidget *viewport = m_view->m_scrollArea->viewport();
    auto lastRight = [this, viewport]() {
        QWidget *last = m_view->columnAt(m_view->columnCount() - 1);
        return last->mapTo(viewport, QPoint(last->width(), 0)).x();
    };
    QTRY_VERIFY_WITH_TIMEOUT(scrollBar->value() < narrowScrollValue, 5000);
    QTRY_VERIFY_WITH_TIMEOUT(lastRight() <= viewport->width(), 5000);
    // No whole column fits in the room left after the last one.
    QTRY_VERIFY_WITH_TIMEOUT(scrollBar->value() == 0 || viewport->width() - lastRight() < m_view->columnAt(0)->width(), 5000);
    for (int i = 0; i < m_view->columnCount(); ++i) {
        const int left = m_view->columnAt(i)->mapTo(viewport, QPoint(0, 0)).x();
        QVERIFY(left >= 0 || left + m_view->columnAt(i)->width() <= 0);
    }
}

void DolphinColumnsViewTest::testALongNameDoesNotPushOtherColumnsOut()
{
    // A column that fits its content stops at its share of the viewport, so the maximum number of
    // visible columns still fits when one of them holds a long name.
    auto *settings = ColumnsModeSettings::self();
    settings->setDynamicColumnWidth(true);
    settings->setMinColumnWidth(100);
    settings->setMaxVisibleColumns(4);
    m_testDir->createDir("alpha/alpha-child/deep");
    m_testDir->createFile(QStringLiteral("alpha/") + QString(80, QLatin1Char('x')) + QStringLiteral(".txt"));

    m_view->resize(1000, 400);
    QTRY_VERIFY_WITH_TIMEOUT(m_view->m_scrollArea->viewport()->width() > 0, 5000);

    m_view->openChild(0, urlOf(QStringLiteral("alpha")));
    QTRY_VERIFY_WITH_TIMEOUT(m_view->m_splitter->sizes().at(1) > 0, 5000);
    m_view->openChild(1, urlOf(QStringLiteral("alpha/alpha-child")));
    QTRY_VERIFY_WITH_TIMEOUT(m_view->m_splitter->sizes().at(2) > 0, 5000);
    m_view->openChild(2, urlOf(QStringLiteral("alpha/alpha-child/deep")));
    QTRY_VERIFY_WITH_TIMEOUT(m_view->m_splitter->sizes().at(3) > 0, 5000);

    const int viewportWidth = m_view->m_scrollArea->viewport()->width();
    QVERIFY(m_view->columnAt(1)->width() <= viewportWidth / 4);
    QVERIFY(m_view->columnAt(1)->width() < m_view->columnAt(1)->calculateOptimalWidth());
    QCOMPARE(m_view->m_scrollArea->horizontalScrollBar()->value(), 0);
    QWidget *last = m_view->columnAt(3);
    QVERIFY(last->mapTo(m_view->m_scrollArea->viewport(), QPoint(last->width(), 0)).x() <= viewportWidth);
}

void DolphinColumnsViewTest::testChangingTheUrlLeavesTheFocusOutsideTheView()
{
    // A cd in the terminal panel sets the url of the view, and the terminal keeps the focus.
    QWidget host;
    auto *layout = new QVBoxLayout(&host);
    auto *terminal = new QLineEdit(&host);
    layout->addWidget(terminal);
    layout->addWidget(m_view);
    host.resize(800, 500);
    host.show();
    QVERIFY(QTest::qWaitForWindowActive(&host));
    terminal->setFocus();
    QTRY_VERIFY_WITH_TIMEOUT(terminal->hasFocus(), 5000);

    // A folder below the open columns.
    m_view->setUrl(urlOf(QStringLiteral("alpha/alpha-child")));
    QTRY_COMPARE_WITH_TIMEOUT(m_view->activePane()->dirUrl().adjusted(QUrl::StripTrailingSlash), urlOf(QStringLiteral("alpha/alpha-child")), 5000);
    QVERIFY(terminal->hasFocus());

    // A folder in no open column, for which the columns are rebuilt.
    TestDir otherDir;
    m_view->setUrl(otherDir.url());
    QTRY_COMPARE_WITH_TIMEOUT(m_view->activePane()->dirUrl().adjusted(QUrl::StripTrailingSlash), otherDir.url().adjusted(QUrl::StripTrailingSlash), 5000);
    QVERIFY(terminal->hasFocus());

    // With the focus in a column, the new active column takes it.
    m_view->activePane()->container()->setFocus();
    m_view->setUrl(urlOf(QStringLiteral("alpha")));
    QTRY_COMPARE_WITH_TIMEOUT(m_view->activePane()->dirUrl().adjusted(QUrl::StripTrailingSlash), urlOf(QStringLiteral("alpha")), 5000);
    QTRY_VERIFY_WITH_TIMEOUT(m_view->activePane()->container()->hasFocus(), 5000);

    m_view->setParent(nullptr);
}

void DolphinColumnsViewTest::testColumnIsNeverWiderThanTheViewport()
{
    // A column that fits its content stops at the width of the viewport, so scrolling to it always
    // reaches its right edge, where its vertical scrollbar is.
    auto *settings = ColumnsModeSettings::self();
    settings->setDynamicColumnWidth(true);
    settings->setMinColumnWidth(10);

    // A name far longer than the window is wide.
    m_testDir->createFile(QString(200, QLatin1Char('x')) + QStringLiteral(".txt"));
    m_view->resize(400, 400);

    auto *scrollArea = m_view->findChild<QScrollArea *>();
    QVERIFY(scrollArea);
    auto *splitter = m_view->findChild<QSplitter *>();
    QVERIFY(splitter);

    activateColumn(0);
    QTRY_VERIFY_WITH_TIMEOUT(m_view->columnAt(0)->calculateOptimalWidth() > scrollArea->viewport()->width(), 5000);
    m_view->recalculateColumnWidths();

    const int columnWidth = splitter->sizes().at(0);
    const int viewportWidth = scrollArea->viewport()->width();
    QVERIFY2(columnWidth <= viewportWidth, qPrintable(QStringLiteral("column (%1) should fit the viewport (%2)").arg(columnWidth).arg(viewportWidth)));
}

QTEST_MAIN(DolphinColumnsViewTest)

#include "dolphincolumnsviewtest.moc"
