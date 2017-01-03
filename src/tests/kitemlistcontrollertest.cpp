/***************************************************************************
 *   Copyright (C) 2012 by Frank Reininghaus <frank78ac@googlemail.com>    *
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

#include "kitemviews/kitemlistcontainer.h"
#include "kitemviews/kfileitemlistview.h"
#include "kitemviews/kfileitemmodel.h"
#include "kitemviews/kitemlistcontroller.h"
#include "kitemviews/kitemlistselectionmanager.h"
#include "kitemviews/private/kitemlistviewlayouter.h"
#include "testdir.h"

#include <QTest>
#include <QGraphicsSceneMouseEvent>
#include <QSignalSpy>
#include <QProxyStyle>

/**
 * \class KItemListControllerTestStyle is a proxy style for testing the
 * KItemListController with different style hint options, e.g. single/double
 * click activation.
 */
class KItemListControllerTestStyle : public QProxyStyle
{
    Q_OBJECT
public:
    KItemListControllerTestStyle(QStyle* style) :
        QProxyStyle(style),
        m_activateItemOnSingleClick((bool)style->styleHint(SH_ItemView_ActivateItemOnSingleClick))
    {
    }

    void setActivateItemOnSingleClick(bool activateItemOnSingleClick)
    {
        m_activateItemOnSingleClick = activateItemOnSingleClick;
    }

    bool activateItemOnSingleClick() const
    {
        return m_activateItemOnSingleClick;
    }

    int styleHint(StyleHint hint,
                  const QStyleOption* option = nullptr,
                  const QWidget* widget = nullptr,
                  QStyleHintReturn* returnData = nullptr) const Q_DECL_OVERRIDE
    {
        switch (hint) {
        case QStyle::SH_ItemView_ActivateItemOnSingleClick:
            return (int)activateItemOnSingleClick();
        default:
            return QProxyStyle::styleHint(hint, option, widget, returnData);
        }
    }

private:
    bool m_activateItemOnSingleClick;
};

Q_DECLARE_METATYPE(KFileItemListView::ItemLayout)
Q_DECLARE_METATYPE(Qt::Orientation)
Q_DECLARE_METATYPE(KItemListController::SelectionBehavior)
Q_DECLARE_METATYPE(KItemSet)

class KItemListControllerTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void init();
    void cleanup();

    void testKeyboardNavigation_data();
    void testKeyboardNavigation();
    void testMouseClickActivation();

private:
    /**
     * Make sure that the number of columns in the view is equal to \a count
     * by changing the geometry of the container.
     */
    void adjustGeometryForColumnCount(int count);

private:
    KFileItemListView* m_view;
    KItemListController* m_controller;
    KItemListSelectionManager* m_selectionManager;
    KFileItemModel* m_model;
    TestDir* m_testDir;
    KItemListContainer* m_container;
    KItemListControllerTestStyle* m_testStyle;
};

/**
 * This function initializes the member objects, creates the temporary files, and
 * shows the view on the screen on startup. This could also be done before every
 * single test, but this would make the time needed to run the test much larger.
 */
void KItemListControllerTest::initTestCase()
{
    qRegisterMetaType<KItemSet>("KItemSet");

    m_testDir = new TestDir();
    m_model = new KFileItemModel();
    m_view = new KFileItemListView();
    m_controller = new KItemListController(m_model, m_view, this);
    m_container = new KItemListContainer(m_controller);
    m_controller = m_container->controller();
    m_controller->setSelectionBehavior(KItemListController::MultiSelection);
    m_selectionManager = m_controller->selectionManager();
    m_testStyle = new KItemListControllerTestStyle(m_view->style());
    m_view->setStyle(m_testStyle);

    QStringList files;
    files
        << "a1" << "a2" << "a3"
        << "b1"
        << "c1" << "c2" << "c3" << "c4" << "c5"
        << "d1" << "d2" << "d3" << "d4"
        << "e" << "e 2" << "e 3" << "e 4" << "e 5" << "e 6" << "e 7";

    m_testDir->createFiles(files);
    m_model->loadDirectory(m_testDir->url());
    QSignalSpy spyDirectoryLoadingCompleted(m_model, SIGNAL(directoryLoadingCompleted()));
    QVERIFY(spyDirectoryLoadingCompleted.wait());

    m_container->show();
    QTest::qWaitForWindowExposed(m_container);
}

