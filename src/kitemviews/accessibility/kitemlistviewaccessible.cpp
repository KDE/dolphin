/*
 * SPDX-FileCopyrightText: 2012 Amandeep Singh <aman.dedman@gmail.com>
 * SPDX-FileCopyrightText: 2024 Felix Ernst <felixernst@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kitemlistviewaccessible.h"
#include "kitemlistcontaineraccessible.h"
#include "kitemlistdelegateaccessible.h"

#include "kitemviews/kitemlistcontainer.h"
#include "kitemviews/kitemlistcontroller.h"
#include "kitemviews/kitemlistselectionmanager.h"
#include "kitemviews/kitemlistview.h"
#include "kitemviews/kitemmodelbase.h"
#include "kitemviews/kstandarditemlistview.h"
#include "kitemviews/private/kitemlistviewlayouter.h"

#include <KLocalizedString>

#include <QApplication> // for figuring out if we should move focus to this view.
#include <QGraphicsScene>
#include <QGraphicsView>

KItemListSelectionManager *KItemListViewAccessible::selectionManager() const
{
    return view()->controller()->selectionManager();
}

KItemListViewAccessible::KItemListViewAccessible(KItemListView *view_, KItemListContainerAccessible *parent)
    : QAccessibleObject(view_)
    , m_parent(parent)
{
    Q_ASSERT(view());
    Q_CHECK_PTR(parent);
    m_accessibleDelegates.resize(childCount());

    m_announceCurrentItemTimer = new QTimer{view_};
    m_announceCurrentItemTimer->setSingleShot(true);
    m_announceCurrentItemTimer->setInterval(100);
    KItemListGroupHeader::connect(m_announceCurrentItemTimer, &QTimer::timeout, view_, [this]() {
        slotAnnounceCurrentItemTimerTimeout();
    });
}

KItemListViewAccessible::~KItemListViewAccessible()
{
    for (AccessibleIdWrapper idWrapper : std::as_const(m_accessibleDelegates)) {
        if (idWrapper.isValid) {
            QAccessible::deleteAccessibleInterface(idWrapper.id);
        }
    }
}

void *KItemListViewAccessible::interface_cast(QAccessible::InterfaceType type)
{
    switch (type) {
    case QAccessible::SelectionInterface:
        return static_cast<QAccessibleSelectionInterface *>(this);
    case QAccessible::TableInterface:
        return static_cast<QAccessibleTableInterface *>(this);
    case QAccessible::ActionInterface:
        return static_cast<QAccessibleActionInterface *>(this);
    default:
        return nullptr;
    }
}

QAccessibleInterface *KItemListViewAccessible::accessibleDelegate(int index) const
{
    if (index < 0 || index >= view()->model()->count()) {
        return nullptr;
    }

    if (m_accessibleDelegates.size() <= index) {
        m_accessibleDelegates.resize(childCount());
    }
    Q_ASSERT(index < m_accessibleDelegates.size());

    AccessibleIdWrapper idWrapper = m_accessibleDelegates.at(index);
    if (!idWrapper.isValid) {
        idWrapper.id = QAccessible::registerAccessibleInterface(new KItemListDelegateAccessible(view(), index));
        idWrapper.isValid = true;
        m_accessibleDelegates.insert(index, idWrapper);
    }
    return QAccessible::accessibleInterface(idWrapper.id);
}

QAccessibleInterface *KItemListViewAccessible::cellAt(int row, int column) const
{
    return accessibleDelegate(columnCount() * row + column);
}

QAccessibleInterface *KItemListViewAccessible::caption() const
{
    return nullptr;
}

QString KItemListViewAccessible::columnDescription(int) const
{
    return QString();
}

int KItemListViewAccessible::columnCount() const
{
    return view()->m_layouter->columnCount();
}

int KItemListViewAccessible::rowCount() const
{
    if (columnCount() <= 0) {
        return 0;
    }

    int itemCount = view()->model()->count();
    int rowCount = itemCount / columnCount();

    if (rowCount <= 0) {
        return 0;
    }

    if (itemCount % columnCount()) {
        ++rowCount;
    }
    return rowCount;
}

int KItemListViewAccessible::selectedCellCount() const
{
    return selectionManager()->selectedItems().count();
}

int KItemListViewAccessible::selectedColumnCount() const
{
    return 0;
}

int KItemListViewAccessible::selectedRowCount() const
{
    return 0;
}

QString KItemListViewAccessible::rowDescription(int) const
{
    return QString();
}

QList<QAccessibleInterface *> KItemListViewAccessible::selectedCells() const
{
    QList<QAccessibleInterface *> cells;
    const auto items = selectionManager()->selectedItems();
    cells.reserve(items.count());
    for (int index : items) {
        cells.append(accessibleDelegate(index));
    }
    return cells;
}

QList<int> KItemListViewAccessible::selectedColumns() const
{
    return QList<int>();
}

QList<int> KItemListViewAccessible::selectedRows() const
{
    return QList<int>();
}

QAccessibleInterface *KItemListViewAccessible::summary() const
{
    return nullptr;
}

bool KItemListViewAccessible::isColumnSelected(int) const
{
    return false;
}

bool KItemListViewAccessible::isRowSelected(int) const
{
    return false;
}

bool KItemListViewAccessible::selectRow(int)
{
    return true;
}

bool KItemListViewAccessible::selectColumn(int)
{
    return true;
}

bool KItemListViewAccessible::unselectRow(int)
{
    return true;
}

bool KItemListViewAccessible::unselectColumn(int)
{
    return true;
}

void KItemListViewAccessible::modelChange(QAccessibleTableModelChangeEvent * /*event*/)
{
}

