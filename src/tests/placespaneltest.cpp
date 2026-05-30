/*
 * SPDX-FileCopyrightText: 2026 Sebastian Englbrecht
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "panels/places/placespanel.h"

#include <KFilePlacesModel>
#include <KLocalizedString>

#include <QApplication>
#include <QContextMenuEvent>
#include <QItemSelectionModel>
#include <QMenu>
#include <QSignalSpy>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QTest>
#include <QTimer>

// Select the given index and open context menu via keyboard event.
static void triggerContextMenuForIndex(KFilePlacesView &view, const QModelIndex &index)
{
    view.selectionModel()->setCurrentIndex(index, QItemSelectionModel::ClearAndSelect);
    QContextMenuEvent event(QContextMenuEvent::Keyboard, QPoint(), QPoint());
    QApplication::sendEvent(&view, &event);
}

// Open context menu with no item selected (empty-area behaviour).
static void triggerContextMenuEmptyArea(KFilePlacesView &view)
{
    view.selectionModel()->clearCurrentIndex();
    view.selectionModel()->clearSelection();
    QContextMenuEvent event(QContextMenuEvent::Keyboard, QPoint(), QPoint());
    QApplication::sendEvent(&view, &event);
}

// Collect visible action texts from the menu (top-level, non-separator, visible only).
static QStringList visibleMenuActionTexts(QMenu *menu)
{
    QStringList texts;
    for (QAction *action : menu->actions()) {
        if (!action->isSeparator() && action->isVisible()) {
            texts << action->text();
        }
    }
    return texts;
}

class PlacesPanelTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();

    void testOpenInSplitViewVisibleForPlace();
    void testOpenInSplitViewHiddenForEmptyArea();
    void testOpenInSplitViewEmitsSignal();
    void testConfigureTrashVisibleForTrash();
    void testConfigureTrashHiddenForOtherPlaces();
    void testCustomContextMenuActionsInEmptyArea();
    void testCustomContextMenuActionsHiddenForItems();
    void testCustomContextMenuActionsRoundTrip();

private:
    // Find the first model index whose URL matches the given scheme.
    QModelIndex indexForScheme(KFilePlacesModel *model, const QString &scheme) const;

    QTemporaryDir m_tmpHome;
};

void PlacesPanelTest::initTestCase()
{
    QVERIFY(m_tmpHome.isValid());
    qputenv("HOME", m_tmpHome.path().toUtf8());
    QStandardPaths::setTestModeEnabled(true);
}

void PlacesPanelTest::cleanupTestCase()
{
    const QString bookmarks = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation) + u"/user-places.xbel";
    QFile::remove(bookmarks);
}

QModelIndex PlacesPanelTest::indexForScheme(KFilePlacesModel *model, const QString &scheme) const
{
    for (int row = 0; row < model->rowCount(); ++row) {
        const QModelIndex idx = model->index(row, 0);
        if (model->url(idx).scheme() == scheme) {
            return idx;
        }
    }
    return {};
}

void PlacesPanelTest::testOpenInSplitViewVisibleForPlace()
{
    PlacesPanel panel(nullptr);
    auto *model = qobject_cast<KFilePlacesModel *>(panel.model());
    QVERIFY(model);

    // Home is the first place and always has a valid URL.
    const QModelIndex homeIndex = model->index(0, 0);
    QVERIFY(homeIndex.isValid());
    QVERIFY(model->url(homeIndex).isValid());

    QStringList visibleActions;
    connect(
        &panel,
        &KFilePlacesView::contextMenuAboutToShow,
        this,
        [&visibleActions](const QModelIndex &, QMenu *menu) {
            QTimer::singleShot(0, menu, [menu, &visibleActions]() {
                visibleActions = visibleMenuActionTexts(menu);
                menu->close();
            });
        },
        Qt::SingleShotConnection);

    triggerContextMenuForIndex(panel, homeIndex);

    QVERIFY(visibleActions.contains(i18nc("@action:inmenu", "Open in Split View")));
}

void PlacesPanelTest::testOpenInSplitViewHiddenForEmptyArea()
{
    PlacesPanel panel(nullptr);

    QStringList visibleActions;
    connect(
        &panel,
        &KFilePlacesView::contextMenuAboutToShow,
        this,
        [&visibleActions](const QModelIndex &, QMenu *menu) {
            QTimer::singleShot(0, menu, [menu, &visibleActions]() {
                visibleActions = visibleMenuActionTexts(menu);
                menu->close();
            });
        },
        Qt::SingleShotConnection);

    triggerContextMenuEmptyArea(panel);

    QVERIFY(!visibleActions.contains(i18nc("@action:inmenu", "Open in Split View")));
}

void PlacesPanelTest::testOpenInSplitViewEmitsSignal()
{
    PlacesPanel panel(nullptr);
    auto *model = qobject_cast<KFilePlacesModel *>(panel.model());
    QVERIFY(model);

    const QModelIndex homeIndex = model->index(0, 0);
    QVERIFY(homeIndex.isValid());
    const QUrl homeUrl = model->url(homeIndex);

    QSignalSpy splitSpy(&panel, &PlacesPanel::openInSplitViewRequested);

    connect(
        &panel,
        &KFilePlacesView::contextMenuAboutToShow,
        this,
        [](const QModelIndex &, QMenu *menu) {
            QTimer::singleShot(0, menu, [menu]() {
                for (QAction *action : menu->actions()) {
                    if (action->isVisible() && action->text() == i18nc("@action:inmenu", "Open in Split View")) {
                        menu->setActiveAction(action);
                        QTest::keyClick(menu, Qt::Key_Return);
                        return;
                    }
                }
                menu->close();
            });
        },
        Qt::SingleShotConnection);

    triggerContextMenuForIndex(panel, homeIndex);

    QCOMPARE(splitSpy.count(), 1);
    QCOMPARE(splitSpy.first().first().toUrl(), homeUrl);
}

void PlacesPanelTest::testConfigureTrashVisibleForTrash()
{
    PlacesPanel panel(nullptr);
    auto *model = qobject_cast<KFilePlacesModel *>(panel.model());
    QVERIFY(model);

    const QModelIndex trashIndex = indexForScheme(model, QStringLiteral("trash"));
    if (!trashIndex.isValid()) {
        QSKIP("No trash place in model");
    }

    QStringList visibleActions;
    connect(
        &panel,
        &KFilePlacesView::contextMenuAboutToShow,
        this,
        [&visibleActions](const QModelIndex &, QMenu *menu) {
            QTimer::singleShot(0, menu, [menu, &visibleActions]() {
                visibleActions = visibleMenuActionTexts(menu);
                menu->close();
            });
        },
        Qt::SingleShotConnection);

    triggerContextMenuForIndex(panel, trashIndex);

    QVERIFY(visibleActions.contains(i18nc("@action:inmenu", "Configure Trash…")));
}

void PlacesPanelTest::testConfigureTrashHiddenForOtherPlaces()
{
    PlacesPanel panel(nullptr);
    auto *model = qobject_cast<KFilePlacesModel *>(panel.model());
    QVERIFY(model);

    const QModelIndex homeIndex = model->index(0, 0);
    QVERIFY(homeIndex.isValid());
    QCOMPARE(model->url(homeIndex).scheme(), QStringLiteral("file"));

    QStringList visibleActions;
    connect(
        &panel,
        &KFilePlacesView::contextMenuAboutToShow,
        this,
        [&visibleActions](const QModelIndex &, QMenu *menu) {
            QTimer::singleShot(0, menu, [menu, &visibleActions]() {
                visibleActions = visibleMenuActionTexts(menu);
                menu->close();
            });
        },
        Qt::SingleShotConnection);

    triggerContextMenuForIndex(panel, homeIndex);

    QVERIFY(!visibleActions.contains(i18nc("@action:inmenu", "Configure Trash…")));
}

void PlacesPanelTest::testCustomContextMenuActionsInEmptyArea()
{
    PlacesPanel panel(nullptr);

    QAction customAction(QStringLiteral("TestCustomAction"));
    panel.setCustomContextMenuActions({&customAction});

    QStringList allActions;
    connect(
        &panel,
        &KFilePlacesView::contextMenuAboutToShow,
        this,
        [&allActions](const QModelIndex &, QMenu *menu) {
            QTimer::singleShot(0, menu, [menu, &allActions]() {
                for (QAction *action : menu->actions()) {
                    if (!action->isSeparator()) {
                        allActions << action->text();
                    }
                }
                menu->close();
            });
        },
        Qt::SingleShotConnection);

    triggerContextMenuEmptyArea(panel);

    QVERIFY(allActions.contains(QStringLiteral("TestCustomAction")));
}

void PlacesPanelTest::testCustomContextMenuActionsHiddenForItems()
{
    PlacesPanel panel(nullptr);
    auto *model = qobject_cast<KFilePlacesModel *>(panel.model());
    QVERIFY(model);

    QAction customAction(QStringLiteral("TestCustomActionForItem"));
    panel.setCustomContextMenuActions({&customAction});

    const QModelIndex homeIndex = model->index(0, 0);
    QVERIFY(homeIndex.isValid());

    QStringList allActions;
    connect(
        &panel,
        &KFilePlacesView::contextMenuAboutToShow,
        this,
        [&allActions](const QModelIndex &, QMenu *menu) {
            QTimer::singleShot(0, menu, [menu, &allActions]() {
                for (QAction *action : menu->actions()) {
                    if (!action->isSeparator()) {
                        allActions << action->text();
                    }
                }
                menu->close();
            });
        },
        Qt::SingleShotConnection);

    triggerContextMenuForIndex(panel, homeIndex);

    QVERIFY(!allActions.contains(QStringLiteral("TestCustomActionForItem")));
}

void PlacesPanelTest::testCustomContextMenuActionsRoundTrip()
{
    PlacesPanel panel(nullptr);
    auto *model = qobject_cast<KFilePlacesModel *>(panel.model());
    QVERIFY(model);

    QAction customAction(QStringLiteral("TestRoundTripAction"));
    panel.setCustomContextMenuActions({&customAction});

    const QModelIndex homeIndex = model->index(0, 0);
    QVERIFY(homeIndex.isValid());

    // First: open item menu — custom action must not appear.
    QStringList itemMenuActions;
    connect(
        &panel,
        &KFilePlacesView::contextMenuAboutToShow,
        this,
        [&itemMenuActions](const QModelIndex &, QMenu *menu) {
            QTimer::singleShot(0, menu, [menu, &itemMenuActions]() {
                for (QAction *action : menu->actions()) {
                    if (!action->isSeparator()) {
                        itemMenuActions << action->text();
                    }
                }
                menu->close();
            });
        },
        Qt::SingleShotConnection);
    triggerContextMenuForIndex(panel, homeIndex);

    QVERIFY(!itemMenuActions.contains(QStringLiteral("TestRoundTripAction")));

    // Second: open empty-area menu — custom action must be re-added.
    QStringList emptyAreaActions;
    connect(
        &panel,
        &KFilePlacesView::contextMenuAboutToShow,
        this,
        [&emptyAreaActions](const QModelIndex &, QMenu *menu) {
            QTimer::singleShot(0, menu, [menu, &emptyAreaActions]() {
                for (QAction *action : menu->actions()) {
                    if (!action->isSeparator()) {
                        emptyAreaActions << action->text();
                    }
                }
                menu->close();
            });
        },
        Qt::SingleShotConnection);
    triggerContextMenuEmptyArea(panel);

    QVERIFY(emptyAreaActions.contains(QStringLiteral("TestRoundTripAction")));
}

QTEST_MAIN(PlacesPanelTest)

#include "placespaneltest.moc"
