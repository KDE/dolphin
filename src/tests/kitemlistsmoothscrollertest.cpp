/*
 * SPDX-FileCopyrightText: 2025 Méven Car <meven@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kitemviews/private/kitemlistsmoothscroller.h"

#include <QScrollBar>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTest>

class KItemListSmoothScrollerTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void testScrollToCurrentPositionEmitsScrollingStopped();
    void testScrollToClampedPositionEmitsScrollingStopped();
};

void KItemListSmoothScrollerTest::initTestCase()
{
    QStandardPaths::setTestModeEnabled(true);
}

void KItemListSmoothScrollerTest::testScrollToCurrentPositionEmitsScrollingStopped()
{
    // KItemListView::scrollToItem() delegates to KItemListSmoothScroller::scrollTo()
    // for any non-zero offset and then relies on scrollingStopped() to know when the
    // view settled. Callers such as DolphinView::renameSelectedItems() only open the
    // inline rename editor once scrollingStopped() arrives. If scrollTo() is asked to
    // move to the position the scrollbar is already at, no animation runs, so it must
    // still emit scrollingStopped() or the editor never appears.
    QScrollBar scrollBar(Qt::Vertical);
    scrollBar.setRange(0, 100);
    scrollBar.setValue(50);

    KItemListSmoothScroller scroller(&scrollBar);
    QSignalSpy spy(&scroller, &KItemListSmoothScroller::scrollingStopped);

    scroller.scrollTo(scrollBar.value());

    QCOMPARE(spy.count(), 1);
}

void KItemListSmoothScrollerTest::testScrollToClampedPositionEmitsScrollingStopped()
{
    // Reproduces renaming an item at the very bottom: the scrollbar is already at its
    // maximum, scrollToItem() still computes a non-zero offset and requests a further
    // scroll, but the clamped target equals the current value, so nothing moves.
    QScrollBar scrollBar(Qt::Vertical);
    scrollBar.setRange(0, 100);
    scrollBar.setValue(scrollBar.maximum());

    KItemListSmoothScroller scroller(&scrollBar);
    QSignalSpy spy(&scroller, &KItemListSmoothScroller::scrollingStopped);

    scroller.scrollTo(scrollBar.maximum() + 50);

    QCOMPARE(spy.count(), 1);
}

QTEST_MAIN(KItemListSmoothScrollerTest)

#include "kitemlistsmoothscrollertest.moc"