QAccessible::Role KItemListViewAccessible::role() const
{
    return QAccessible::List;
}

QAccessible::State KItemListViewAccessible::state() const
{
    QAccessible::State s;
    s.focusable = true;
    s.active = true;
    const KItemListController *controller = view()->m_controller;
    s.multiSelectable = controller->selectionBehavior() == KItemListController::MultiSelection;
    s.focused = !childCount() && (view()->hasFocus() || m_parent->container()->hasFocus()); // Usually the children have focus.
    return s;
}

QAccessibleInterface *KItemListViewAccessible::childAt(int x, int y) const
{
    const QPointF point = QPointF(x, y);
    const std::optional<int> itemIndex = view()->itemAt(view()->mapFromScene(point));
    return child(itemIndex.value_or(-1));
}

QAccessibleInterface *KItemListViewAccessible::parent() const
{
    return m_parent;
}

int KItemListViewAccessible::childCount() const
{
    return view()->model()->count();
}

int KItemListViewAccessible::indexOfChild(const QAccessibleInterface *interface) const
{
    const KItemListDelegateAccessible *widget = static_cast<const KItemListDelegateAccessible *>(interface);
    return widget->index();
}

QString KItemListViewAccessible::text(QAccessible::Text t) const
{
    const KItemListController *controller = view()->m_controller;
    const KItemModelBase *model = controller->model();
    const QUrl modelRootUrl = model->directory();
    if (t == QAccessible::Name) {
        return modelRootUrl.fileName();
    }
    if (t != QAccessible::Description) {
        return QString();
    }

    QAccessibleInterface *currentItem = child(controller->selectionManager()->currentItem());

    /**
     * Always announce the path last because it might be very long.
     * We do not need to announce the total count of items here because accessibility software like Orca alrady announces this automatically for lists.
     */
    if (!currentItem) {
        return i18nc("@info 1 states that the folder is empty and sometimes why, 2 is the full filesystem path",
                     "%1 at location %2",
                     m_placeholderMessage,
                     modelRootUrl.toDisplayString());
    }

    const int numberOfSelectedItems = selectedItemCount();

    // Determine if we should announce the item layout. For end users of the accessibility tree there is an expectation that a list can be scrolled through by
    // pressing the "Down" key repeatedly. This is not the case in the icon view mode, where pressing "Right" or "Left" moves through the whole list of items.
    // Therefore we need to announce this layout when in icon view mode.
    if (auto standardView = qobject_cast<const KStandardItemListView *>(view())) {
        if (standardView->itemLayout() == KStandardItemListView::ItemLayout::IconsLayout) {
            if (numberOfSelectedItems < 1 || (numberOfSelectedItems == 1 && isSelected(currentItem))) {
                // We do not announce the number of selected items if the only selected item is the current item
                // because the selection state of the current item is already announced elsewhere.
                return i18nc("@info accessibility, 1 is path", "in a grid layout in location %1", modelRootUrl.toDisplayString());
            }
            return i18ncp("@info accessibility, 2 is path",
                          "%1 selected item in a grid layout in location %2",
                          "%1 selected items in a grid layout in location %2",
                          numberOfSelectedItems,
                          modelRootUrl.toDisplayString());
        }
    }

    if (numberOfSelectedItems < 1 || (numberOfSelectedItems == 1 && isSelected(currentItem))) {
        // We do not announce the number of selected items if the only selected item is the current item
        // because the selection state of the current item is already announced elsewhere.
        return i18nc("@info accessibility, 1 is path", "in location %1", modelRootUrl.toDisplayString());
    }
    return i18ncp("@info accessibility, 2 is path",
                  "%1 selected item in location %2",
                  "%1 selected items in location %2",
                  numberOfSelectedItems,
                  modelRootUrl.toDisplayString());
}

