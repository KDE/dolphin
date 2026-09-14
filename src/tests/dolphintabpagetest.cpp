/*
 * SPDX-FileCopyrightText: 2026 Méven Car <meven@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "dolphintabpage.h"
#include "dolphinviewcontainer.h"
#include "testdir.h"
#include "views/dolphinview.h"

#include <KConfig>
#include <KConfigGroup>

#include <QScopedPointer>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>

class DolphinTabPageTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void aStateCutShortIsNotRestored();
    void theStateOfALargeSelectionComesBackFromTheSessionFile();

private:
    DolphinTabPage *aTabPageShowing(const QUrl &url);
};

void DolphinTabPageTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
}

DolphinTabPage *DolphinTabPageTest::aTabPageShowing(const QUrl &url)
{
    auto *tabPage = new DolphinTabPage(url);
    tabPage->resize(400, 400);
    tabPage->show();
    return tabPage;
}

// A session file holds only so much, so a state can come back shorter than it was written. Every
// beginning of a state is restored without bringing the tab down with it.
void DolphinTabPageTest::aStateCutShortIsNotRestored()
{
    TestDir testDir;
    testDir.createFile(QStringLiteral("a.txt"));
    testDir.createFile(QStringLiteral("b.txt"));

    QScopedPointer<DolphinTabPage> tabPage(aTabPageShowing(testDir.url()));
    QTRY_COMPARE(tabPage->activeViewContainer()->view()->itemsCount(), 2);
    tabPage->activeViewContainer()->view()->selectAll();

    const QByteArray state = tabPage->saveState();
    QVERIFY(!state.isEmpty());

    for (qsizetype length = 0; length < state.size(); ++length) {
        QScopedPointer<DolphinTabPage> restored(aTabPageShowing(testDir.url()));
        restored->restoreState(state.left(length));
        QVERIFY(restored->activeViewContainer());
    }
}

// The selected items of a folder are as many as the folder holds, and a state carrying all of them
// grows past what the session file can give back. With all of a large folder selected, the state
// reads back from a session file as it was written.
void DolphinTabPageTest::theStateOfALargeSelectionComesBackFromTheSessionFile()
{
    TestDir testDir;
    QStringList files;
    const QString longName = QString(200, QLatin1Char('n'));
    for (int i = 0; i < 8000; ++i) {
        files.append(QStringLiteral("%1-%2.txt").arg(i).arg(longName));
    }
    testDir.createFiles(files);

    QScopedPointer<DolphinTabPage> tabPage(aTabPageShowing(testDir.url()));
    DolphinView *view = tabPage->activeViewContainer()->view();
    QTRY_COMPARE_WITH_TIMEOUT(view->itemsCount(), files.count(), 60000);
    view->selectAll();
    QCOMPARE(view->selectedItemsCount(), files.count());

    const QByteArray state = tabPage->saveState();

    // The state goes through a session file the way DolphinTabWidget writes it.
    QTemporaryDir configDir;
    QVERIFY(configDir.isValid());
    const QString sessionFile = configDir.filePath(QStringLiteral("session"));
    {
        KConfig config(sessionFile, KConfig::SimpleConfig);
        config.group(QStringLiteral("Tab 0")).writeEntry("State", state);
        QVERIFY(config.sync());
    }
    KConfig config(sessionFile, KConfig::SimpleConfig);
    QCOMPARE(config.group(QStringLiteral("Tab 0")).readEntry("State", QByteArray()), state);
}

QTEST_MAIN(DolphinTabPageTest)

#include "dolphintabpagetest.moc"
