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
#include "dolphintabpage.h"
#include "dolphintabwidget.h"
#include "dolphinviewcontainer.h"

#include <QTest>

class DolphinMainWindowTest : public QObject
{
    Q_OBJECT

private slots:
    void init();
    void testClosingTabsWithSearchBoxVisible();

private:
    QScopedPointer<DolphinMainWindow> m_mainWindow;
};

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

QTEST_MAIN(DolphinMainWindowTest)

#include "dolphinmainwindowtest.moc"
