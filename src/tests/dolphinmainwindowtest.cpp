/*
 * SPDX-FileCopyrightText: 2017 Elvis Angelaccio <elvis.angelaccio@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "dolphinmainwindow.h"
#include "dolphinnewfilemenu.h"
#include "dolphintabpage.h"
#include "dolphintabwidget.h"
#include "dolphinviewcontainer.h"
#include "kitemviews/kitemlistcontainer.h"
#include "testdir.h"

#include <KActionCollection>
#include <KConfig>
#include <KConfigGui>

#include <QAccessible>
#include <QFileSystemWatcher>
#include <QScopedPointer>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTest>

#include <set>

class DolphinMainWindowTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void init();
    void testClosingTabsWithSearchBoxVisible();
    void testActiveViewAfterClosingSplitView_data();
    void testActiveViewAfterClosingSplitView();
    void testUpdateWindowTitleAfterClosingSplitView();
    void testUpdateWindowTitleAfterChangingSplitView();
    void testOpenInNewTabTitle();
    void testNewFileMenuEnabled_data();
    void testNewFileMenuEnabled();
    void testWindowTitle_data();
    void testWindowTitle();
    void testPlacesPanelWidthResistance();
    void testGoActions();
    void testOpenFiles();
    void testAccessibilityAncestorTree();
    void testAutoSaveSession();
    void cleanupTestCase();

private:
    QScopedPointer<DolphinMainWindow> m_mainWindow;
};

void DolphinMainWindowTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
}

void DolphinMainWindowTest::init()
{
    m_mainWindow.reset(new DolphinMainWindow());
}

// See https://bugs.kde.org/show_bug.cgi?id=379135
void DolphinMainWindowTest::testClosingTabsWithSearchBoxVisible()
{
    m_mainWindow->openDirectories({QUrl::fromLocalFile(QDir::homePath())}, false);
    m_mainWindow->show();
    // Without this call the searchbox doesn't get FocusIn events.
    QVERIFY(QTest::qWaitForWindowExposed(m_mainWindow.data()));
    QVERIFY(m_mainWindow->isVisible());

    auto tabWidget = m_mainWindow->findChild<DolphinTabWidget *>("tabWidget");
    QVERIFY(tabWidget);

    // Show search box on first tab.
    tabWidget->currentTabPage()->activeViewContainer()->setSearchModeEnabled(true);

    tabWidget->openNewActivatedTab(QUrl::fromLocalFile(QDir::homePath()));
    QCOMPARE(tabWidget->count(), 2);

    // Triggers the crash in bug #379135.
    tabWidget->closeTab();
    QCOMPARE(tabWidget->count(), 1);
}

void DolphinMainWindowTest::testActiveViewAfterClosingSplitView_data()
{
    QTest::addColumn<bool>("closeLeftView");

    QTest::newRow("close left view") << true;
    QTest::newRow("close right view") << false;
}

void DolphinMainWindowTest::testActiveViewAfterClosingSplitView()
{
    m_mainWindow->openDirectories({QUrl::fromLocalFile(QDir::homePath())}, false);
    m_mainWindow->show();
    QVERIFY(QTest::qWaitForWindowExposed(m_mainWindow.data()));
    QVERIFY(m_mainWindow->isVisible());

    auto tabWidget = m_mainWindow->findChild<DolphinTabWidget *>("tabWidget");
    QVERIFY(tabWidget);
    QVERIFY(tabWidget->currentTabPage()->primaryViewContainer());
    QVERIFY(!tabWidget->currentTabPage()->secondaryViewContainer());

    // Open split view.
    m_mainWindow->actionCollection()->action(QStringLiteral("split_view"))->trigger();
    QVERIFY(tabWidget->currentTabPage()->splitViewEnabled());
    QVERIFY(tabWidget->currentTabPage()->secondaryViewContainer());

    // Make sure the right view is the active one.
    auto leftViewContainer = tabWidget->currentTabPage()->primaryViewContainer();
    auto rightViewContainer = tabWidget->currentTabPage()->secondaryViewContainer();
    QVERIFY(!leftViewContainer->isActive());
    QVERIFY(rightViewContainer->isActive());

    QFETCH(bool, closeLeftView);
    if (closeLeftView) {
        // Activate left view.
        leftViewContainer->setActive(true);
        QVERIFY(leftViewContainer->isActive());
        QVERIFY(!rightViewContainer->isActive());

        // Close left view. The secondary view (which was on the right) will become the primary one and must be active.
        m_mainWindow->actionCollection()->action(QStringLiteral("split_view"))->trigger();
        QVERIFY(!leftViewContainer->isActive());
        QVERIFY(rightViewContainer->isActive());
        QCOMPARE(rightViewContainer, tabWidget->currentTabPage()->activeViewContainer());
    } else {
        // Close right view. The left view will become active.
        m_mainWindow->actionCollection()->action(QStringLiteral("split_view"))->trigger();
        QVERIFY(leftViewContainer->isActive());
        QVERIFY(!rightViewContainer->isActive());
        QCOMPARE(leftViewContainer, tabWidget->currentTabPage()->activeViewContainer());
    }
}

// Test case for bug #385111
void DolphinMainWindowTest::testUpdateWindowTitleAfterClosingSplitView()
{
    m_mainWindow->openDirectories({QUrl::fromLocalFile(QDir::homePath())}, false);
    m_mainWindow->show();
    QVERIFY(QTest::qWaitForWindowExposed(m_mainWindow.data()));
    QVERIFY(m_mainWindow->isVisible());

    auto tabWidget = m_mainWindow->findChild<DolphinTabWidget *>("tabWidget");
    QVERIFY(tabWidget);
    QVERIFY(tabWidget->currentTabPage()->primaryViewContainer());
    QVERIFY(!tabWidget->currentTabPage()->secondaryViewContainer());

    // Open split view.
    m_mainWindow->actionCollection()->action(QStringLiteral("split_view"))->trigger();
    QVERIFY(tabWidget->currentTabPage()->splitViewEnabled());
    QVERIFY(tabWidget->currentTabPage()->secondaryViewContainer());

    // Make sure the right view is the active one.
    auto leftViewContainer = tabWidget->currentTabPage()->primaryViewContainer();
    auto rightViewContainer = tabWidget->currentTabPage()->secondaryViewContainer();
    QVERIFY(!leftViewContainer->isActive());
    QVERIFY(rightViewContainer->isActive());

    // Activate left view.
    leftViewContainer->setActive(true);
    QVERIFY(leftViewContainer->isActive());
    QVERIFY(!rightViewContainer->isActive());

    // Close split view. The secondary view (which was on the right) will become the primary one and must be active.
    m_mainWindow->actionCollection()->action(QStringLiteral("split_view"))->trigger();
    QVERIFY(!leftViewContainer->isActive());
    QVERIFY(rightViewContainer->isActive());
    QCOMPARE(rightViewContainer, tabWidget->currentTabPage()->activeViewContainer());

    // Change URL and make sure we emit the currentUrlChanged signal (which triggers the window title update).
    QSignalSpy currentUrlChangedSpy(tabWidget, &DolphinTabWidget::currentUrlChanged);
    tabWidget->currentTabPage()->activeViewContainer()->setUrl(QUrl::fromLocalFile(QDir::rootPath()));
    QCOMPARE(currentUrlChangedSpy.count(), 1);
}

// Test case for bug #402641
void DolphinMainWindowTest::testUpdateWindowTitleAfterChangingSplitView()
{
    m_mainWindow->openDirectories({QUrl::fromLocalFile(QDir::homePath())}, false);
    m_mainWindow->show();
    QVERIFY(QTest::qWaitForWindowExposed(m_mainWindow.data()));
    QVERIFY(m_mainWindow->isVisible());

    auto tabWidget = m_mainWindow->findChild<DolphinTabWidget *>("tabWidget");
    QVERIFY(tabWidget);

    // Open split view.
    m_mainWindow->actionCollection()->action(QStringLiteral("split_view"))->trigger();
    QVERIFY(tabWidget->currentTabPage()->splitViewEnabled());

    auto leftViewContainer = tabWidget->currentTabPage()->primaryViewContainer();
    auto rightViewContainer = tabWidget->currentTabPage()->secondaryViewContainer();

    // Store old window title.
    const auto oldTitle = m_mainWindow->windowTitle();

    // Change URL in the right view and make sure the title gets updated.
    rightViewContainer->setUrl(QUrl::fromLocalFile(QDir::rootPath()));
    QVERIFY(m_mainWindow->windowTitle() != oldTitle);

    // Activate back the left view and check whether the old title gets restored.
    leftViewContainer->setActive(true);
    QCOMPARE(m_mainWindow->windowTitle(), oldTitle);
}

// Test case for bug #397910
void DolphinMainWindowTest::testOpenInNewTabTitle()
{
    const QUrl homePathUrl{QUrl::fromLocalFile(QDir::homePath())};
    m_mainWindow->openDirectories({homePathUrl}, false);
    m_mainWindow->show();
    QVERIFY(QTest::qWaitForWindowExposed(m_mainWindow.data()));
    QVERIFY(m_mainWindow->isVisible());

    auto tabWidget = m_mainWindow->findChild<DolphinTabWidget *>("tabWidget");
    QVERIFY(tabWidget);

    const QUrl tempPathUrl{QUrl::fromLocalFile(QDir::tempPath())};
    tabWidget->openNewTab(tempPathUrl);
    QCOMPARE(tabWidget->count(), 2);
    QVERIFY(tabWidget->tabText(0) != tabWidget->tabText(1));

    QVERIFY2(!tabWidget->tabIcon(0).isNull() && !tabWidget->tabIcon(1).isNull(), "Tabs are supposed to have icons.");
    QCOMPARE(KIO::iconNameForUrl(homePathUrl), tabWidget->tabIcon(0).name());
    QCOMPARE(KIO::iconNameForUrl(tempPathUrl), tabWidget->tabIcon(1).name());
}

void DolphinMainWindowTest::testNewFileMenuEnabled_data()
{
    QTest::addColumn<QUrl>("activeViewUrl");
    QTest::addColumn<bool>("expectedEnabled");

    QTest::newRow("home") << QUrl::fromLocalFile(QDir::homePath()) << true;
    QTest::newRow("root") << QUrl::fromLocalFile(QDir::rootPath()) << false;
    QTest::newRow("trash") << QUrl::fromUserInput(QStringLiteral("trash:/")) << false;
}

void DolphinMainWindowTest::testNewFileMenuEnabled()
{
    QFETCH(QUrl, activeViewUrl);
    m_mainWindow->openDirectories({activeViewUrl}, false);
    m_mainWindow->show();
    QVERIFY(QTest::qWaitForWindowExposed(m_mainWindow.data()));
    QVERIFY(m_mainWindow->isVisible());

    auto newFileMenu = m_mainWindow->findChild<DolphinNewFileMenu *>("new_menu");
    QVERIFY(newFileMenu);

    QFETCH(bool, expectedEnabled);
    QTRY_COMPARE(newFileMenu->isEnabled(), expectedEnabled);
}

void DolphinMainWindowTest::testWindowTitle_data()
{
    QTest::addColumn<QUrl>("activeViewUrl");
    QTest::addColumn<QString>("expectedWindowTitle");

    // TODO: this test should enforce the english locale.
    QTest::newRow("home") << QUrl::fromLocalFile(QDir::homePath()) << QStringLiteral("Home");
    QTest::newRow("home with trailing slash") << QUrl::fromLocalFile(QStringLiteral("%1/").arg(QDir::homePath())) << QStringLiteral("Home");
    QTest::newRow("trash") << QUrl::fromUserInput(QStringLiteral("trash:/")) << QStringLiteral("Trash");
}

void DolphinMainWindowTest::testWindowTitle()
{
    QFETCH(QUrl, activeViewUrl);
    m_mainWindow->openDirectories({activeViewUrl}, false);
    m_mainWindow->show();
    QVERIFY(QTest::qWaitForWindowExposed(m_mainWindow.data()));
    QVERIFY(m_mainWindow->isVisible());

    QFETCH(QString, expectedWindowTitle);
    QCOMPARE(m_mainWindow->windowTitle(), expectedWindowTitle);
}

/**
 * The places panel will resize itself if any of the other widgets requires too much horizontal space
 * but a user never wants the size of the places panel to change unless they resized it themselves explicitly.
 */
