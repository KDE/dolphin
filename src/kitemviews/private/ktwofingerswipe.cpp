/*
 * SPDX-FileCopyrightText: 2020 Steffen Hartleib <steffenhartleib@t-online.de>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

// Self
#include "ktwofingerswipe.h"

// Qt
#include <QTouchEvent>
#include <QLineF>

KTwoFingerSwipeRecognizer::KTwoFingerSwipeRecognizer() :
    QGestureRecognizer(),
    m_touchBeginnTimestamp(0),
    m_gestureAlreadyTriggered(false)
{
}

KTwoFingerSwipeRecognizer::~KTwoFingerSwipeRecognizer()
{
}

QGesture* KTwoFingerSwipeRecognizer::create(QObject*)
{
    return static_cast<QGesture*>(new KTwoFingerSwipe());
}

QGestureRecognizer::Result KTwoFingerSwipeRecognizer::recognize(QGesture* gesture, QObject* watched, QEvent* event)
{
    Q_UNUSED(watched)

    KTwoFingerSwipe* const kTwoFingerSwipe = static_cast<KTwoFingerSwipe*>(gesture);
    const QTouchEvent* touchEvent = static_cast<const QTouchEvent*>(event);

    const int maxTimeFrameForSwipe = 90;
    const int minDistanceForSwipe = 30;

    switch (event->type()) {
    case QEvent::TouchBegin: {
        m_touchBeginnTimestamp = touchEvent->timestamp();
        m_gestureAlreadyTriggered = false;
        kTwoFingerSwipe->setHotSpot(touchEvent->touchPoints().first().startScreenPos());
        kTwoFingerSwipe->setPos(touchEvent->touchPoints().first().startPos());
        kTwoFingerSwipe->setScreenPos(touchEvent->touchPoints().first().startScreenPos());
        kTwoFingerSwipe->setScenePos(touchEvent->touchPoints().first().startScenePos());
        return MayBeGesture;
    }

    case QEvent::TouchUpdate: {
        const qint64 now = touchEvent->timestamp();
        const qint64 elapsedTime = now - m_touchBeginnTimestamp;
        const QPointF distance = touchEvent->touchPoints().first().startPos() - touchEvent->touchPoints().first().pos();
        kTwoFingerSwipe->setHotSpot(touchEvent->touchPoints().first().startScreenPos());
        kTwoFingerSwipe->setPos(touchEvent->touchPoints().first().startPos());
        kTwoFingerSwipe->setScreenPos(touchEvent->touchPoints().first().startScreenPos());
        kTwoFingerSwipe->setScenePos(touchEvent->touchPoints().first().startScenePos());
        const QLineF ql = QLineF(touchEvent->touchPoints().first().startPos(), touchEvent->touchPoints().first().pos());
        kTwoFingerSwipe->setSwipeAngle(ql.angle());

        if (touchEvent->touchPoints().size() > 2) {
            return CancelGesture;
        }

        if (touchEvent->touchPoints().size() == 2) {
            if ((elapsedTime) > maxTimeFrameForSwipe) {
                return CancelGesture;
            }

            if (distance.manhattanLength() >= minDistanceForSwipe &&
                    (elapsedTime) <= maxTimeFrameForSwipe && !m_gestureAlreadyTriggered) {
                m_gestureAlreadyTriggered = true;
                return FinishGesture;
            } else if ((elapsedTime) <= maxTimeFrameForSwipe && !m_gestureAlreadyTriggered) {
                return TriggerGesture;
            }
        }
        break;
    }

    default:
        return Ignore;
    }
    return Ignore;
}

KTwoFingerSwipe::KTwoFingerSwipe(QObject* parent) :
    QGesture(parent),
    m_pos(QPointF(-1, -1)),
    m_screenPos(QPointF(-1, -1)),
    m_scenePos(QPointF(-1, -1)),
    m_swipeAngle(0.0)
{
}

KTwoFingerSwipe::~KTwoFingerSwipe()
{
}

QPointF KTwoFingerSwipe::pos() const
{
    return m_pos;
}

void KTwoFingerSwipe::setPos(QPointF _pos)
{
    m_pos = _pos;
}

QPointF KTwoFingerSwipe::screenPos() const
{
    return m_screenPos;
}

void KTwoFingerSwipe::setScreenPos(QPointF _screenPos)
{
    m_screenPos = _screenPos;
}

QPointF KTwoFingerSwipe::scenePos() const
{
    return m_scenePos;
}

void KTwoFingerSwipe::setScenePos(QPointF _scenePos)
{
    m_scenePos = _scenePos;
}

qreal KTwoFingerSwipe::swipeAngle() const
{
    return m_swipeAngle;
}
 void KTwoFingerSwipe::setSwipeAngle(qreal _swipeAngle)
{
    m_swipeAngle = _swipeAngle;
}

