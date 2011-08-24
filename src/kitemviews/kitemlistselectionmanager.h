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

#include <libdolphin_export.h>

#include <kitemviews/kitemmodelbase.h>

#include <QObject>
#include <QSet>

class KItemModelBase;

/**
 * @brief Allows to select and deselect items of a KItemListView.
 */
class LIBDOLPHINPRIVATE_EXPORT KItemListSelectionManager : public QObject
{
    Q_OBJECT

public:
    enum SelectionMode {
        Select,
        Deselect,
        Toggle
    };

    KItemListSelectionManager(QObject* parent = 0);
    virtual ~KItemListSelectionManager();

    void setCurrentItem(int current);
    int currentItem() const;

    void setSelectedItems(const QSet<int>& items);
    QSet<int> selectedItems() const;
    bool hasSelection() const;

    void setSelected(int index, int count = 1, SelectionMode mode = Select);
    void clearSelection();

    void beginAnchoredSelection(int anchor);
    void endAnchoredSelection();

    /**
     * Sets the anchor to \a anchor and emits anchorChanged() if the
     * anchor differs from the current anchor value. If no anchor selection is active (see
     * KItemListSelectionManager::beginAnchoredSelection()) or the index
     * is not within the available model items the anchor will not be modified.
     */
    void setAnchorItem(int anchor);
    int anchorItem() const;

    bool isAnchoredSelectionActive() const;

    KItemModelBase* model() const;

signals:
    void currentChanged(int current, int previous);
    void selectionChanged(const QSet<int>& current, const QSet<int>& previous);
    void anchorChanged(int anchor, int previous);

private:
    void setModel(KItemModelBase* model);
    void itemsInserted(const KItemRangeList& itemRanges);
    void itemsRemoved(const KItemRangeList& itemRanges);

private:
    int m_currentItem;
    int m_anchorItem;
    QSet<int> m_selectedItems;
    bool m_isAnchoredSelectionActive;

    KItemModelBase* m_model;

    friend class KItemListController; // Calls setModel()
    friend class KItemListView;       // Calls itemsInserted() and itemsRemoved()
    friend class KItemListSelectionManagerTest;
};

#endif