void KItemListControllerTest::cleanupTestCase()
{
    delete m_container;
    m_container = 0;

    delete m_testDir;
    m_testDir = 0;
}

/** Before each test, the current item, selection, and item size are reset to the defaults. */
void KItemListControllerTest::init()
{
    m_selectionManager->setCurrentItem(0);
    QCOMPARE(m_selectionManager->currentItem(), 0);

    m_selectionManager->clearSelection();
    QVERIFY(!m_selectionManager->hasSelection());

    const QSizeF itemSize(50, 50);
    m_view->setItemSize(itemSize);
    QCOMPARE(m_view->itemSize(), itemSize);
}

void KItemListControllerTest::cleanup()
{
}

/**
 * \class KeyPress is a small helper struct that represents a key press event,
 * including the key and the keyboard modifiers.
 */
struct KeyPress {

    KeyPress(Qt::Key key, Qt::KeyboardModifiers modifier = Qt::NoModifier) :
        m_key(key),
        m_modifier(modifier)
    {}

    Qt::Key m_key;
    Qt::KeyboardModifiers m_modifier;
};

/**
 * \class ViewState is a small helper struct that represents a certain state
 * of the view, including the current item, the selected items in MultiSelection
 * mode (in the other modes, the selection is either empty or equal to the
 * current item), and the information whether items were activated by the last
 * key press.
 */
struct ViewState {

    ViewState(int current, const KItemSet selection, bool activated = false) :
        m_current(current),
        m_selection(selection),
        m_activated(activated)
    {}

    int m_current;
    KItemSet m_selection;
    bool m_activated;
};

// We have to define a typedef for the pair in order to make the test compile.
typedef QPair<KeyPress, ViewState> keyPressViewStatePair;
Q_DECLARE_METATYPE(QList<keyPressViewStatePair>)

/**
 * This function provides the data for the actual test function
 * KItemListControllerTest::testKeyboardNavigation().
 * It tests all possible combinations of view layouts, selection behaviors,
 * and enabled/disabled groupings for different column counts, and
 * provides a list of key presses and the states that the view should be in
 * after the key press event.
 */