QRect KItemListViewAccessible::rect() const
{
    if (!view()->isVisible()) {
        return QRect();
    }

    const QGraphicsScene *scene = view()->scene();
    if (scene) {
        const QPoint origin = scene->views().at(0)->mapToGlobal(QPoint(0, 0));
        const QRect viewRect = view()->geometry().toRect();
        return viewRect.translated(origin);
    } else {
        return QRect();
    }
}

QAccessibleInterface *KItemListViewAccessible::child(int index) const
{
    if (index >= 0 && index < childCount()) {
        return accessibleDelegate(index);
    }
    return nullptr;
}

KItemListViewAccessible::AccessibleIdWrapper::AccessibleIdWrapper()
    : isValid(false)
    , id(0)
{
}

/* Selection interface */

bool KItemListViewAccessible::clear()
{
    selectionManager()->clearSelection();
    return true;
}

bool KItemListViewAccessible::isSelected(QAccessibleInterface *childItem) const
{
    Q_CHECK_PTR(childItem);
    return static_cast<KItemListDelegateAccessible *>(childItem)->isSelected();
}

bool KItemListViewAccessible::select(QAccessibleInterface *childItem)
{
    selectionManager()->setSelected(indexOfChild(childItem));
    return true;
}

bool KItemListViewAccessible::selectAll()
{
    selectionManager()->setSelected(0, childCount());
    return true;
}

QAccessibleInterface *KItemListViewAccessible::selectedItem(int selectionIndex) const
{
    const auto selectedItems = selectionManager()->selectedItems();
    int i = 0;
    for (auto it = selectedItems.rbegin(); it != selectedItems.rend(); ++it) {
        if (i == selectionIndex) {
            return child(*it);
        }
    }
    return nullptr;
}

int KItemListViewAccessible::selectedItemCount() const
{
    return selectionManager()->selectedItems().count();
}

QList<QAccessibleInterface *> KItemListViewAccessible::selectedItems() const
{
    const auto selectedItems = selectionManager()->selectedItems();
    QList<QAccessibleInterface *> selectedItemsInterfaces;
    for (auto it = selectedItems.rbegin(); it != selectedItems.rend(); ++it) {
        selectedItemsInterfaces.append(child(*it));
    }
    return selectedItemsInterfaces;
}

bool KItemListViewAccessible::unselect(QAccessibleInterface *childItem)
{
    selectionManager()->setSelected(indexOfChild(childItem), 1, KItemListSelectionManager::Deselect);
    return true;
}

/* Action Interface */

QStringList KItemListViewAccessible::actionNames() const
{
    return {setFocusAction()};
}

void KItemListViewAccessible::doAction(const QString &actionName)
{
    if (actionName == setFocusAction()) {
        view()->setFocus();
    }
}

