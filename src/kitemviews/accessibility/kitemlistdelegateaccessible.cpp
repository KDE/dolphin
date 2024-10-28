/*
 * SPDX-FileCopyrightText: 2012 Amandeep Singh <aman.dedman@gmail.com>
 * SPDX-FileCopyrightText: 2024 Felix Ernst <felixernst@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kitemlistdelegateaccessible.h"
#include "kitemviews/kfileitemlistwidget.h"
#include "kitemviews/kfileitemmodel.h"
#include "kitemviews/kitemlistcontroller.h"
#include "kitemviews/kitemlistselectionmanager.h"
#include "kitemviews/kitemlistview.h"
#include "kitemviews/private/kitemlistviewlayouter.h"

#include <KLocalizedString>

#include <QGraphicsScene>
#include <QGraphicsView>

KItemListDelegateAccessible::KItemListDelegateAccessible(KItemListView *view, int index)
    : m_view(view)
    , m_index(index)
{
    Q_ASSERT(index >= 0 && index < view->model()->count());
}

void *KItemListDelegateAccessible::interface_cast(QAccessible::InterfaceType type)
{
    if (type == QAccessible::TableCellInterface) {
        return static_cast<QAccessibleTableCellInterface *>(this);
    }
    return nullptr;
}

int KItemListDelegateAccessible::columnExtent() const
{
    return 1;
}

int KItemListDelegateAccessible::rowExtent() const
{
    return 1;
}

QList<QAccessibleInterface *> KItemListDelegateAccessible::rowHeaderCells() const
{
    return QList<QAccessibleInterface *>();
}

QList<QAccessibleInterface *> KItemListDelegateAccessible::columnHeaderCells() const
{
    return QList<QAccessibleInterface *>();
}

int KItemListDelegateAccessible::columnIndex() const
{
    return m_view->m_layouter->itemColumn(m_index);
}

int KItemListDelegateAccessible::rowIndex() const
{
    return m_view->m_layouter->itemRow(m_index);
}

bool KItemListDelegateAccessible::isSelected() const
{
    return m_view->controller()->selectionManager()->isSelected(m_index);
}

QAccessibleInterface *KItemListDelegateAccessible::table() const
{
    return QAccessible::queryAccessibleInterface(m_view);
}

QAccessible::Role KItemListDelegateAccessible::role() const
{
    return QAccessible::ListItem; // We could also return "Cell" here which would then announce the exact row and column of the item. However, different from
    // applications that actually have a strong cell workflow -- like LibreOfficeCalc -- we have no advantage of announcing the row or column aside from us
    // generally being interested in announcing that users in Icon View mode need to use the Left and Right arrow keys to arrive at every item. There are ways
    // for users to figure this out regardless by paying attention to the index that is being announced for each list item. In KitemListViewAccessible in icon
    // view mode it is also mentioned that the items are positioned in a grid, so the two-dimensionality should be clear enough.
}

QAccessible::State KItemListDelegateAccessible::state() const
{
    QAccessible::State state;

    state.selectable = true;
    if (isSelected()) {
        state.selected = true;
    }

    state.focusable = true;
    if (m_view->controller()->selectionManager()->currentItem() == m_index) {
        state.focused = true;
        state.active = true;
    }

    if (m_view->controller()->selectionBehavior() == KItemListController::MultiSelection) {
        state.multiSelectable = true;
    }

    if (m_view->supportsItemExpanding() && m_view->model()->isExpandable(m_index)) {
        state.expandable = true;
        state.expanded = m_view->model()->isExpanded(m_index);
        state.collapsed = !state.expanded;
    }

    return state;
}

bool KItemListDelegateAccessible::isExpandable() const
{
    return m_view->model()->isExpandable(m_index);
}

QRect KItemListDelegateAccessible::rect() const
{
    QRect rect = m_view->itemRect(m_index).toRect();

    if (rect.isNull()) {
        return QRect();
    }

    rect.translate(m_view->mapToScene(QPointF(0.0, 0.0)).toPoint());
    rect.translate(m_view->scene()->views()[0]->mapToGlobal(QPoint(0, 0)));
    return rect;
}

QString KItemListDelegateAccessible::text(QAccessible::Text t) const
{
    const QHash<QByteArray, QVariant> data = m_view->model()->data(m_index);
    switch (t) {
    case QAccessible::Name: {
        return data["text"].toString();
    }
    case QAccessible::Description: {
        QString description;

        if (data["isHidden"].toBool()) {
            description += i18nc("@info", "hidden");
        }

        QString mimeType{data["type"].toString()};
        if (mimeType.isEmpty()) {
            const KFileItemModel *model = qobject_cast<KFileItemModel *>(m_view->model());
            if (model) {
                mimeType = model->fileItem(m_index).mimeComment();
            }
            Q_ASSERT_X(!mimeType.isEmpty(), "KItemListDelegateAccessible::text", "Unable to retrieve mime type.");
        }

        if (data["isLink"].toBool()) {
            QString linkDestination{data["destination"].toString()};
            if (linkDestination.isEmpty()) {
                const KFileItemModel *model = qobject_cast<KFileItemModel *>(m_view->model());
                if (model) {
                    linkDestination = model->fileItem(m_index).linkDest();
                }
                Q_ASSERT_X(!linkDestination.isEmpty(), "KItemListDelegateAccessible::text", "Unable to retrieve link destination.");
            }

            description += i18nc("@info enumeration saying this is a link to $1, %1 is mimeType", ", link to %1 at %2", mimeType, linkDestination);
        } else {
            description += i18nc("@info enumeration, %1 is mimeType", ", %1", mimeType);
        }
        const QList<QByteArray> additionallyShownInformation{m_view->visibleRoles()};
        const KItemModelBase *model = m_view->model();
        for (const auto &roleInformation : additionallyShownInformation) {
            if (roleInformation == "text") {
                continue;
            }
            KFileItemListWidgetInformant informant;
            const auto roleText{informant.roleText(roleInformation, data, KFileItemListWidgetInformant::ForUsageAs::SpokenText)};
            if (roleText.isEmpty()) {
                continue; // No need to announce roles which are empty for this item.
            }
            description +=
                // i18n: The text starts with a comma because multiple occurences of this text can follow after each others as an enumeration.
                // Normally it would make sense to have a colon between property and value to make the relation between the property and its property value
                // clear, however this is accessible text that will be read out by screen readers. That's why there is only a space between the two here,
                // because screen readers would read the colon literally as "colon", which is just a waste of time for users who might go through a list of
                // hundreds of items. So, if you want to add any more punctation there to improve structure, try to make sure that it will not lead to annoying
                // announcements when read out by a screen reader.
                i18nc("@info accessibility enumeration, %1 is property, %2 is value", ", %1 %2", model->roleDescription(roleInformation), roleText);
        }
        return description;
    }
    default:
        break;
    }

    return QString();
}

void KItemListDelegateAccessible::setText(QAccessible::Text, const QString &)
{
}

QAccessibleInterface *KItemListDelegateAccessible::child(int) const
{
    return nullptr;
}

bool KItemListDelegateAccessible::isValid() const
{
    return m_view && (m_index >= 0) && (m_index < m_view->model()->count());
}

QAccessibleInterface *KItemListDelegateAccessible::childAt(int, int) const
{
    return nullptr;
}

int KItemListDelegateAccessible::childCount() const
{
    return 0;
}

int KItemListDelegateAccessible::indexOfChild(const QAccessibleInterface *child) const
{
    Q_UNUSED(child)
    return -1;
}

QAccessibleInterface *KItemListDelegateAccessible::parent() const
{
    return QAccessible::queryAccessibleInterface(m_view);
}

int KItemListDelegateAccessible::index() const
{
    return m_index;
}

QObject *KItemListDelegateAccessible::object() const
{
    return nullptr;
}