void KItemListControllerTest::testKeyboardNavigation_data()
{
    QTest::addColumn<KFileItemListView::ItemLayout>("layout");
    QTest::addColumn<Qt::Orientation>("scrollOrientation");
    QTest::addColumn<int>("columnCount");
    QTest::addColumn<KItemListController::SelectionBehavior>("selectionBehavior");
    QTest::addColumn<bool>("groupingEnabled");
    QTest::addColumn<QList<QPair<KeyPress, ViewState> > >("testList");

    QList<KFileItemListView::ItemLayout> layoutList;
    QHash<KFileItemListView::ItemLayout, QString> layoutNames;
    layoutList.append(KFileItemListView::IconsLayout);
    layoutNames[KFileItemListView::IconsLayout] = "Icons";
    layoutList.append(KFileItemListView::CompactLayout);
    layoutNames[KFileItemListView::CompactLayout] = "Compact";
    layoutList.append(KFileItemListView::DetailsLayout);
    layoutNames[KFileItemListView::DetailsLayout] = "Details";

    QList<KItemListController::SelectionBehavior> selectionBehaviorList;
    QHash<KItemListController::SelectionBehavior, QString> selectionBehaviorNames;
    selectionBehaviorList.append(KItemListController::NoSelection);
    selectionBehaviorNames[KItemListController::NoSelection] = "NoSelection";
    selectionBehaviorList.append(KItemListController::SingleSelection);
    selectionBehaviorNames[KItemListController::SingleSelection] = "SingleSelection";
    selectionBehaviorList.append(KItemListController::MultiSelection);
    selectionBehaviorNames[KItemListController::MultiSelection] = "MultiSelection";

    QList<bool> groupingEnabledList;
    QHash<bool, QString> groupingEnabledNames;
    groupingEnabledList.append(false);
    groupingEnabledNames[false] = "ungrouped";
    groupingEnabledList.append(true);
    groupingEnabledNames[true] = "grouping enabled";

    foreach (const KFileItemListView::ItemLayout& layout, layoutList) {
        // The following settings depend on the layout.
        // Note that 'columns' are actually 'rows' in
        // Compact layout.
        Qt::Orientation scrollOrientation;
        QList<int> columnCountList;
        Qt::Key nextItemKey;
        Qt::Key previousItemKey;
        Qt::Key nextRowKey;
        Qt::Key previousRowKey;

        switch (layout) {
        case KFileItemListView::IconsLayout:
            scrollOrientation = Qt::Vertical;
            columnCountList << 1 << 3 << 5;
            nextItemKey = Qt::Key_Right;
            previousItemKey = Qt::Key_Left;
            nextRowKey = Qt::Key_Down;
            previousRowKey = Qt::Key_Up;
            break;
        case KFileItemListView::CompactLayout:
            scrollOrientation = Qt::Horizontal;
            columnCountList << 1 << 3 << 5;
            nextItemKey = Qt::Key_Down;
            previousItemKey = Qt::Key_Up;
            nextRowKey = Qt::Key_Right;
            previousRowKey = Qt::Key_Left;
            break;
        case KFileItemListView::DetailsLayout:
            scrollOrientation = Qt::Vertical;
            columnCountList << 1;
            nextItemKey = Qt::Key_Down;
            previousItemKey = Qt::Key_Up;
            nextRowKey = Qt::Key_Down;
            previousRowKey = Qt::Key_Up;
            break;
        }

        foreach (int columnCount, columnCountList) {
            foreach (const KItemListController::SelectionBehavior& selectionBehavior, selectionBehaviorList) {
                foreach (bool groupingEnabled, groupingEnabledList) { // krazy:exclude=foreach
                    QList<QPair<KeyPress, ViewState> > testList;

                    // First, key presses which should have the same effect
                    // for any layout and any number of columns.
                    testList
                        << qMakePair(KeyPress(nextItemKey), ViewState(1, KItemSet() << 1))
                        << qMakePair(KeyPress(Qt::Key_Return), ViewState(1, KItemSet() << 1, true))
                        << qMakePair(KeyPress(Qt::Key_Enter), ViewState(1, KItemSet() << 1, true))
                        << qMakePair(KeyPress(nextItemKey), ViewState(2, KItemSet() << 2))
                        << qMakePair(KeyPress(nextItemKey, Qt::ShiftModifier), ViewState(3, KItemSet() << 2 << 3))
                        << qMakePair(KeyPress(Qt::Key_Return), ViewState(3, KItemSet() << 2 << 3, true))
                        << qMakePair(KeyPress(previousItemKey, Qt::ShiftModifier), ViewState(2, KItemSet() << 2))
                        << qMakePair(KeyPress(nextItemKey, Qt::ShiftModifier), ViewState(3, KItemSet() << 2 << 3))
                        << qMakePair(KeyPress(nextItemKey, Qt::ControlModifier), ViewState(4, KItemSet() << 2 << 3))
                        << qMakePair(KeyPress(Qt::Key_Return), ViewState(4, KItemSet() << 2 << 3, true))
                        << qMakePair(KeyPress(previousItemKey), ViewState(3, KItemSet() << 3))
                        << qMakePair(KeyPress(Qt::Key_Home, Qt::ShiftModifier), ViewState(0, KItemSet() << 0 << 1 << 2 << 3))
                        << qMakePair(KeyPress(nextItemKey, Qt::ControlModifier), ViewState(1, KItemSet() << 0 << 1 << 2 << 3))
                        << qMakePair(KeyPress(Qt::Key_Space, Qt::ControlModifier), ViewState(1, KItemSet() << 0 << 2 << 3))
                        << qMakePair(KeyPress(Qt::Key_Space, Qt::ControlModifier), ViewState(1, KItemSet() << 0 << 1 << 2 << 3))
                        << qMakePair(KeyPress(Qt::Key_End), ViewState(19, KItemSet() << 19))
                        << qMakePair(KeyPress(previousItemKey, Qt::ShiftModifier), ViewState(18, KItemSet() << 18 << 19))
                        << qMakePair(KeyPress(Qt::Key_Home), ViewState(0, KItemSet() << 0))
                        << qMakePair(KeyPress(Qt::Key_Space, Qt::ControlModifier), ViewState(0, KItemSet()))
                        << qMakePair(KeyPress(Qt::Key_Enter), ViewState(0, KItemSet(), true))
                        << qMakePair(KeyPress(Qt::Key_Space, Qt::ControlModifier), ViewState(0, KItemSet() << 0))
                        << qMakePair(KeyPress(Qt::Key_Space, Qt::ControlModifier), ViewState(0, KItemSet()))
                        << qMakePair(KeyPress(Qt::Key_Space), ViewState(0, KItemSet() << 0))
                        << qMakePair(KeyPress(Qt::Key_E), ViewState(13, KItemSet() << 13))
                        << qMakePair(KeyPress(Qt::Key_Space), ViewState(14, KItemSet() << 14))
                        << qMakePair(KeyPress(Qt::Key_3), ViewState(15, KItemSet() << 15))
                        << qMakePair(KeyPress(Qt::Key_Home), ViewState(0, KItemSet() << 0))
                        << qMakePair(KeyPress(Qt::Key_Escape), ViewState(0, KItemSet()));

                    // Next, we test combinations of key presses which only work for a
                    // particular number of columns and either enabled or disabled grouping.

                    // One column.
                    if (columnCount == 1) {
                        testList
                            << qMakePair(KeyPress(nextRowKey), ViewState(1, KItemSet() << 1))
                            << qMakePair(KeyPress(nextRowKey, Qt::ShiftModifier), ViewState(2, KItemSet() << 1 << 2))
                            << qMakePair(KeyPress(nextRowKey, Qt::ControlModifier), ViewState(3, KItemSet() << 1 << 2))
                            << qMakePair(KeyPress(previousRowKey), ViewState(2, KItemSet() << 2))
                            << qMakePair(KeyPress(previousItemKey), ViewState(1, KItemSet() << 1))
                            << qMakePair(KeyPress(Qt::Key_Home), ViewState(0, KItemSet() << 0));
                    }

                    // Multiple columns: we test both 3 and 5 columns with grouping
                    // enabled or disabled. For each case, the layout of the items
                    // in the view is shown (both using file names and indices) to
                    // make it easier to understand what the tests do.

                    if (columnCount == 3 && !groupingEnabled) {
                        // 3 columns, no grouping:
                        //
                        // a1 a2 a3 |  0  1  2
                        // b1 c1 c2 |  3  4  5
                        // c3 c4 c5 |  6  7  8
                        // d1 d2 d3 |  9 10 11
                        // d4 e1 e2 | 12 13 14
                        // e3 e4 e5 | 15 16 17
                        // e6 e7    | 18 19
                        testList
                            << qMakePair(KeyPress(nextRowKey), ViewState(3, KItemSet() << 3))
                            << qMakePair(KeyPress(nextItemKey, Qt::ControlModifier), ViewState(4, KItemSet() << 3))
                            << qMakePair(KeyPress(nextRowKey), ViewState(7, KItemSet() << 7))
                            << qMakePair(KeyPress(nextItemKey, Qt::ShiftModifier), ViewState(8, KItemSet() << 7 << 8))
                            << qMakePair(KeyPress(nextItemKey, Qt::ShiftModifier), ViewState(9, KItemSet() << 7 << 8 << 9))
                            << qMakePair(KeyPress(previousItemKey, Qt::ShiftModifier), ViewState(8, KItemSet() << 7 << 8))
                            << qMakePair(KeyPress(previousItemKey, Qt::ShiftModifier), ViewState(7, KItemSet() << 7))
                            << qMakePair(KeyPress(previousItemKey, Qt::ShiftModifier), ViewState(6, KItemSet() << 6 << 7))
                            << qMakePair(KeyPress(previousItemKey, Qt::ShiftModifier), ViewState(5, KItemSet() << 5 << 6 << 7))
                            << qMakePair(KeyPress(nextItemKey, Qt::ShiftModifier), ViewState(6, KItemSet() << 6 << 7))
                            << qMakePair(KeyPress(nextItemKey, Qt::ShiftModifier), ViewState(7, KItemSet() << 7))
                            << qMakePair(KeyPress(nextRowKey), ViewState(10, KItemSet() << 10))
                            << qMakePair(KeyPress(nextItemKey), ViewState(11, KItemSet() << 11))
                            << qMakePair(KeyPress(nextRowKey), ViewState(14, KItemSet() << 14))
                            << qMakePair(KeyPress(nextRowKey), ViewState(17, KItemSet() << 17))
                            << qMakePair(KeyPress(nextRowKey), ViewState(19, KItemSet() << 19))
                            << qMakePair(KeyPress(previousRowKey), ViewState(17, KItemSet() << 17))
                            << qMakePair(KeyPress(Qt::Key_End), ViewState(19, KItemSet() << 19))
                            << qMakePair(KeyPress(previousRowKey), ViewState(16, KItemSet() << 16))
                            << qMakePair(KeyPress(Qt::Key_Home), ViewState(0, KItemSet() << 0));
                    }

                    if (columnCount == 5 && !groupingEnabled) {
                        // 5 columns, no grouping:
                        //
                        // a1 a2 a3 b1 c1 |  0  1  2  3  4
                        // c2 c3 c4 c5 d1 |  5  6  7  8  9
                        // d2 d3 d4 e1 e2 | 10 11 12 13 14
                        // e3 e4 e5 e6 e7 | 15 16 17 18 19
                        testList
                            << qMakePair(KeyPress(nextRowKey), ViewState(5, KItemSet() << 5))
                            << qMakePair(KeyPress(nextItemKey, Qt::ControlModifier), ViewState(6, KItemSet() << 5))
                            << qMakePair(KeyPress(nextRowKey), ViewState(11, KItemSet() << 11))
                            << qMakePair(KeyPress(nextItemKey), ViewState(12, KItemSet() << 12))
                            << qMakePair(KeyPress(nextRowKey, Qt::ShiftModifier), ViewState(17, KItemSet() << 12 << 13 << 14 << 15 << 16 << 17))
                            << qMakePair(KeyPress(previousRowKey, Qt::ShiftModifier), ViewState(12, KItemSet() << 12))
                            << qMakePair(KeyPress(previousRowKey, Qt::ShiftModifier), ViewState(7, KItemSet() << 7 << 8 << 9 << 10 << 11 << 12))
                            << qMakePair(KeyPress(nextRowKey, Qt::ShiftModifier), ViewState(12, KItemSet() << 12))
                            << qMakePair(KeyPress(Qt::Key_End, Qt::ControlModifier), ViewState(19, KItemSet() << 12))
                            << qMakePair(KeyPress(previousRowKey), ViewState(14, KItemSet() << 14))
                            << qMakePair(KeyPress(Qt::Key_Home), ViewState(0, KItemSet() << 0));
                    }

                    if (columnCount == 3 && groupingEnabled) {
                        // 3 columns, with grouping:
                        //
                        // a1 a2 a3 |  0  1  2
                        // b1       |  3
                        // c1 c2 c3 |  4  5  6
                        // c4 c5    |  7  8
                        // d1 d2 d3 |  9 10 11
                        // d4       | 12
                        // e1 e2 e3 | 13 14 15
                        // e4 e5 e6 | 16 17 18
                        // e7       | 19
                        testList
                            << qMakePair(KeyPress(nextItemKey), ViewState(1, KItemSet() << 1))
                            << qMakePair(KeyPress(nextItemKey), ViewState(2, KItemSet() << 2))
                            << qMakePair(KeyPress(nextRowKey, Qt::ShiftModifier), ViewState(3, KItemSet() << 2 << 3))
                            << qMakePair(KeyPress(nextRowKey, Qt::ShiftModifier), ViewState(6, KItemSet() << 2 << 3 << 4 << 5 << 6))
                            << qMakePair(KeyPress(nextRowKey), ViewState(8, KItemSet() << 8))
                            << qMakePair(KeyPress(nextRowKey), ViewState(11, KItemSet() << 11))
                            << qMakePair(KeyPress(nextItemKey, Qt::ControlModifier), ViewState(12, KItemSet() << 11))
                            << qMakePair(KeyPress(nextRowKey), ViewState(13, KItemSet() << 13))
                            << qMakePair(KeyPress(nextRowKey), ViewState(16, KItemSet() << 16))
                            << qMakePair(KeyPress(nextItemKey), ViewState(17, KItemSet() << 17))
                            << qMakePair(KeyPress(nextRowKey), ViewState(19, KItemSet() << 19))
                            << qMakePair(KeyPress(previousRowKey), ViewState(17, KItemSet() << 17))
                            << qMakePair(KeyPress(Qt::Key_Home), ViewState(0, KItemSet() << 0));
                    }

                    if (columnCount == 5 && groupingEnabled) {
                        // 5 columns, with grouping:
                        //
                        // a1 a2 a3       |  0  1  2
                        // b1             |  3
                        // c1 c2 c3 c4 c5 |  4  5  6  7  8
                        // d1 d2 d3 d4    |  9 10 11 12
                        // e1 e2 e3 e4 e5 | 13 14 15 16 17
                        // e6 e7          | 18 19
                        testList
                            << qMakePair(KeyPress(nextItemKey), ViewState(1, KItemSet() << 1))
                            << qMakePair(KeyPress(nextRowKey, Qt::ShiftModifier), ViewState(3, KItemSet() << 1 << 2 << 3))
                            << qMakePair(KeyPress(nextRowKey, Qt::ShiftModifier), ViewState(5, KItemSet() << 1 << 2 << 3 << 4 << 5))
                            << qMakePair(KeyPress(nextItemKey), ViewState(6, KItemSet() << 6))
                            << qMakePair(KeyPress(nextItemKey, Qt::ControlModifier), ViewState(7, KItemSet() << 6))
                            << qMakePair(KeyPress(nextItemKey, Qt::ControlModifier), ViewState(8, KItemSet() << 6))
                            << qMakePair(KeyPress(nextRowKey), ViewState(12, KItemSet() << 12))
                            << qMakePair(KeyPress(nextRowKey), ViewState(17, KItemSet() << 17))
                            << qMakePair(KeyPress(nextRowKey), ViewState(19, KItemSet() << 19))
                            << qMakePair(KeyPress(previousRowKey), ViewState(17, KItemSet() << 17))
                            << qMakePair(KeyPress(Qt::Key_End, Qt::ShiftModifier), ViewState(19, KItemSet() << 17 << 18 << 19))
                            << qMakePair(KeyPress(previousRowKey, Qt::ShiftModifier), ViewState(14, KItemSet() << 14 << 15 << 16 << 17))
                            << qMakePair(KeyPress(Qt::Key_Home), ViewState(0, KItemSet() << 0));
                    }

                    const QString testName =
                        layoutNames[layout] + ", " +
                        QString("%1 columns, ").arg(columnCount) +
                        selectionBehaviorNames[selectionBehavior] + ", " +
                        groupingEnabledNames[groupingEnabled];

                    const QByteArray testNameAscii = testName.toLatin1();

                    QTest::newRow(testNameAscii.data())
                        << layout
                        << scrollOrientation
                        << columnCount
                        << selectionBehavior
                        << groupingEnabled
                        << testList;
                }
            }
        }
    }
}

