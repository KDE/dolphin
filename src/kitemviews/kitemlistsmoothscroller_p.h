/***************************************************************************
 *   Copyright (C) 2011 by Peter Penz <peter.penz19@gmail.com>             *
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

#ifndef KITEMLISTSMOOTHSCROLLER_H
#define KITEMLISTSMOOTHSCROLLER_H

#include <libdolphin_export.h>

#include <QAbstractAnimation>
#include <QObject>

class QPropertyAnimation;
class QScrollBar;
class QWheelEvent;

/**
 * @brief Helper class for KItemListContainer to have a smooth
 *        scrolling when adjusting the scrollbars.
 */
class LIBDOLPHINPRIVATE_EXPORT KItemListSmoothScroller : public QObject
{
    Q_OBJECT

public:
    KItemListSmoothScroller(QScrollBar* scrollBar,
                            QObject* parent = 0);
    virtual ~KItemListSmoothScroller();

    void setScrollBar(QScrollBar* scrollBar);
    QScrollBar* scrollBar() const;

    void setTargetObject(QObject* target);
    QObject* targetObject() const;

    void setPropertyName(const QByteArray& propertyName);
    QByteArray propertyName() const;

    /**
     * Adjusts the position of the target by \p distance
     * pixels. Is invoked in the context of QAbstractScrollArea::scrollContentsBy()
     * where the scrollbars already have the new position but the content
     * has not been scrolled yet.
     */
    void scrollContentsBy(qreal distance);

    /**
     * Does a smooth-scrolling to the position \p position
     * on the target and also adjusts the corresponding scrollbar
     * to the new position.
     */
    void scrollTo(qreal position);

    /**
     * Must be invoked before the scrollbar should get updated to have a new
     * maximum. True is returned if the new maximum can be applied. If false
     * is returned the maximum has already been reached and the value will
     * be reached at the end of the animation.
     */
    // TODO: This interface is tricky to understand. Try to make this more
    // generic/readable if the corresponding code in KItemListContainer got
    // stable.
    bool requestScrollBarUpdate(int newMaximum);

protected:
    virtual bool eventFilter(QObject* obj, QEvent* event);

private slots:
    void slotAnimationStateChanged(QAbstractAnimation::State newState,
                                   QAbstractAnimation::State oldState);

private:
    /**
     * Results into a smooth-scrolling of the target dependent on the direction
     * of the wheel event.
     */
    void handleWheelEvent(QWheelEvent* event);

private:
    bool m_scrollBarPressed;
    bool m_smoothScrolling;
    QScrollBar* m_scrollBar;
    QPropertyAnimation* m_animation;
};

#endif


