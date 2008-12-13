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

#ifndef DOLPHINVIEWAUTOSCROLLER_H
#define DOLPHINVIEWAUTOSCROLLER_H

#include <QObject>

class QAbstractItemView;
class QTimer;

/**
 * @brief Assures that an autoscrolling is done for item views.
 *
 * This is a workaround as QAbstractItemView::setAutoScroll() is not usable
 * when selecting items (see Qt issue #214542).
 */
class DolphinViewAutoScroller : public QObject
{
    Q_OBJECT

public:
    DolphinViewAutoScroller(QAbstractItemView* parent);
    virtual ~DolphinViewAutoScroller();

protected:
    virtual bool eventFilter(QObject* watched, QEvent* event);

private slots:
    void scrollViewport();

private:
    void triggerAutoScroll();
    void stopAutoScroll();

    /**
     * Calculates the scroll increment m_scrollInc dependent from
     * the cursor position \a cursorPos and the range 0 - \a rangeSize - 1.
     */
    void calculateScrollIncrement(int cursorPos, int rangeSize);

private:
    bool m_rubberBandSelection;
    int m_scrollInc;
    QAbstractItemView* m_itemView;
    QTimer* m_timer;
};

#endif
