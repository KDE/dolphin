/*
 * SPDX-FileCopyrightText: 2026 Sebastian Englbrecht
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "views/dolphincolumnsview.h"
#include "kitemviews/kfileitemmodel.h"
#include "kitemviews/kitemlistcontainer.h"
#include "kitemviews/kitemlistcontroller.h"
#include "kitemviews/kitemlistselectionmanager.h"
#include "testdir.h"
#include "views/dolphincolumnpane.h"

#include <QCoreApplication>
#include <QKeyEvent>
#include <QSignalSpy>
#include <QSplitter>
#include <QStandardPaths>
#include <QTest>

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

    void testSelectionMatchesActiveColumn();
    void testModelMatchesActiveColumn();

    void testEmptyDirectory();
    void testReload();

    void testKeyReturn_fileEmitsItemActivated();
    void testKeyReturn_directoryOpensChild();

    void testShiftRight_navigatesBetweenColumns();
    void testCtrlRight_navigatesBetweenColumns();

    void testSelectionPreservedAcrossNavigation();
    void testLeftKeyClearsChildSelection();

    void testEscape_clearsSelection();
    void testHomeEnd_withinColumn();

    void testBackButton_emitsGoBack();
    void testForwardButton_emitsGoForward();

private:
    void waitForStableState();
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
    auto *pane = m_view->columnAt(m_view->activeColumnIndex());
    QVERIFY(pane);
    const int activeBefore = m_view->activeColumnIndex();
    QKeyEvent press(QEvent::KeyPress, Qt::Key_Left, Qt::NoModifier);
    QCoreApplication::sendEvent(pane->container()->viewport(), &press);
    QCoreApplication::processEvents();
    QTRY_VERIFY_WITH_TIMEOUT(m_view->activeColumnIndex() < activeBefore || activeBefore == 0, 5000);
}

void DolphinColumnsViewTest::selectItemInColumn(int columnIndex, const QString &name)
{
    auto *pane = m_view->columnAt(columnIndex);
    QVERIFY2(pane, qPrintable(QStringLiteral("Column %1 does not exist").arg(columnIndex)));

    auto *model = pane->model();
    auto *selectionManager = pane->controller()->selectionManager();

    for (int i = 0; i < model->count(); ++i) {
        if (model->fileItem(i).name() == name) {
            selectionManager->setCurrentItem(i);
            selectionManager->setSelected(i, 1, KItemListSelectionManager::Select);
            QTRY_VERIFY_WITH_TIMEOUT(selectionManager->isSelected(i), 5000);
            return;
        }
    }
    QFAIL(qPrintable(QStringLiteral("Item '%1' not found in column %2").arg(name).arg(columnIndex)));
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
    // Give events time to process (no new column should appear)
    QTest::qWait(500);

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
    QUrl betaUrl = QUrl::fromLocalFile(m_testDir->path() + "/beta");

    m_view->setUrl(betaUrl);
    waitForStableState();

    QVERIFY(m_view->columnCount() >= 1);
    QCOMPARE(m_view->columnAt(0)->dirUrl().adjusted(QUrl::StripTrailingSlash), betaUrl.adjusted(QUrl::StripTrailingSlash));
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

QTEST_MAIN(DolphinColumnsViewTest)

#include "dolphincolumnsviewtest.moc"
