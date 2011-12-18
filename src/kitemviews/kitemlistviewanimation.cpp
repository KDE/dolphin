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

#include "kitemlistviewanimation_p.h"

#include "kitemlistview.h"

#include <KDebug>
#include <KGlobalSettings>

#include <QGraphicsWidget>
#include <QPropertyAnimation>

KItemListViewAnimation::KItemListViewAnimation(QObject* parent) :
    QObject(parent),
    m_animationDuration(200),
    m_scrollOrientation(Qt::Vertical),
    m_scrollOffset(0),
    m_animation()
{
    if (KGlobalSettings::graphicEffectsLevel() == KGlobalSettings::NoEffects) {
        m_animationDuration = 1;
    }
}

KItemListViewAnimation::~KItemListViewAnimation()
{
    for (int type = 0; type < AnimationTypeCount; ++type) {
        qDeleteAll(m_animation[type]);
    }
}

void KItemListViewAnimation::setScrollOrientation(Qt::Orientation orientation)
{
    m_scrollOrientation = orientation;
}

Qt::Orientation KItemListViewAnimation::scrollOrientation() const
{
    return m_scrollOrientation;
}

void KItemListViewAnimation::setScrollOffset(qreal offset)
{
    const qreal diff = m_scrollOffset - offset;
    m_scrollOffset = offset;

    // The change of the offset requires that the position of all
    // animated QGraphicsWidgets get adjusted. An exception is made
    // for the delete animation that should just fade away on the
    // existing position.
    for (int type = 0; type < AnimationTypeCount; ++type) {
        if (type == DeleteAnimation) {
            continue;
        }

        QHashIterator<QGraphicsWidget*, QPropertyAnimation*>  it(m_animation[type]);
        while (it.hasNext()) {
            it.next();

            QGraphicsWidget* widget = it.key();
            QPropertyAnimation* propertyAnim = it.value();

            QPointF currentPos = widget->pos();
            if (m_scrollOrientation == Qt::Vertical) {
                currentPos.ry() += diff;
            } else {
                currentPos.rx() += diff;
            }

            if (type == MovingAnimation) {
                // Stop the animation, calculate the moved start- and end-value
                // and restart the animation for the remaining duration.
                const int remainingDuration = propertyAnim->duration()
                                              - propertyAnim->currentTime();

                const bool block = propertyAnim->signalsBlocked();
                propertyAnim->blockSignals(true);
                propertyAnim->stop();

                QPointF endPos = propertyAnim->endValue().toPointF();
                if (m_scrollOrientation == Qt::Vertical) {
                    endPos.ry() += diff;
                } else {
                    endPos.rx() += diff;
                }

                propertyAnim->setDuration(remainingDuration);
                propertyAnim->setStartValue(currentPos);
                propertyAnim->setEndValue(endPos);
                propertyAnim->start();
                propertyAnim->blockSignals(block);
            } else {
                widget->setPos(currentPos);
            }
        }
    }
}

qreal KItemListViewAnimation::scrollOffset() const
{
    return m_scrollOffset;
}

void KItemListViewAnimation::start(QGraphicsWidget* widget, AnimationType type, const QVariant& endValue)
{
    stop(widget, type);

    QPropertyAnimation* propertyAnim = 0;

    switch (type) {
    case MovingAnimation: {
        const QPointF newPos = endValue.toPointF();
        if (newPos == widget->pos()) {
            return;
        }

        propertyAnim = new QPropertyAnimation(widget, "pos");
        propertyAnim->setDuration(m_animationDuration);
        propertyAnim->setEndValue(newPos);
        break;
    }

    case CreateAnimation: {
        propertyAnim = new QPropertyAnimation(widget, "opacity");
        propertyAnim->setEasingCurve(QEasingCurve::InQuart);
        propertyAnim->setDuration(m_animationDuration);
        propertyAnim->setStartValue(0.0);
        propertyAnim->setEndValue(1.0);
        break;
    }

    case DeleteAnimation: {
        propertyAnim = new QPropertyAnimation(widget, "opacity");
        propertyAnim->setEasingCurve(QEasingCurve::OutQuart);
        propertyAnim->setDuration(m_animationDuration);
        propertyAnim->setStartValue(1.0);
        propertyAnim->setEndValue(0.0);
        break;
    }

    case ResizeAnimation: {
        const QSizeF newSize = endValue.toSizeF();
        if (newSize == widget->size()) {
            return;
        }

        propertyAnim = new QPropertyAnimation(widget, "size");
        propertyAnim->setDuration(m_animationDuration);
        propertyAnim->setEndValue(newSize);
        break;
    }

    default:
        break;
    }

    Q_ASSERT(propertyAnim);
    connect(propertyAnim, SIGNAL(finished()), this, SLOT(slotFinished()));
    m_animation[type].insert(widget, propertyAnim);

    propertyAnim->start();
}

void KItemListViewAnimation::stop(QGraphicsWidget* widget, AnimationType type)
{
    QPropertyAnimation* propertyAnim = m_animation[type].value(widget);
    if (propertyAnim) {
        propertyAnim->stop();

        switch (type) {
        case MovingAnimation: break;
        case CreateAnimation: widget->setOpacity(1.0); break;
        case DeleteAnimation: widget->setOpacity(0.0); break;
        case ResizeAnimation: break;
        default: break;
        }

        m_animation[type].remove(widget);
        delete propertyAnim;

        emit finished(widget, type);
    }
}

void KItemListViewAnimation::stop(QGraphicsWidget* widget)
{
    for (int type = 0; type < AnimationTypeCount; ++type) {
        stop(widget, static_cast<AnimationType>(type));
    }
}

bool KItemListViewAnimation::isStarted(QGraphicsWidget *widget, AnimationType type) const
{
    return m_animation[type].value(widget);
}

bool KItemListViewAnimation::isStarted(QGraphicsWidget* widget) const
{
    for (int type = 0; type < AnimationTypeCount; ++type) {
        if (isStarted(widget, static_cast<AnimationType>(type))) {
            return true;
        }
    }
    return false;
}

void KItemListViewAnimation::slotFinished()
{
    QPropertyAnimation* finishedAnim = qobject_cast<QPropertyAnimation*>(sender());
    for (int type = 0; type < AnimationTypeCount; ++type) {
        QHashIterator<QGraphicsWidget*, QPropertyAnimation*> it(m_animation[type]);
        while (it.hasNext()) {
            it.next();
            QPropertyAnimation* propertyAnim = it.value();
            if (propertyAnim == finishedAnim) {
                QGraphicsWidget* widget = it.key();
                m_animation[type].remove(widget);
                finishedAnim->deleteLater();

                emit finished(widget, static_cast<AnimationType>(type));
                return;
            }
        }
    }
    Q_ASSERT(false);
}

#include "kitemlistviewanimation_p.moc"
