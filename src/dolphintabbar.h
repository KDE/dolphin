/***************************************************************************
 * Copyright (C) 2014 by Emmanuel Pescosta <emmanuelpescosta099@gmail.com> *
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

#ifndef DOLPHIN_TAB_BAR_H
#define DOLPHIN_TAB_BAR_H

#include <QTabBar>

class DolphinTabBar : public QTabBar
{
    Q_OBJECT

public:
    explicit DolphinTabBar(QWidget* parent);

signals:
    void openNewActivatedTab(int index);
    void tabDropEvent(int index, QDropEvent* event);
    void tabDetachRequested(int index);

protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;

    /**
     * Opens a context menu for the tab on the \a event position.
     */
    void contextMenuEvent(QContextMenuEvent* event) override;

private slots:
    void slotAutoActivationTimeout();

private:
    /**
     * If \a index is a valid index (>= 0), store the index and start the timer
     * (if the interval >= 0 ms). If the index is not valid (< 0), stop the timer.
     */
    void updateAutoActivationTimer(const int index);

private:
    QTimer* m_autoActivationTimer;
    int m_autoActivationIndex;
    int m_tabToBeClosedOnMiddleMouseButtonRelease;
};

#endif // DOLPHIN_TAB_BAR_H
