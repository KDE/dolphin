/***************************************************************************
 *   Copyright (C) 2011 by Peter Penz <peter.penz19@gmail.com>             *
 *   Copyright (C) 2011 by Frank Reininghaus <frank78ac@googlemail.com>    *
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

#include "kitemlistselectionmanager.h"

#include "kitemmodelbase.h"
#include <KDebug>

KItemListSelectionManager::KItemListSelectionManager(QObject* parent) :
    QObject(parent),
    m_currentItem(-1),
    m_anchorItem(-1),
    m_selectedItems(),
    m_isAnchoredSelectionActive(false),
    m_model(0)
{
}

KItemListSelectionManager::~KItemListSelectionManager()
{
}

void KItemListSelectionManager::setCurrentItem(int current)
{
    const int previous = m_currentItem;
    if (m_model && current >= 0 && current < m_model->count()) {
        m_currentItem = current;
    } else {
        m_currentItem = -1;
    }

    if (m_currentItem != previous) {
        emit currentChanged(m_currentItem, previous);
    }
}

int KItemListSelectionManager::currentItem() const
{
    return m_currentItem;
}

void KItemListSelectionManager::setSelectedItems(const QSet<int>& items)
{
    if (m_selectedItems != items) {
        const QSet<int> previous = m_selectedItems;
        m_selectedItems = items;
        emit selectionChanged(m_selectedItems, previous);
    }
}

QSet<int> KItemListSelectionManager::selectedItems() const
{
    QSet<int> selectedItems = m_selectedItems;

    if (m_isAnchoredSelectionActive) {
        const int from = qMin(m_anchorItem, m_currentItem);
        const int to = qMax(m_anchorItem, m_currentItem);

        for (int index = from; index <= to; index++) {
            selectedItems.insert(index);
        }
    }

    return selectedItems;
}

bool KItemListSelectionManager::hasSelection() const
{
    return !m_selectedItems.isEmpty() || m_isAnchoredSelectionActive;
}

void KItemListSelectionManager::setSelected(int index, int count, SelectionMode mode)
{
    if (index < 0 || count < 1 || !m_model || index >= m_model->count()) {
        return;
    }

    const QSet<int> previous = m_selectedItems;

    count = qMin(count, m_model->count() - index);

    const int endIndex = index + count -1;
    switch (mode) {
    case Select:
        for (int i = index; i <= endIndex; ++i) {
            m_selectedItems.insert(i);
        }
        break;

    case Deselect:
        for (int i = index; i <= endIndex; ++i) {
            m_selectedItems.remove(i);
        }
        break;

    case Toggle:
        for (int i = index; i <= endIndex; ++i) {
            if (m_selectedItems.contains(i)) {
                m_selectedItems.remove(i);
            } else {
                m_selectedItems.insert(i);
            }
        }
        break;

    default:
        Q_ASSERT(false);
        break;
    }

    if (m_selectedItems != previous) {
        emit selectionChanged(m_selectedItems, previous);
    }
}

void KItemListSelectionManager::clearSelection()
{
    if (!m_selectedItems.isEmpty()) {
        const QSet<int> previous = m_selectedItems;
        m_selectedItems.clear();
        m_isAnchoredSelectionActive = false;
        emit selectionChanged(m_selectedItems, previous);
    }
    else if (m_isAnchoredSelectionActive) {
        m_isAnchoredSelectionActive = false;
        // TODO: the 'previous' parameter of the signal has to be set correctly, but do we actually need it?
        emit selectionChanged(m_selectedItems, m_selectedItems);
    }
}

void KItemListSelectionManager::beginAnchoredSelection(int anchor)
{
    m_isAnchoredSelectionActive = true;
    setAnchorItem(anchor);
}

void KItemListSelectionManager::endAnchoredSelection()
{
    if (m_isAnchoredSelectionActive) {
        const int from = qMin(m_anchorItem, m_currentItem);
        const int to = qMax(m_anchorItem, m_currentItem);

        for (int index = from; index <= to; index++) {
            m_selectedItems.insert(index);
        }

        m_isAnchoredSelectionActive = false;
    }
}

void KItemListSelectionManager::setAnchorItem(int anchor)
{
    const int previous = m_anchorItem;
    if (m_model && anchor < m_model->count()) {
        m_anchorItem = anchor;
    } else {
        m_anchorItem = -1;
    }

    if (m_anchorItem != previous) {
        emit anchorChanged(m_anchorItem, previous);
    }
}

int KItemListSelectionManager::anchorItem() const
{
    return m_anchorItem;
}

bool KItemListSelectionManager::isAnchoredSelectionActive() const
{
    return m_isAnchoredSelectionActive;
}

void KItemListSelectionManager::setAnchoredSelectionActive(bool active)
{
    m_isAnchoredSelectionActive = active;
}

KItemModelBase* KItemListSelectionManager::model() const
{
    return m_model;
}

void KItemListSelectionManager::setModel(KItemModelBase* model)
{
    m_model = model;
    if (model && model->count() > 0) {
        m_currentItem = 0;
    }
}

void KItemListSelectionManager::itemsInserted(const KItemRangeList& itemRanges)
{
    // Update the current item
    if (m_currentItem < 0) {
        setCurrentItem(0);
    } else {
        int inc = 0;
        foreach (const KItemRange& itemRange, itemRanges) {
            if (m_currentItem < itemRange.index) {
                break;
            }
            inc += itemRange.count;
        }
        setCurrentItem(m_currentItem + inc);
    }

    // Update the anchor item
    if (m_anchorItem < 0) {
        setAnchorItem(0);
    } else {
        int inc = 0;
        foreach (const KItemRange& itemRange, itemRanges) {
            if (m_anchorItem < itemRange.index) {
                break;
            }
            inc += itemRange.count;
        }
        setAnchorItem(m_anchorItem + inc);
    }

    // Update the selections
    if (!m_selectedItems.isEmpty()) {
        const QSet<int> previous = m_selectedItems;

        QSet<int> current;
        current.reserve(m_selectedItems.count());
        QSetIterator<int> it(m_selectedItems);
        while (it.hasNext()) {
            const int index = it.next();
            int inc = 0;
            foreach (const KItemRange& itemRange, itemRanges) {
                if (index < itemRange.index) {
                    break;
                }
                inc += itemRange.count;
            }
            current.insert(index + inc);
        }

        if (current != previous) {
            m_selectedItems = current;
            emit selectionChanged(current, previous);
        }
    }
}

void KItemListSelectionManager::itemsRemoved(const KItemRangeList& itemRanges)
{
    // Update the current item
    if (m_currentItem >= 0) {
        int currentItem = m_currentItem;
        foreach (const KItemRange& itemRange, itemRanges) {
            if (currentItem < itemRange.index) {
                break;
            }
            if (currentItem >= itemRange.index + itemRange.count) {
                currentItem -= itemRange.count;
            } else if (currentItem >= m_model->count()) {
                currentItem = m_model->count() - 1;
            }
        }
        setCurrentItem(currentItem);
    }

    // Update the anchor item
    if (m_anchorItem >= 0) {
        int anchorItem = m_anchorItem;
        foreach (const KItemRange& itemRange, itemRanges) {
            if (anchorItem < itemRange.index) {
                break;
            }
            if (anchorItem >= itemRange.index + itemRange.count) {
                anchorItem -= itemRange.count;
            } else if (anchorItem >= m_model->count()) {
                anchorItem = m_model->count() - 1;
            }
        }
        setAnchorItem(anchorItem);
    }

    // Update the selections
    if (!m_selectedItems.isEmpty()) {
        const QSet<int> previous = m_selectedItems;

        QSet<int> current;
        current.reserve(m_selectedItems.count());
        QSetIterator<int> it(m_selectedItems);
        while (it.hasNext()) {
            int index = it.next();
            int dec = 0;
            foreach (const KItemRange& itemRange, itemRanges) {
                if (index < itemRange.index) {
                    break;
                }

                if (index < itemRange.index + itemRange.count) {
                    // The selection is part of the removed range
                    // and will get deleted
                    index = -1;
                    break;
                }

                dec += itemRange.count;
            }
            index -= dec;
            if (index >= 0)  {
                current.insert(index);
            }
        }

        if (current != previous) {
            m_selectedItems = current;
            emit selectionChanged(current, previous);
        }
    }
}

#include "kitemlistselectionmanager.moc"