/**
 * This function sets the view's properties according to the data provided by
 * KItemListControllerTest::testKeyboardNavigation_data().
 *
 * The list \a testList contains pairs of key presses, which are sent to the
 * container, and expected view states, which are verified then.
 */
void KItemListControllerTest::testKeyboardNavigation()
{
    QFETCH(KFileItemListView::ItemLayout, layout);
    QFETCH(Qt::Orientation, scrollOrientation);
    QFETCH(int, columnCount);
    QFETCH(KItemListController::SelectionBehavior, selectionBehavior);
    QFETCH(bool, groupingEnabled);
    QFETCH(QList<keyPressViewStatePair>, testList);

    m_view->setItemLayout(layout);
    QCOMPARE(m_view->itemLayout(), layout);

    m_view->setScrollOrientation(scrollOrientation);
    QCOMPARE(m_view->scrollOrientation(), scrollOrientation);

    m_controller->setSelectionBehavior(selectionBehavior);
    QCOMPARE(m_controller->selectionBehavior(), selectionBehavior);

    m_model->setGroupedSorting(groupingEnabled);
    QCOMPARE(m_model->groupedSorting(), groupingEnabled);

    adjustGeometryForColumnCount(columnCount);
    QCOMPARE(m_view->m_layouter->m_columnCount, columnCount);

    QSignalSpy spySingleItemActivated(m_controller, SIGNAL(itemActivated(int)));
    QSignalSpy spyMultipleItemsActivated(m_controller, SIGNAL(itemsActivated(KItemSet)));

    while (!testList.isEmpty()) {
        const QPair<KeyPress, ViewState> test = testList.takeFirst();
        const Qt::Key key = test.first.m_key;
        const Qt::KeyboardModifiers modifier = test.first.m_modifier;
        const int current = test.second.m_current;
        const KItemSet selection = test.second.m_selection;
        const bool activated = test.second.m_activated;

        QTest::keyClick(m_container, key, modifier);

        QCOMPARE(m_selectionManager->currentItem(), current);
        switch (selectionBehavior) {
        case KItemListController::NoSelection: QVERIFY(m_selectionManager->selectedItems().isEmpty()); break;
        case KItemListController::SingleSelection: QCOMPARE(m_selectionManager->selectedItems(), KItemSet() << current); break;
        case KItemListController::MultiSelection: QCOMPARE(m_selectionManager->selectedItems(), selection); break;
        }

        if (activated) {
            switch (selectionBehavior) {
            case KItemListController::MultiSelection:
                if (!selection.isEmpty()) {
                    // The selected items should be activated.
                    if (selection.count() == 1) {
                        QVERIFY(!spySingleItemActivated.isEmpty());
                        QCOMPARE(qvariant_cast<int>(spySingleItemActivated.takeFirst().at(0)), selection.first());
                        QVERIFY(spyMultipleItemsActivated.isEmpty());
                    } else {
                        QVERIFY(spySingleItemActivated.isEmpty());
                        QVERIFY(!spyMultipleItemsActivated.isEmpty());
                        QCOMPARE(qvariant_cast<KItemSet>(spyMultipleItemsActivated.takeFirst().at(0)), selection);
                    }
                    break;
                }
                // No items are selected. Therefore, the current item should be activated.
                // This is handled by falling through to the NoSelection/SingleSelection case.
            case KItemListController::NoSelection:
            case KItemListController::SingleSelection:
                // In NoSelection and SingleSelection mode, the current item should be activated.
                QVERIFY(!spySingleItemActivated.isEmpty());
                QCOMPARE(qvariant_cast<int>(spySingleItemActivated.takeFirst().at(0)), current);
                QVERIFY(spyMultipleItemsActivated.isEmpty());
                break;
            }
        }
    }
}

