/***************************************************************************
 *   Copyright (C) 2017 by Elvis Angelaccio <elvis.angelaccio@kde.org>     *
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

#include "dolphinmainwindow.h"
#include "dolphinnewfilemenu.h"
#include "dolphintabpage.h"
#include "dolphintabwidget.h"
#include "dolphinviewcontainer.h"

#include <KActionCollection>

#include <QSignalSpy>
#include <QStandardPaths>
#include <QTest>

class DolphinMainWindowTest : public QObject
{
    Q_OBJECT

private slots:
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
        QVERIFY(tabWidget->tabIcon(0).name() != tabWidget->tabIcon(1).name());
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

    auto newFileMenu = m_mainWindow->findChild<DolphinNewFileMenu*>("newFileMenu");
    QVERIFY(newFileMenu);

    QFETCH(bool, expectedEnabled);
    QTRY_COMPARE(newFileMenu->isEnabled(), expectedEnabled);
}

QTEST_MAIN(DolphinMainWindowTest)

#include "dolphinmainwindowtest.moc"