QStringList KItemListViewAccessible::keyBindingsForAction(const QString &actionName) const
{
    Q_UNUSED(actionName)
    return {};
}

/* Custom non-interface methods */

KItemListView *KItemListViewAccessible::view() const
{
    Q_CHECK_PTR(qobject_cast<KItemListView *>(object()));
    return static_cast<KItemListView *>(object());
}

void KItemListViewAccessible::setAccessibleFocusAndAnnounceAll()
{
    const int currentItemIndex = view()->m_controller->selectionManager()->currentItem();
    if (currentItemIndex < 0) {
        // The current item is invalid (perhaps because the folder is empty), so we set the focus to the view itself instead.
        QAccessibleEvent accessibleFocusInEvent(this, QAccessible::Focus);
        QAccessible::updateAccessibility(&accessibleFocusInEvent);
        return;
    }

    QAccessibleEvent accessibleFocusInEvent(this, QAccessible::Focus);
    accessibleFocusInEvent.setChild(currentItemIndex);
    QAccessible::updateAccessibility(&accessibleFocusInEvent);
    m_shouldAnnounceLocation = true;
    announceCurrentItem();
}

void KItemListViewAccessible::announceNewlyLoadedLocation(const QString &placeholderMessage)
{
    m_placeholderMessage = placeholderMessage;
    m_shouldAnnounceLocation = true;

    // Changes might still be happening in the view. We (re)start the timer to make it less likely that it announces a state that is still in flux.
    m_announceCurrentItemTimer->start();
}

void KItemListViewAccessible::announceCurrentItem()
{
    m_announceCurrentItemTimer->start();
}

void KItemListViewAccessible::slotAnnounceCurrentItemTimerTimeout()
{
    if (!view()->hasFocus() && QApplication::focusWidget() && QApplication::focusWidget()->isVisible()
        && !static_cast<QWidget *>(m_parent->object())->isAncestorOf(QApplication::focusWidget())) {
        // Something else than this view has focus, so we do not announce anything.
        m_lastAnnouncedIndex = -1; // Reset this to -1 so we properly move focus to the current item the next time this method is called.
        return;
    }

    /// Announce the current item (or the view if there is no current item).
    const int currentIndex = view()->m_controller->selectionManager()->currentItem();
    if (currentIndex < 0) {
        // The current index is invalid! There might be no items in the list. Instead the list itself is announced.
        m_shouldAnnounceLocation = true;
        QAccessibleEvent announceEmptyViewPlaceholderMessageEvent(this, QAccessible::Focus);
        QAccessible::updateAccessibility(&announceEmptyViewPlaceholderMessageEvent);
    } else if (currentIndex != m_lastAnnouncedIndex) {
        QAccessibleEvent announceNewlyFocusedItemEvent(this, QAccessible::Focus);
        announceNewlyFocusedItemEvent.setChild(currentIndex);
        QAccessible::updateAccessibility(&announceNewlyFocusedItemEvent);
    } else {
        QAccessibleEvent announceCurrentItemNameChangeEvent(this, QAccessible::NameChanged);
        announceCurrentItemNameChangeEvent.setChild(currentIndex);
        QAccessible::updateAccessibility(&announceCurrentItemNameChangeEvent);
        QAccessibleEvent announceCurrentItemDescriptionChangeEvent(this, QAccessible::DescriptionChanged);
        announceCurrentItemDescriptionChangeEvent.setChild(currentIndex);
        QAccessible::updateAccessibility(&announceCurrentItemDescriptionChangeEvent);
    }
    m_lastAnnouncedIndex = currentIndex;

    /// Announce the location if we are not just moving within the same location.
    if (m_shouldAnnounceLocation) {
        m_shouldAnnounceLocation = false;

        QAccessibleEvent announceAccessibleDescriptionEvent1(this, QAccessible::NameChanged);
        QAccessible::updateAccessibility(&announceAccessibleDescriptionEvent1);
        QAccessibleEvent announceAccessibleDescriptionEvent(this, QAccessible::DescriptionChanged);
        QAccessible::updateAccessibility(&announceAccessibleDescriptionEvent);
    }
}
