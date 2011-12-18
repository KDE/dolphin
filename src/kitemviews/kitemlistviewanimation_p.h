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

#ifndef KITEMLISTVIEWANIMATION_H
#define KITEMLISTVIEWANIMATION_H

#include <libdolphin_export.h>

#include <QHash>
#include <QObject>
#include <QVariant>

class KItemListView;
class QGraphicsWidget;
class QPointF;
class QPropertyAnimation;

class LIBDOLPHINPRIVATE_EXPORT KItemListViewAnimation : public QObject
{
    Q_OBJECT

public:
    enum AnimationType {
        MovingAnimation,
        CreateAnimation,
        DeleteAnimation,
        ResizeAnimation
    };

    KItemListViewAnimation(QObject* parent = 0);
    virtual ~KItemListViewAnimation();

    void setScrollOrientation(Qt::Orientation orientation);
    Qt::Orientation scrollOrientation() const;

    void setScrollOffset(qreal scrollOffset);
    qreal scrollOffset() const;

    void start(QGraphicsWidget* widget, AnimationType type, const QVariant& endValue = QVariant());

    void stop(QGraphicsWidget* widget, AnimationType type);
    void stop(QGraphicsWidget* widget);

    bool isStarted(QGraphicsWidget *widget, AnimationType type) const;
    bool isStarted(QGraphicsWidget* widget) const;

signals:
    void finished(QGraphicsWidget* widget, KItemListViewAnimation::AnimationType type);

private slots:
    void slotFinished();

private:
    enum { AnimationTypeCount = 4 };

    int m_animationDuration;
    Qt::Orientation m_scrollOrientation;
    qreal m_scrollOffset;
    QHash<QGraphicsWidget*, QPropertyAnimation*> m_animation[AnimationTypeCount];
};

#endif


