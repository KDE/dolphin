/*
 * SPDX-FileCopyrightText: 2011 Peter Penz <peter.penz19@gmail.com>
 *
 * Based on the Itemviews NG project from Trolltech Labs
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KITEMLISTSELECTIONMANAGER_H
#define KITEMLISTSELECTIONMANAGER_H

#include "dolphin_export.h"
#include "kitemviews/kitemmodelbase.h"
#include "kitemviews/kitemset.h"

#include <QObject>

class KItemModelBase;

/**
 * @brief Allows to select and deselect items of a KItemListView.
 */
class DOLPHIN_EXPORT KItemListSelectionManager : public QObject
{
    Q_OBJECT

    enum RangesRemovingBehaviour {
        DiscardRemovedIndex,
        AdjustRemovedIndex
    };

public:
    enum SelectionMode {
        Select,
        Deselect,
        Toggle
    };

    explicit KItemListSelectionManager(QObject* parent = nullptr);
    ~KItemListSelectionManager() override;

    void setCurrentItem(int current);
    int currentItem() const;

    void setSelectedItems(const KItemSet& items);
    KItemSet selectedItems() const;
    bool isSelected(int index) const;
    bool hasSelection() const;

    void setSelected(int index, int count = 1, SelectionMode mode = Select);
    /**
     * Equivalent to:
     * clearSelection();
     * setSelected(index, count);
     * but emitting once only selectionChanged signal
     */
    void replaceSelection(int index, int count = 1);
    void clearSelection();

    void beginAnchoredSelection(int anchor);
    void endAnchoredSelection();
    bool isAnchoredSelectionActive() const;

    KItemModelBase* model() const;

Q_SIGNALS:
    void currentChanged(int current, int previous);
    void selectionChanged(const KItemSet& current, const KItemSet& previous);

private:
    void setModel(KItemModelBase* model);
    void itemsInserted(const KItemRangeList& itemRanges);
    void itemsRemoved(const KItemRangeList& itemRanges);
    void itemsMoved(const KItemRange& itemRange, const QList<int>& movedToIndexes);


    /**
     * Helper method for itemsRemoved. Returns the changed index after removing
     * the given range. If the index is part of the range, std::nullopt will be
     * returned.
     */
    std::optional<int> indexAfterRangesRemoving(int index, const KItemRangeList& itemRanges, const RangesRemovingBehaviour behaviour) const;

private:
    int m_currentItem;
    int m_anchorItem;
    KItemSet m_selectedItems;
    bool m_isAnchoredSelectionActive;

    KItemModelBase* m_model;

    friend class KItemListController; // Calls setModel()
    friend class KItemListView;       // Calls itemsInserted(), itemsRemoved() and itemsMoved()
    friend class KItemListSelectionManagerTest;
};

#endif
