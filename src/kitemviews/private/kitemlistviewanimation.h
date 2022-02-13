/*
 * SPDX-FileCopyrightText: 2011 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KITEMLISTVIEWANIMATION_H
#define KITEMLISTVIEWANIMATION_H

#include "dolphin_export.h"

#include <QHash>
#include <QObject>
#include <QVariant>

class KItemListView;
class QGraphicsWidget;
class QPropertyAnimation;

/**
 * @brief Internal helper class for KItemListView to animate the items.
 *
 * Supports item animations for moving, creating, deleting and resizing
 * an item. Several applications can be applied to one item in parallel.
 */
class DOLPHIN_EXPORT KItemListViewAnimation : public QObject
{
    Q_OBJECT

public:
    enum AnimationType {
        MovingAnimation,
        CreateAnimation,
        DeleteAnimation,
        ResizeAnimation,
        IconResizeAnimation,
        AnimationTypeCount
    };

    explicit KItemListViewAnimation(QObject* parent = nullptr);
    ~KItemListViewAnimation() override;

    void setScrollOrientation(Qt::Orientation orientation);
    Qt::Orientation scrollOrientation() const;

    void setScrollOffset(qreal scrollOffset);
    qreal scrollOffset() const;

    /**
     * Starts the animation of the type \a type for the widget \a widget. If an animation
     * of the type is already running, this animation will be stopped before starting
     * the new animation.
     */
    void start(QGraphicsWidget* widget, AnimationType type, const QVariant& endValue = QVariant());

    /**
     * Stops the animation of the type \a type for the widget \a widget.
     */
    void stop(QGraphicsWidget* widget, AnimationType type);

    /**
     * Stops all animations that have been applied to the widget \a widget.
     */
    void stop(QGraphicsWidget* widget);

    /**
     * @return True if the animation of the type \a type has been started
     *         for the widget \a widget..
     */
    bool isStarted(QGraphicsWidget *widget, AnimationType type) const;

    /**
     * @return True if any animation has been started for the widget.
     */
    bool isStarted(QGraphicsWidget* widget) const;

Q_SIGNALS:
    void finished(QGraphicsWidget* widget, KItemListViewAnimation::AnimationType type);

private Q_SLOTS:
    void slotFinished();

private:
    Qt::Orientation m_scrollOrientation;
    qreal m_scrollOffset;
    QHash<QGraphicsWidget*, QPropertyAnimation*> m_animation[AnimationTypeCount];
};

#endif


