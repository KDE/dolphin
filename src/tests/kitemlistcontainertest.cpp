/*
 * SPDX-FileCopyrightText: 2026 Méven Car <meven@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kitemviews/kitemlistcontainer.h"
#include "kitemviews/kfileitemlistview.h"
#include "kitemviews/kfileitemmodel.h"
#include "kitemviews/kitemlistcontroller.h"

#include "testdir.h"
#include <KDirLister>

#include <QApplication>
#include <QElapsedTimer>
#include <QProxyStyle>
#include <QScrollBar>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTest>

/**
 * A style which says animations last long enough for a test to look at one while it runs. The
 * smooth scroller asks its scroll bar's style how long to animate for, and a style which answers
 * zero leaves nothing to observe.
 */
static constexpr int animationDuration = 5000;

class SlowAnimationStyle : public QProxyStyle
{
    Q_OBJECT
public:
    int styleHint(StyleHint hint, const QStyleOption *option, const QWidget *widget, QStyleHintReturn *returnData) const override
    {
        if (hint == SH_Widget_Animation_Duration) {
            return animationDuration;
        }
        return QProxyStyle::styleHint(hint, option, widget, returnData);
    }
};

class KItemListContainerTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void testScrollBarCatchesUpWithAViewWhichGrewWhileScrolling();
};

void KItemListContainerTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
}

void KItemListContainerTest::testScrollBarCatchesUpWithAViewWhichGrewWhileScrolling()
{
    // The scroll bars are left alone while a scroller is animating, so that a view on its way to
    // an item is not stopped short of it. What the view has become still has to reach them: a
    // directory which gains files while the view is scrolling has further to scroll than the
    // scroll bar says, and the last of it cannot be reached until something else happens to ask
    // for an update.
    QApplication::setStyle(new SlowAnimationStyle);

    TestDir testDir;
    QStringList files;
    for (int i = 0; i < 200; ++i) {
        files << QStringLiteral("file%1").arg(i, 3, 10, QLatin1Char('0'));
    }
    testDir.createFiles(files);

    // The controller takes both of these over: it reparents them and deletes them with itself.
    auto *model = new KFileItemModel();
    auto *view = new KFileItemListView();
    auto *controller = new KItemListController(model, view, this);
    KItemListContainer container(controller);
#ifndef QT_NO_ACCESSIBILITY
    view->setAccessibleParentsObject(&container);
#endif
    container.resize(400, 200);

    QSignalSpy loaded(model, &KFileItemModel::directoryLoadingCompleted);
    model->loadDirectory(testDir.url());
    QVERIFY(loaded.wait());

    container.show();
    if (!QTest::qWaitForWindowExposed(&container)) {
        QSKIP("The window was never exposed, so there is nothing to scroll.");
    }

    QScrollBar *scrollBar = container.verticalScrollBar();
    QTRY_VERIFY(scrollBar->maximum() > 0);
    const int maximumBefore = scrollBar->maximum();

    // The check below is only worth making while the scrolling is still going on, and how long the
    // listing takes is not this test's to decide.
    QElapsedTimer sinceScrollingStarted;
    sinceScrollingStarted.start();

    // Start scrolling, and while that is under way give the directory more files to hold.
    view->scrollToItem(190);

    QStringList moreFiles;
    for (int i = 200; i < 400; ++i) {
        moreFiles << QStringLiteral("file%1").arg(i, 3, 10, QLatin1Char('0'));
    }
    testDir.createFiles(moreFiles);

    // The way a watched directory tells the view it has grown: the lister notices and hands over
    // the new items, which the model adds to the ones it already has. Reloading the whole
    // directory, which is what F5 does, is not what happens when files simply turn up.
    QSignalSpy inserted(model, &KFileItemModel::itemsInserted);
    model->m_dirLister->updateDirectory(testDir.url());
    QVERIFY(inserted.wait());
    QCOMPARE(model->count(), 400);

    // While the scrolling is under way the scroll bar is left as it was on purpose, so that a view
    // on its way to an item is not stopped short of it.
    if (sinceScrollingStarted.elapsed() < animationDuration - 1000) {
        QCOMPARE(scrollBar->maximum(), maximumBefore);
    }

    // Once the scrolling is over it reaches as far as the view now does, without anything else
    // having to come along and ask.
    QTRY_VERIFY_WITH_TIMEOUT(scrollBar->maximum() > maximumBefore, 10000);
    QCOMPARE(scrollBar->maximum(), qMax(0, int(view->maximumScrollOffset() - view->size().height())));
}

QTEST_MAIN(KItemListContainerTest)

#include "kitemlistcontainertest.moc"