void DolphinMainWindowTest::testPlacesPanelWidthResistance()
{
    m_mainWindow->openDirectories({QUrl::fromLocalFile(QDir::homePath())}, false);
    m_mainWindow->show();
    m_mainWindow->resize(800, m_mainWindow->height()); // make sure the size is sufficient so a places panel resize shouldn't be necessary.
    QVERIFY(QTest::qWaitForWindowExposed(m_mainWindow.data()));
    QVERIFY(m_mainWindow->isVisible());

    QWidget *placesPanel = reinterpret_cast<QWidget *>(m_mainWindow->m_placesPanel);
    QVERIFY2(QTest::qWaitFor(
                 [&]() {
                     return placesPanel && placesPanel->isVisible() && placesPanel->width() > 0;
                 },
                 5000),
             "The test couldn't be initialised properly. The places panel should be visible.");
    QTest::qWait(100);
    const int initialPlacesPanelWidth = placesPanel->width();

    m_mainWindow->actionCollection()->action(QStringLiteral("split_view"))->trigger(); // enable split view (starts animation)
    QTest::qWait(300); // wait for animation
    QCOMPARE(placesPanel->width(), initialPlacesPanelWidth);

    m_mainWindow->actionCollection()->action(QStringLiteral("show_filter_bar"))->trigger();
    QCOMPARE(placesPanel->width(), initialPlacesPanelWidth);

    // Make all selection mode bars appear and test for each that this doesn't affect the places panel's width.
    // One of the bottom bars (SelectionMode::BottomBar::GeneralContents) only shows up when at least one item is selected so we do that before we begin iterating.
    m_mainWindow->actionCollection()->action(KStandardAction::name(KStandardAction::SelectAll))->trigger();
    for (int selectionModeStates = SelectionMode::BottomBar::CopyContents; selectionModeStates != SelectionMode::BottomBar::RenameContents;
         selectionModeStates++) {
        const auto contents = static_cast<SelectionMode::BottomBar::Contents>(selectionModeStates);
        m_mainWindow->slotSetSelectionMode(true, contents);
        QTest::qWait(20); // give time for a paint/resize
        QCOMPARE(placesPanel->width(), initialPlacesPanelWidth);
    }

    m_mainWindow->actionCollection()->action(KStandardAction::name(KStandardAction::Find))->trigger();
    QCOMPARE(placesPanel->width(), initialPlacesPanelWidth);

#if HAVE_BALOO
    m_mainWindow->actionCollection()->action(QStringLiteral("show_information_panel"))->setChecked(true); // toggle visible
    QCOMPARE(placesPanel->width(), initialPlacesPanelWidth);
#endif

#if HAVE_TERMINAL
    m_mainWindow->actionCollection()->action(QStringLiteral("show_terminal_panel"))->setChecked(true); // toggle visible
    QCOMPARE(placesPanel->width(), initialPlacesPanelWidth);
#endif

    m_mainWindow->actionCollection()->action(QStringLiteral("split_view"))->trigger(); // disable split view (starts animation)
    QCOMPARE(placesPanel->width(), initialPlacesPanelWidth);

#if HAVE_BALOO
    m_mainWindow->actionCollection()->action(QStringLiteral("show_information_panel"))->trigger(); // toggle invisible
    QCOMPARE(placesPanel->width(), initialPlacesPanelWidth);
#endif

#if HAVE_TERMINAL
    m_mainWindow->actionCollection()->action(QStringLiteral("show_terminal_panel"))->trigger(); // toggle invisible
    QCOMPARE(placesPanel->width(), initialPlacesPanelWidth);
#endif

    m_mainWindow->showMaximized();
    QCOMPARE(placesPanel->width(), initialPlacesPanelWidth);

    QTest::qWait(300); // wait for split view closing animation
    QCOMPARE(placesPanel->width(), initialPlacesPanelWidth);
}