void KItemListControllerTest::testMouseClickActivation()
{
    m_view->setItemLayout(KFileItemListView::IconsLayout);

    // Make sure that we have a large window, such that
    // the items are visible and clickable.
    adjustGeometryForColumnCount(5);

    // Make sure that the first item is visible in the view.
    m_view->setScrollOffset(0);
    QCOMPARE(m_view->firstVisibleIndex(), 0);

    const QPointF pos = m_view->itemContextRect(0).center();

    // Save the "single click" setting.
    const bool restoreSettingsSingleClick = m_testStyle->activateItemOnSingleClick();

    QGraphicsSceneMouseEvent mousePressEvent(QEvent::GraphicsSceneMousePress);
    mousePressEvent.setPos(pos);
    mousePressEvent.setButton(Qt::LeftButton);
    mousePressEvent.setButtons(Qt::LeftButton);

    QGraphicsSceneMouseEvent mouseReleaseEvent(QEvent::GraphicsSceneMouseRelease);
    mouseReleaseEvent.setPos(pos);
    mouseReleaseEvent.setButton(Qt::LeftButton);
    mouseReleaseEvent.setButtons(Qt::NoButton);

    QSignalSpy spyItemActivated(m_controller, SIGNAL(itemActivated(int)));

    // Default setting: single click activation.
    m_testStyle->setActivateItemOnSingleClick(true);
    m_view->event(&mousePressEvent);
    m_view->event(&mouseReleaseEvent);
    QCOMPARE(spyItemActivated.count(), 1);
    spyItemActivated.clear();

    // Set the global setting to "double click activation".
    m_testStyle->setActivateItemOnSingleClick(false);
    m_view->event(&mousePressEvent);
    m_view->event(&mouseReleaseEvent);
    QCOMPARE(spyItemActivated.count(), 0);
    spyItemActivated.clear();

    // Enforce single click activation in the controller.
    m_controller->setSingleClickActivationEnforced(true);
    m_view->event(&mousePressEvent);
    m_view->event(&mouseReleaseEvent);
    QCOMPARE(spyItemActivated.count(), 1);
    spyItemActivated.clear();

    // Do not enforce single click activation in the controller.
    m_controller->setSingleClickActivationEnforced(false);
    m_view->event(&mousePressEvent);
    m_view->event(&mouseReleaseEvent);
    QCOMPARE(spyItemActivated.count(), 0);
    spyItemActivated.clear();

    // Set the global setting back to "single click activation".
    m_testStyle->setActivateItemOnSingleClick(true);
    m_view->event(&mousePressEvent);
    m_view->event(&mouseReleaseEvent);
    QCOMPARE(spyItemActivated.count(), 1);
    spyItemActivated.clear();

    // Enforce single click activation in the controller.
    m_controller->setSingleClickActivationEnforced(true);
    m_view->event(&mousePressEvent);
    m_view->event(&mouseReleaseEvent);
    QCOMPARE(spyItemActivated.count(), 1);
    spyItemActivated.clear();

    // Restore previous settings.
    m_controller->setSingleClickActivationEnforced(true);
    m_testStyle->setActivateItemOnSingleClick(restoreSettingsSingleClick);
}

void KItemListControllerTest::adjustGeometryForColumnCount(int count)
{
    const QSize size = m_view->itemSize().toSize();

    QRect rect = m_container->geometry();
    rect.setSize(size * count);
    m_container->setGeometry(rect);

    // Increase the size of the container until the correct column count is reached.
    while (m_view->m_layouter->m_columnCount < count) {
        rect = m_container->geometry();
        rect.setSize(rect.size() + size);
        m_container->setGeometry(rect);
    }
}

QTEST_MAIN(KItemListControllerTest)

#include "kitemlistcontrollertest.moc"
