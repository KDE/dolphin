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

#include <KFileItem>

#include <QObject>

class QAbstractItemView;
class QModelIndex;
class QItemSelection;
class SelectionToggle;

/**
 * @brief Allows to select and deselect items for item views.
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
    virtual bool eventFilter(QObject* watched, QEvent* event);

public slots:
    /**
     * Resets the selection manager so that the toggle button gets
     * invisible.
     */
    void reset();

signals:
    /** Is emitted if the selection has been changed by the toggle button. */
    void selectionChanged();

private slots:
    void slotEntered(const QModelIndex& index);
    void slotViewportEntered();
    void setItemSelected(bool selected);
    void slotRowsRemoved(const QModelIndex& parent, int start, int end);
    void slotSelectionChanged(const QItemSelection& selected, const QItemSelection& deselected);

private:
    KUrl urlForIndex(const QModelIndex& index) const;
    const QModelIndex indexForUrl(const KUrl& url) const;
    void applyPointingHandCursor();
    void restoreCursor();

private:
    QAbstractItemView* m_view;
    SelectionToggle* m_toggle;
    bool m_connected;
    bool m_appliedPointingHandCursor;
};

#endif