void DolphinMainWindowTest::testGoActions()
{
    QScopedPointer<TestDir> testDir{new TestDir()};
    testDir->createDir("a");
    testDir->createDir("b");
    testDir->createDir("b/b-1");
    testDir->createFile("b/b-2");
    testDir->createDir("c");
    QUrl childDirUrl(QDir::cleanPath(testDir->url().toString() + "/b"));
    m_mainWindow->openDirectories({childDirUrl}, false); // Open "b" dir
    m_mainWindow->show();
    QVERIFY(QTest::qWaitForWindowExposed(m_mainWindow.data()));
    QVERIFY(m_mainWindow->isVisible());
    QVERIFY(!m_mainWindow->actionCollection()->action(KStandardAction::name(KStandardAction::Forward))->isEnabled());

    m_mainWindow->actionCollection()->action(KStandardAction::name(KStandardAction::Up))->trigger();
    /**
     * Now, after going "up" in the file hierarchy (to "testDir"), the folder one has emerged from ("b") should have keyboard focus.
     * This is especially important when a user wants to peek into multiple folders in quick succession.
     */
    QSignalSpy spyDirectoryLoadingCompleted(m_mainWindow->m_activeViewContainer->view(), &DolphinView::directoryLoadingCompleted);
    QVERIFY(spyDirectoryLoadingCompleted.wait());
    QVERIFY(QTest::qWaitFor([&]() {
        return !m_mainWindow->actionCollection()->action(QStringLiteral("stop"))->isEnabled();
    })); // "Stop" command should be disabled because it finished loading
    QTest::qWait(500); // Somehow the item we emerged from doesn't have keyboard focus yet if we don't wait a split second.
    const QUrl parentDirUrl = m_mainWindow->activeViewContainer()->url();
    QVERIFY(parentDirUrl != childDirUrl);

    // The item we just emerged from should now have keyboard focus but this doesn't necessarily mean that it is selected.
    // To test if it has keyboard focus, we press "Down" to select "c" below and then "Up" so the folder "b" we just emerged from is actually selected.
    m_mainWindow->actionCollection()->action(QStringLiteral("compact"))->trigger();
    QTest::keyClick(m_mainWindow->activeViewContainer()->view()->m_container, Qt::Key::Key_Down, Qt::NoModifier);
    QCOMPARE(m_mainWindow->m_activeViewContainer->view()->selectedItems().count(), 1);
    QTest::keyClick(m_mainWindow->activeViewContainer()->view()->m_container, Qt::Key::Key_Up, Qt::NoModifier);
    QCOMPARE(m_mainWindow->m_activeViewContainer->view()->selectedItems().count(), 1);
    QTest::keyClick(m_mainWindow->activeViewContainer()->view()->m_container, Qt::Key::Key_Enter, Qt::NoModifier);
    QVERIFY(spyDirectoryLoadingCompleted.wait());
    QCOMPARE(m_mainWindow->activeViewContainer()->url(), childDirUrl);
    QVERIFY(m_mainWindow->isUrlOpen(childDirUrl.toString()));

    // Go back to the parent folder
    m_mainWindow->actionCollection()->action(KStandardAction::name(KStandardAction::Back))->trigger();
    QVERIFY(spyDirectoryLoadingCompleted.wait());
    QTest::qWait(100); // Somehow the item we emerged from doesn't have keyboard focus yet if we don't wait a split second.
    QCOMPARE(m_mainWindow->activeViewContainer()->url(), parentDirUrl);
    QVERIFY(m_mainWindow->isUrlOpen(parentDirUrl.toString()));

    // Open a new tab for the "b" child dir and verify that this doesn't interfere with anything.
    QTest::keyClick(m_mainWindow->activeViewContainer()->view()->m_container, Qt::Key::Key_Enter, Qt::ControlModifier); // Open new inactive tab
    QVERIFY(m_mainWindow->m_tabWidget->count() == 2);
    QCOMPARE(m_mainWindow->activeViewContainer()->url(), parentDirUrl);
    QVERIFY(m_mainWindow->isUrlOpen(parentDirUrl.toString()));
    QVERIFY(!m_mainWindow->actionCollection()->action(QStringLiteral("undo_close_tab"))->isEnabled());

    // Go forward to the child folder.
    m_mainWindow->actionCollection()->action(KStandardAction::name(KStandardAction::Forward))->trigger();
    QVERIFY(spyDirectoryLoadingCompleted.wait());
    QCOMPARE(m_mainWindow->activeViewContainer()->url(), childDirUrl);
    QCOMPARE(m_mainWindow->m_activeViewContainer->view()->selectedItems().count(), 0); // There was no action in this view yet that would warrant a selection.

    // Go back to the parent folder.
    m_mainWindow->actionCollection()->action(KStandardAction::name(KStandardAction::Back))->trigger();
    QVERIFY(spyDirectoryLoadingCompleted.wait());
    QTest::qWait(100); // Somehow the item we emerged from doesn't have keyboard focus yet if we don't wait a split second.
    QCOMPARE(m_mainWindow->activeViewContainer()->url(), parentDirUrl);
    QVERIFY(m_mainWindow->isUrlOpen(parentDirUrl.toString()));

    // Close current tab and see if the "go" actions are correctly disabled in the remaining tab that was never active until now and shows the "b" dir
    m_mainWindow->actionCollection()->action(KStandardAction::name(KStandardAction::Close))->trigger(); // Close current tab
    QVERIFY(m_mainWindow->m_tabWidget->count() == 1);
    QCOMPARE(m_mainWindow->activeViewContainer()->url(), childDirUrl);
    QCOMPARE(m_mainWindow->m_activeViewContainer->view()->selectedItems().count(), 0); // There was no action in this tab yet that would warrant a selection.
    QVERIFY(!m_mainWindow->actionCollection()->action(KStandardAction::name(KStandardAction::Back))->isEnabled());
    QVERIFY(!m_mainWindow->actionCollection()->action(KStandardAction::name(KStandardAction::Forward))->isEnabled());
    QVERIFY(m_mainWindow->actionCollection()->action(QStringLiteral("undo_close_tab"))->isEnabled());
}

