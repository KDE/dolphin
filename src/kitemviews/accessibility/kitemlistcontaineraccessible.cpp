/*
 * SPDX-FileCopyrightText: 2012 Amandeep Singh <aman.dedman@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kitemlistcontaineraccessible.h"

#include "kitemlistcontaineraccessible.h"
#include "kitemlistviewaccessible.h"
#include "kitemviews/kitemlistcontainer.h"
#include "kitemviews/kitemlistcontroller.h"
#include "kitemviews/kitemlistselectionmanager.h"
#include "kitemviews/kitemlistview.h"
#include "kitemviews/kitemmodelbase.h"

#include <KLocalizedString>

KItemListContainerAccessible::KItemListContainerAccessible(KItemListContainer *container)
    : QAccessibleWidget(container)
{
}

KItemListContainerAccessible::~KItemListContainerAccessible()
{
}

QString KItemListContainerAccessible::text(QAccessible::Text t) const
{
    Q_UNUSED(t)
    return QString(); // This class should never have focus. Instead KItemListViewAccessible should be focused and read out.
}

int KItemListContainerAccessible::childCount() const
{
    return 1;
}

int KItemListContainerAccessible::indexOfChild(const QAccessibleInterface *child) const
{
    if (child == KItemListContainerAccessible::child(0)) {
        return 0;
    }
    return -1;
}

QAccessibleInterface *KItemListContainerAccessible::child(int index) const
{
    if (index == 0) {
        Q_CHECK_PTR(static_cast<KItemListViewAccessible *>(QAccessible::queryAccessibleInterface(container()->controller()->view())));
        return QAccessible::queryAccessibleInterface(container()->controller()->view());
    }
    qWarning("Calling KItemListContainerAccessible::child(index) with index != 0 is always pointless.");
    return nullptr;
}

QAccessibleInterface *KItemListContainerAccessible::focusChild() const
{
    return child(0);
}

QAccessible::State KItemListContainerAccessible::state() const
{
    auto state = QAccessibleWidget::state();
    state.focusable = false;
    state.focused = false;
    return state;
}

void KItemListContainerAccessible::doAction(const QString &actionName)
{
    auto view = static_cast<KItemListViewAccessible *>(child(0));
    Q_CHECK_PTR(view); // A container should always have a view. Otherwise it has no reason to exist.
    if (actionName == setFocusAction() && view) {
        view->doAction(actionName);
        return;
    }
    QAccessibleWidget::doAction(actionName);
}

const KItemListContainer *KItemListContainerAccessible::container() const
{
    Q_CHECK_PTR(qobject_cast<KItemListContainer *>(object()));
    return static_cast<KItemListContainer *>(object());
}
