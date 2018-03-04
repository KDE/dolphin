/***************************************************************************
 *   Copyright (C) 2011 by Peter Penz <peter.penz19@gmail.com>             *
 *                                                                         *
 *   Based on the Itemviews NG project from Trolltech Labs:                *
 *   http://qt.gitorious.org/qt-labs/itemviews-ng                          *
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
    void clearSelection();

    void beginAnchoredSelection(int anchor);
    void endAnchoredSelection();
    bool isAnchoredSelectionActive() const;

    KItemModelBase* model() const;

signals:
    void currentChanged(int current, int previous);
    void selectionChanged(const KItemSet& current, const KItemSet& previous);

private:
    void setModel(KItemModelBase* model);
    void itemsInserted(const KItemRangeList& itemRanges);
    void itemsRemoved(const KItemRangeList& itemRanges);
    void itemsMoved(const KItemRange& itemRange, const QList<int>& movedToIndexes);


    /**
     * Helper method for itemsRemoved. Returns the changed index after removing
     * the given range. If the index is part of the range, -1 will be returned.
     */
    int indexAfterRangesRemoving(int index, const KItemRangeList& itemRanges, const RangesRemovingBehaviour behaviour) const;

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
