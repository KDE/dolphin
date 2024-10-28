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

    m_announceDescriptionChangeTimer = new QTimer{view_};
    m_announceDescriptionChangeTimer->setSingleShot(true);
    m_announceDescriptionChangeTimer->setInterval(100);
    KItemListGroupHeader::connect(m_announceDescriptionChangeTimer, &QTimer::timeout, view_, [this]() {
        // The below will have no effect if one of the list items has focus and not the view itself. Still we announce the accessibility description change
        // here in case the view itself has focus e.g. after tabbing there or after opening a new location.
        QAccessibleEvent announceAccessibleDescriptionEvent(this, QAccessible::DescriptionChanged);
        QAccessible::updateAccessibility(&announceAccessibleDescriptionEvent);
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

void KItemListViewAccessible::modelReset()
{
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
    const auto currentItem = child(controller->selectionManager()->currentItem());
    if (!currentItem) {
        return i18nc("@info 1 states that the folder is empty and sometimes why, 2 is the full filesystem path",
                     "%1 at location %2",
                     m_placeholderMessage,
                     modelRootUrl.toDisplayString());
    }

    const QString selectionStateString{isSelected(currentItem) ? QString()
                                                               // i18n: There is a comma at the end because this is one property in an enumeration of
                                                               // properties that a file or folder has. Accessible text for accessibility software like screen
                                                               // readers.
                                                               : i18n("not selected,")};

    QString expandableStateString;
    if (currentItem->state().expandable) {
        if (currentItem->state().collapsed) {
            // i18n: There is a comma at the end because this is one property in an enumeration of properties that a folder in a tree view has.
            // Accessible text for accessibility software like screen readers.
            expandableStateString = i18n("collapsed,");
        } else {
            // i18n: There is a comma at the end because this is one property in an enumeration of properties that a folder in a tree view has.
            // Accessible text for accessibility software like screen readers.
            expandableStateString = i18n("expanded,");
        }
    }

    const QString selectedItemCountString{selectedItemCount() > 1
                                              // i18n: There is a "—" at the beginning because this is a followup sentence to a text that did not properly end
                                              // with a period. Accessible text for accessibility software like screen readers.
                                              ? i18np("— %1 selected item", "— %1 selected items", selectedItemCount())
                                              : QString()};

    // Determine if we should announce the item layout. For end users of the accessibility tree there is an expectation that a list can be scrolled through by
    // pressing the "Down" key repeatedly. This is not the case in the icon view mode, where pressing "Right" or "Left" moves through the whole list of items.
    // Therefore we need to announce this layout when in icon view mode.
    QString layoutAnnouncementString;
    if (auto standardView = qobject_cast<const KStandardItemListView *>(view())) {
        if (standardView->itemLayout() == KStandardItemListView::ItemLayout::IconsLayout) {
            layoutAnnouncementString = i18nc("@info refering to a file or folder", "in a grid layout");
        }
    }

    /**
     * Announce it in this order so the most important information is at the beginning and the potentially very long path at the end:
     * "$currentlyFocussedItemName, $currentlyFocussedItemDescription, $currentFolderPath".
     * We do not need to announce the total count of items here because accessibility software like Orca alrady announces this automatically for lists.
     * Normally for list items the selection and expandadable state are also automatically announced by Orca, however we are building the accessible
     * description of the view here, so we need to manually add all infomation about the current item we also want to announce.
     */
    return i18nc(
        "@info 1 is currentlyFocussedItemName, 2 is empty or \"not selected, \", 3 is currentlyFocussedItemDescription, 3 is currentFolderName, 4 is "
        "currentFolderPath",
        "%1, %2 %3 %4 %5 %6 in location %7",
        currentItem->text(QAccessible::Name),
        selectionStateString,
        expandableStateString,
        currentItem->text(QAccessible::Description),
        selectedItemCountString,
        layoutAnnouncementString,
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

void KItemListViewAccessible::announceOverallViewState(const QString &placeholderMessage)
{
    m_placeholderMessage = placeholderMessage;

    // Make sure we announce this placeholderMessage. However, do not announce it when the focus is on an unrelated object currently.
    // We for example do not want to announce "Loading cancelled" when the focus is currently on an error message explaining why the loading was cancelled.
    if (view()->hasFocus() || !QApplication::focusWidget() || static_cast<QWidget *>(m_parent->object())->isAncestorOf(QApplication::focusWidget())) {
        view()->setFocus();
        // If we move focus to an item and right after that the description of the item is changed, the item will be announced twice.
        // We want to avoid that so we wait until after the description change was announced to move focus.
        KItemListGroupHeader::connect(
            m_announceDescriptionChangeTimer,
            &QTimer::timeout,
            view(),
            [this]() {
                if (view()->hasFocus() || !QApplication::focusWidget()
                    || static_cast<QWidget *>(m_parent->object())->isAncestorOf(QApplication::focusWidget())) {
                    QAccessibleEvent accessibleFocusEvent(this, QAccessible::Focus);
                    QAccessible::updateAccessibility(&accessibleFocusEvent); // This accessibility update is perhaps even too important: It is generally
                    // the last triggered update after changing the currently viewed folder. This call makes sure that we announce the new directory in
                    // full. Furthermore it also serves its original purpose of making sure we announce the placeholderMessage in empty folders.
                }
            },
            Qt::SingleShotConnection);
        if (!m_announceDescriptionChangeTimer->isActive()) {
            m_announceDescriptionChangeTimer->start();
        }
    }
}

void KItemListViewAccessible::announceDescriptionChange()
{
    m_announceDescriptionChangeTimer->start();
}
