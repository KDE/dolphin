/***************************************************************************
 *   Copyright (C) 2008 by Peter Penz <peter.penz@gmx.at>                  *
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

#ifndef SELECTIONMANAGER_H
#define SELECTIONMANAGER_H

#include <kfileitem.h>

#include <QObject>

class DolphinSortFilterProxyModel;
class QAbstractItemView;
class QModelIndex;
class QAbstractButton;

/**
 * @brief Allows to select and deselect items for the single-click mode.
 *
 * Whenever an item is hovered by the mouse, a toggle button is shown
 * which allows to select/deselect the current item.
 */
class SelectionManager : public QObject
{
    Q_OBJECT

public:
    SelectionManager(QAbstractItemView* parent);
    virtual ~SelectionManager();

signals:
    /** Is emitted if the selection has been changed by the toggle button. */
    void selectionChanged();

private slots:
    void slotEntered(const QModelIndex& index);
    void slotViewportEntered();
    void slotSelectionChanged();
    void setItemSelected(bool selected);

private:
    KFileItem itemForIndex(const QModelIndex& index) const;
    const QModelIndex indexForItem(const KFileItem& item) const;

private:
    QAbstractItemView* m_view;
    QAbstractButton* m_button;
    KFileItem m_item;
};

#endif