void DolphinMainWindowTest::testOpenFiles()
{
    QScopedPointer<TestDir> testDir{new TestDir()};
    QString testDirUrl(QDir::cleanPath(testDir->url().toString()));
    testDir->createDir("a");
    testDir->createDir("a/b");
    testDir->createDir("a/b/c");
    testDir->createDir("a/b/c/d");
    m_mainWindow->openDirectories({testDirUrl}, false);
    m_mainWindow->show();

    // We only see the unselected "a" folder in the test dir. There are no other tabs.
    QVERIFY(m_mainWindow->isUrlOpen(testDirUrl));
    QVERIFY(m_mainWindow->isItemVisibleInAnyView(testDirUrl + "/a"));
    QVERIFY(!m_mainWindow->isUrlOpen(testDirUrl + "/a"));
    QVERIFY(!m_mainWindow->isItemVisibleInAnyView(testDirUrl + "/a/b"));
    QCOMPARE(m_mainWindow->m_tabWidget->count(), 1);
    QCOMPARE(m_mainWindow->m_tabWidget->currentIndex(), 0);
    QCOMPARE(m_mainWindow->m_activeViewContainer->view()->selectedItems().count(), 0);

    // "a" is already in view, so "opening" "a" should simply select it without opening a new tab.
    m_mainWindow->openFiles({testDirUrl + "/a"}, false);
    QTRY_COMPARE(m_mainWindow->m_activeViewContainer->view()->selectedItems().count(), 1);
    QCOMPARE(m_mainWindow->m_tabWidget->count(), 1);
    QVERIFY(m_mainWindow->isItemVisibleInAnyView(testDirUrl + "/a"));

    // "b" is not in view, so "opening" "b" should open a new active tab of the parent folder "a" and select "b" there.
    m_mainWindow->openFiles({testDirUrl + "/a/b"}, false);
    QTRY_VERIFY(m_mainWindow->isUrlOpen(testDirUrl + "/a"));
    QCOMPARE(m_mainWindow->m_tabWidget->count(), 2);
    QCOMPARE(m_mainWindow->m_tabWidget->currentIndex(), 1);
    QTRY_VERIFY(m_mainWindow->isItemVisibleInAnyView(testDirUrl + "/a/b"));
    QVERIFY2(!m_mainWindow->isUrlOpen(testDirUrl + "/a/b"), "The directory b is supposed to be visible but not open in its own tab.");
    QTRY_COMPARE(m_mainWindow->m_activeViewContainer->view()->selectedItems().count(), 1);

    QVERIFY(m_mainWindow->isUrlOpen(testDirUrl));
    QVERIFY(m_mainWindow->isItemVisibleInAnyView(testDirUrl + "/a"));
    // "a" is still in view in the first tab, so "opening" "a" should switch to the first tab and select "a" there.
    m_mainWindow->openFiles({testDirUrl + "/a"}, false);
    QCOMPARE(m_mainWindow->m_tabWidget->count(), 2);
    QCOMPARE(m_mainWindow->m_tabWidget->currentIndex(), 0);
    QVERIFY(m_mainWindow->isUrlOpen(testDirUrl));
    QVERIFY(m_mainWindow->isUrlOpen(testDirUrl + "/a"));

    // Directory "a" is already open in the second tab in which "b" is selected, so opening the directory "a" should switch to that tab.
    m_mainWindow->openDirectories({testDirUrl + "/a"}, false);
    QCOMPARE(m_mainWindow->m_tabWidget->count(), 2);
    QCOMPARE(m_mainWindow->m_tabWidget->currentIndex(), 1);

    // In the details view mode directories can be expanded, which changes if openFiles() needs to open a new tab or not to open a file.
    m_mainWindow->actionCollection()->action(QStringLiteral("details"))->trigger();
    QTRY_VERIFY(m_mainWindow->activeViewContainer()->view()->itemsExpandable());

    // Expand the already selected "b" with the right arrow key. This should make "c" visible.
    QVERIFY2(!m_mainWindow->isItemVisibleInAnyView(testDirUrl + "/a/b/c"), "The parent folder wasn't expanded yet, so c shouldn't be visible.");
    QTest::keyClick(m_mainWindow->activeViewContainer()->view()->m_container, Qt::Key::Key_Right);
    QTRY_VERIFY(m_mainWindow->isItemVisibleInAnyView(testDirUrl + "/a/b/c"));
    QVERIFY2(!m_mainWindow->isUrlOpen(testDirUrl + "/a/b"), "b is supposed to be expanded, however it shouldn't be open in its own tab.");
    QVERIFY(m_mainWindow->isUrlOpen(testDirUrl + "/a"));

    // Switch to first tab by opening it even though it is already open.
    m_mainWindow->openDirectories({testDirUrl}, false);
    QCOMPARE(m_mainWindow->m_tabWidget->count(), 2);
    QCOMPARE(m_mainWindow->m_tabWidget->currentIndex(), 0);

    // "c" is in view in the second tab because "b" is expanded there, so "opening" "c" should switch to that tab and select "c" there.
    m_mainWindow->openFiles({testDirUrl + "/a/b/c"}, false);
    QCOMPARE(m_mainWindow->m_tabWidget->count(), 2);
    QCOMPARE(m_mainWindow->m_tabWidget->currentIndex(), 1);
    QTRY_COMPARE(m_mainWindow->m_activeViewContainer->view()->selectedItems().count(), 1);
    QVERIFY(m_mainWindow->isUrlOpen(testDirUrl));
    QVERIFY(m_mainWindow->isUrlOpen(testDirUrl + "/a"));

    // Opening the directory "c" on the other hand will open it in a new tab even though it is already visible in the view
    // because openDirecories() and openFiles() serve different purposes. One opens views at urls, the other selects files within views.
    m_mainWindow->openDirectories({testDirUrl + "/a/b/c/d", testDirUrl + "/a/b/c"}, true);
    QCOMPARE(m_mainWindow->m_tabWidget->count(), 3);
    QCOMPARE(m_mainWindow->m_tabWidget->currentIndex(), 2);
    QVERIFY(m_mainWindow->m_tabWidget->currentTabPage()->splitViewEnabled());
    QVERIFY(m_mainWindow->isItemVisibleInAnyView(testDirUrl + "/a/b/c")); // It should still be visible in the second tab.
    QTRY_COMPARE(m_mainWindow->m_activeViewContainer->view()->selectedItems().count(), 0);
    QVERIFY(m_mainWindow->isUrlOpen(testDirUrl + "/a/b/c/d"));
    QVERIFY(m_mainWindow->isUrlOpen(testDirUrl + "/a/b/c"));

    // "c" is in view in the second tab because "b" is expanded there,
    // so "opening" "c" should switch to that tab even though "c" as a directory is open in the current tab.
    m_mainWindow->openFiles({testDirUrl + "/a/b/c"}, false);
    QCOMPARE(m_mainWindow->m_tabWidget->count(), 3);
    QCOMPARE(m_mainWindow->m_tabWidget->currentIndex(), 1);
    QVERIFY2(m_mainWindow->isItemVisibleInAnyView(testDirUrl + "/a/b/c/d"), "It should be visible in the secondary view of the third tab.");

    // Select "b" and un-expand it with the left arrow key. This should make "c" invisible.
    m_mainWindow->openFiles({testDirUrl + "/a/b"}, false);
    QTest::keyClick(m_mainWindow->activeViewContainer()->view()->m_container, Qt::Key::Key_Left);
    QTRY_VERIFY(!m_mainWindow->isItemVisibleInAnyView(testDirUrl + "/a/b/c"));

    // "d" is in view in the third tab in the secondary view, so "opening" "d" should select that view.
    m_mainWindow->openFiles({testDirUrl + "/a/b/c/d"}, false);
    QCOMPARE(m_mainWindow->m_tabWidget->count(), 3);
    QCOMPARE(m_mainWindow->m_tabWidget->currentIndex(), 2);
    QVERIFY(m_mainWindow->m_tabWidget->currentTabPage()->secondaryViewContainer()->isActive());
    QTRY_COMPARE(m_mainWindow->m_activeViewContainer->view()->selectedItems().count(), 1);
}

