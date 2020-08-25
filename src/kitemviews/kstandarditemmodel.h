/*
 * SPDX-FileCopyrightText: 2012 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KSTANDARDITEMMODEL_H
#define KSTANDARDITEMMODEL_H

#include "dolphin_export.h"
#include "kitemviews/kitemmodelbase.h"

#include <QHash>
#include <QList>

class KStandardItem;

/**
 * @brief Model counterpart for KStandardItemListView.
 *
 * Allows to add items to the model in an easy way by the
 * class KStandardItem.
 *
 * @see KStandardItem
 */
class DOLPHIN_EXPORT KStandardItemModel : public KItemModelBase
{
    Q_OBJECT

public:
    explicit KStandardItemModel(QObject* parent = nullptr);
    ~KStandardItemModel() override;

    /**
     * Inserts the item \a item at the index \a index. If the index
     * is equal to the number of items of the model, the item
     * gets appended as last element. KStandardItemModel takes
     * the ownership of the item. If the index is invalid, the item
     * gets deleted.
     */
    void insertItem(int index, KStandardItem* item);

    /**
     * Changes the item on the index \a index to \a item.
     * KStandardItemModel takes the ownership of the item. The
     * old item gets deleted. If the index is invalid, the item
     * gets deleted.
     */
    void changeItem(int index, KStandardItem* item);

    void removeItem(int index);
    KStandardItem* item(int index) const;
    int index(const KStandardItem* item) const;

    /**
     * Convenience method for insertItem(count(), item).
     */
    void appendItem(KStandardItem* item);

    int count() const override;
    QHash<QByteArray, QVariant> data(int index) const override;
    bool setData(int index, const QHash<QByteArray, QVariant>& values) override;
    QMimeData* createMimeData(const KItemSet& indexes) const override;
    int indexForKeyboardSearch(const QString& text, int startFromIndex = 0) const override;
    bool supportsDropping(int index) const override;
    QString roleDescription(const QByteArray& role) const override;
    QList<QPair<int, QVariant> > groups() const override;

    virtual void clear();
protected:
    /**
     * Is invoked after an item has been inserted and before the signal
     * itemsInserted() gets emitted.
     */
    virtual void onItemInserted(int index);

    /**
     * Is invoked after an item or one of its roles has been changed and
     * before the signal itemsChanged() gets emitted.
     */
    virtual void onItemChanged(int index, const QSet<QByteArray>& changedRoles);

    /**
     * Is invoked after an item has been removed and before the signal
     * itemsRemoved() gets emitted. The item \a removedItem has already
     * been removed from the model and will get deleted after the
     * execution of onItemRemoved().
     */
    virtual void onItemRemoved(int index, KStandardItem* removedItem);

private:
    QList<KStandardItem*> m_items;
    QHash<const KStandardItem*, int> m_indexesForItems;

    friend class KStandardItem;
    friend class KStandardItemModelTest;  // For unit testing
};

#endif


