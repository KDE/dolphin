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

#include <QScopedPointer>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTest>

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
    m_mainWindow->openDirectories({ QUrl::fromLocalFile(QDir::homePath()) }, false);
    m_mainWindow->show();
    // Without this call the searchbox doesn't get FocusIn events.
    QVERIFY(QTest::qWaitForWindowExposed(m_mainWindow.data()));
    QVERIFY(m_mainWindow->isVisible());

    auto tabWidget = m_mainWindow->findChild<DolphinTabWidget*>("tabWidget");
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
    m_mainWindow->openDirectories({ QUrl::fromLocalFile(QDir::homePath()) }, false);
    m_mainWindow->show();
    QVERIFY(QTest::qWaitForWindowExposed(m_mainWindow.data()));
    QVERIFY(m_mainWindow->isVisible());

    auto tabWidget = m_mainWindow->findChild<DolphinTabWidget*>("tabWidget");
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
    m_mainWindow->openDirectories({ QUrl::fromLocalFile(QDir::homePath()) }, false);
    m_mainWindow->show();
    QVERIFY(QTest::qWaitForWindowExposed(m_mainWindow.data()));
    QVERIFY(m_mainWindow->isVisible());

    auto tabWidget = m_mainWindow->findChild<DolphinTabWidget*>("tabWidget");
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
    m_mainWindow->openDirectories({ QUrl::fromLocalFile(QDir::homePath()) }, false);
    m_mainWindow->show();
    QVERIFY(QTest::qWaitForWindowExposed(m_mainWindow.data()));
    QVERIFY(m_mainWindow->isVisible());

    auto tabWidget = m_mainWindow->findChild<DolphinTabWidget*>("tabWidget");
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
    m_mainWindow->openDirectories({ QUrl::fromLocalFile(QDir::homePath()) }, false);
    m_mainWindow->show();
    QVERIFY(QTest::qWaitForWindowExposed(m_mainWindow.data()));
    QVERIFY(m_mainWindow->isVisible());

    auto tabWidget = m_mainWindow->findChild<DolphinTabWidget*>("tabWidget");
    QVERIFY(tabWidget);

    tabWidget->openNewTab(QUrl::fromLocalFile(QDir::tempPath()));
    QCOMPARE(tabWidget->count(), 2);
    QVERIFY(tabWidget->tabText(0) != tabWidget->tabText(1));
    if (!tabWidget->tabIcon(0).isNull() && !tabWidget->tabIcon(1).isNull()) {
        QCOMPARE(QStringLiteral("inode-directory"), tabWidget->tabIcon(0).name());
        QCOMPARE(QStringLiteral("inode-directory"), tabWidget->tabIcon(1).name());
    }
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
    m_mainWindow->openDirectories({ activeViewUrl }, false);
    m_mainWindow->show();
    QVERIFY(QTest::qWaitForWindowExposed(m_mainWindow.data()));
    QVERIFY(m_mainWindow->isVisible());

    auto newFileMenu = m_mainWindow->findChild<DolphinNewFileMenu*>("new_menu");
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
    m_mainWindow->openDirectories({ activeViewUrl }, false);
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
    m_mainWindow->openDirectories({ QUrl::fromLocalFile(QDir::homePath()) }, false);
    m_mainWindow->show();
    QVERIFY(QTest::qWaitForWindowExposed(m_mainWindow.data()));
    QVERIFY(m_mainWindow->isVisible());

    QWidget *placesPanel = reinterpret_cast<QWidget *>(m_mainWindow->m_placesPanel);
    QVERIFY2(QTest::qWaitFor([&](){ return placesPanel && placesPanel->isVisible() && placesPanel->width() > 0; }, 5000), "The test couldn't be initialised properly. The places panel should be visible.");
    QTest::qWait(100);
    const int initialPlacesPanelWidth = placesPanel->width();

    m_mainWindow->actionCollection()->action(QStringLiteral("split_view"))->trigger(); // enable split view (starts animation)
    QTest::qWait(300); // wait for animation
    QCOMPARE(placesPanel->width(), initialPlacesPanelWidth);

    m_mainWindow->actionCollection()->action(QStringLiteral("show_filter_bar"))->trigger();
    QCOMPARE(placesPanel->width(), initialPlacesPanelWidth);

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
    testDir->createDir("c");
    QUrl childDirUrl(QDir::cleanPath(testDir->url().toString() + "/b"));
    m_mainWindow->openDirectories({ childDirUrl }, false); // Open "b" dir
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
    QVERIFY(QTest::qWaitFor([&](){ return !m_mainWindow->actionCollection()->action(QStringLiteral("stop"))->isEnabled(); })); // "Stop" command should be disabled because it finished loading
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
