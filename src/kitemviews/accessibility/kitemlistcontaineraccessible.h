/*
 * SPDX-FileCopyrightText: 2012 Amandeep Singh <aman.dedman@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KITEMLISTCONTAINERACCESSIBLE_H
#define KITEMLISTCONTAINERACCESSIBLE_H

#include "dolphin_export.h"

#include <QAccessibleWidget>

class KItemListContainer;
class KItemListViewAccessible;

/**
 * The accessible interface for KItemListContainer.
 *
 * Truthfully, there is absolutely no reason for screen reader users to interact with this interface.
 * It is only there to bridge the gap between custom accessible interfaces and the automatically by Qt and QWidgets provided accessible interfaces.
 * Really, the main issue is that KItemListContainer itself is the last proper QWidget in the hierarchy while the actual main view is completely custom using
 * QGraphicsView instead, so focus usually officially goes to KItemListContainer which messes with the custom accessibility hierarchy.
 */
class DOLPHIN_EXPORT KItemListContainerAccessible : public QAccessibleWidget
{
public:
    explicit KItemListContainerAccessible(KItemListContainer *container);
    ~KItemListContainerAccessible() override;

    QString text(QAccessible::Text t) const override;

    QAccessibleInterface *child(int index) const override;
    QAccessibleInterface *focusChild() const override;
    int childCount() const override;
    int indexOfChild(const QAccessibleInterface *child) const override;

    QAccessible::State state() const override;
    void doAction(const QString &actionName) override;

    /** @returns the object() of this interface cast to its actual class. */
    const KItemListContainer *container() const;
};

#endif // KITEMLISTCONTAINERACCESSIBLE_H
