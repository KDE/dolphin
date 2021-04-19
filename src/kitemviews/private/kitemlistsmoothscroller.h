/*
 * SPDX-FileCopyrightText: 2011 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KITEMLISTSMOOTHSCROLLER_H
#define KITEMLISTSMOOTHSCROLLER_H

#include "dolphin_export.h"

#include <QAbstractAnimation>
#include <QObject>

class QPropertyAnimation;
class QScrollBar;
class QWheelEvent;

/**
 * @brief Helper class for KItemListContainer to have a smooth
 *        scrolling when adjusting the scrollbars.
 */
class DOLPHIN_EXPORT KItemListSmoothScroller : public QObject
{
    Q_OBJECT

public:
    explicit KItemListSmoothScroller(QScrollBar* scrollBar,
                                     QObject* parent = nullptr);
    ~KItemListSmoothScroller() override;

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

    /**
     * Forwards wheel events to the scrollbar, ensuring smooth and proper scrolling
     */
    void handleWheelEvent(QWheelEvent* event);

Q_SIGNALS:
    /**
     * Emitted when the scrolling animation has finished
     */
    void scrollingStopped();
protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private Q_SLOTS:
    void slotAnimationStateChanged(QAbstractAnimation::State newState,
                                   QAbstractAnimation::State oldState);

private:
    bool m_scrollBarPressed;
    bool m_smoothScrolling;
    QScrollBar* m_scrollBar;
    QPropertyAnimation* m_animation;
};

#endif