void DolphinMainWindowTest::testAccessibilityAncestorTree()
{
    m_mainWindow->openDirectories({QUrl::fromLocalFile(QDir::homePath())}, false);
    m_mainWindow->show();
    QVERIFY(QTest::qWaitForWindowExposed(m_mainWindow.data()));
    QVERIFY(m_mainWindow->isVisible());

    QAccessibleInterface *accessibleInterfaceOfMainWindow = QAccessible::queryAccessibleInterface(m_mainWindow.get());
    Q_CHECK_PTR(accessibleInterfaceOfMainWindow);

    // We will test the accessibility of objects traversing forwards and backwards.
    int testedObjectsSizeAfterTraversingForwards = 0;
    for (int i = 0; i < 2; i++) {
        std::tuple<Qt::Key, Qt::KeyboardModifier> focusChainTraversalKeyCombination = {Qt::Key::Key_Tab, Qt::NoModifier};
        if (i) {
            focusChainTraversalKeyCombination = {Qt::Key::Key_Tab, Qt::ShiftModifier};
        }

        // We will do accessibility checks for every object that gets focus. Focus will be changed using the focusChainTraversalKeyCombination.
        std::set<const QObject *> testedObjects; // Makes sure we stop testing when we arrive at an item that was already tested.
        while (qApp->focusObject() && !testedObjects.count(qApp->focusObject())) {
            const auto currentlyFocusedObject = qApp->focusObject();

            QAccessibleInterface *accessibleInterface = QAccessible::queryAccessibleInterface(currentlyFocusedObject);
            // The accessibleInterfaces of focused objects might themselves have children.
            // We go down that hierarchy as far as possible and then test the ancestor tree from there.
            while (accessibleInterface->childCount() > 0) {
                accessibleInterface = accessibleInterface->child(0);
            }
            while (accessibleInterface != accessibleInterfaceOfMainWindow) {
                QVERIFY2(accessibleInterface,
                         qPrintable(QString("%1's accessibleInterface or one of its accessible children doesn't have the main window as an ancestor.")
                                        .arg(currentlyFocusedObject->metaObject()->className())));
                accessibleInterface = accessibleInterface->parent();
            }

            testedObjects.insert(currentlyFocusedObject); // Add it to testedObjects so we won't test it again later.
            QTest::keyClick(m_mainWindow.get(), std::get<0>(focusChainTraversalKeyCombination), std::get<1>(focusChainTraversalKeyCombination));
            QVERIFY2(currentlyFocusedObject != qApp->focusObject(),
                     "The focus chain is broken. The focused object should have changed after pressing the focusChainTraversalKeyCombination.");
        }

        if (i == 0) {
            testedObjectsSizeAfterTraversingForwards = testedObjects.size();
        } else {
            QCOMPARE(testedObjects.size(), testedObjectsSizeAfterTraversingForwards); // The size after traversing backwards is different than
                                                                                      // after going forwards which is probably not intended.
        }
    }
}

void DolphinMainWindowTest::testAutoSaveSession()
{
    m_mainWindow->openDirectories({QUrl::fromLocalFile(QDir::homePath())}, false);
    m_mainWindow->show();
    QVERIFY(QTest::qWaitForWindowExposed(m_mainWindow.data()));
    QVERIFY(m_mainWindow->isVisible());

    // Create config file
    KConfigGui::setSessionConfig(QStringLiteral("dolphin"), QStringLiteral("dolphin"));
    KConfig *config = KConfigGui::sessionConfig();
    m_mainWindow->saveGlobalProperties(config);
    m_mainWindow->savePropertiesInternal(config, 1);
    config->sync();

    // Setup watcher for config file changes
    const QString configFileName = QStandardPaths::writableLocation(QStandardPaths::GenericConfigLocation) + "/" + KConfigGui::sessionConfig()->name();
    QFileSystemWatcher *configWatcher = new QFileSystemWatcher({configFileName}, this);
    QSignalSpy spySessionSaved(configWatcher, &QFileSystemWatcher::fileChanged);

    // Enable session autosave.
    m_mainWindow->setSessionAutoSaveEnabled(true);
    m_mainWindow->m_sessionSaveTimer->setInterval(200); // Lower the interval to speed up the testing

    // Open a new tab
    auto tabWidget = m_mainWindow->findChild<DolphinTabWidget *>("tabWidget");
    QVERIFY(tabWidget);
    tabWidget->openNewActivatedTab(QUrl::fromLocalFile(QDir::tempPath()));
    QCOMPARE(tabWidget->count(), 2);

    // Wait till a session save occurs
    QVERIFY(spySessionSaved.wait(60000));

    // Disable session autosave.
    m_mainWindow->setSessionAutoSaveEnabled(false);
}

void DolphinMainWindowTest::cleanupTestCase()
{
    m_mainWindow->showNormal();
    m_mainWindow->actionCollection()->action(QStringLiteral("split_view"))->setChecked(false); // disable split view (starts animation)

#if HAVE_BALOO
    m_mainWindow->actionCollection()->action(QStringLiteral("show_information_panel"))->setChecked(false); // hide panel
#endif

#if HAVE_TERMINAL
    m_mainWindow->actionCollection()->action(QStringLiteral("show_terminal_panel"))->setChecked(false); // hide panel
#endif

    // Quit Dolphin to save the hiding of panels and make sure that normal Quit doesn't crash.
    m_mainWindow->actionCollection()->action(KStandardAction::name(KStandardAction::Quit))->trigger();
}

QTEST_MAIN(DolphinMainWindowTest)

#include "dolphinmainwindowtest.moc"
