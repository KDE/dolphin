/*
 * SPDX-FileCopyrightText: 2020 Steffen Hartleib <steffenhartleib@t-online.de>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

// Self
#include "ktwofingertap.h"

// Qt
#include <QTouchEvent>
#include <QApplication>
#include <QGraphicsWidget>

KTwoFingerTapRecognizer::KTwoFingerTapRecognizer() :
    QGestureRecognizer(),
    m_gestureTriggered(false)
{
}

KTwoFingerTapRecognizer::~KTwoFingerTapRecognizer()
{
}

QGesture* KTwoFingerTapRecognizer::create(QObject*)
{
    return static_cast<QGesture*>(new KTwoFingerTap());
}

QGestureRecognizer::Result KTwoFingerTapRecognizer::recognize(QGesture* gesture, QObject* watched, QEvent* event)
{
    if (qobject_cast<QGraphicsWidget*>(watched)) {
        return Ignore;
    }

    KTwoFingerTap* const kTwoFingerTap = static_cast<KTwoFingerTap*>(gesture);
    const QTouchEvent* touchEvent = static_cast<const QTouchEvent*>(event);

    switch (event->type()) {
    case QEvent::TouchBegin: {
        kTwoFingerTap->setHotSpot(touchEvent->touchPoints().first().startScreenPos());
        kTwoFingerTap->setPos(touchEvent->touchPoints().first().startPos());
        kTwoFingerTap->setScreenPos(touchEvent->touchPoints().first().startScreenPos());
        kTwoFingerTap->setScenePos(touchEvent->touchPoints().first().startScenePos());
        m_gestureTriggered = false;
        return MayBeGesture;
    }

    case QEvent::TouchUpdate: {

        if (touchEvent->touchPoints().size() > 2) {
            m_gestureTriggered = false;
            return CancelGesture;
        }

        if (touchEvent->touchPoints().size() == 2) {
            if ((touchEvent->touchPoints().first().startPos() - touchEvent->touchPoints().first().pos()).manhattanLength() >= QApplication::startDragDistance()) {
                m_gestureTriggered = false;
                return CancelGesture;
            }
            if ((touchEvent->touchPoints().at(1).startPos() - touchEvent->touchPoints().at(1).pos()).manhattanLength() >= QApplication::startDragDistance()) {
                m_gestureTriggered = false;
                return CancelGesture;
            }
            if (touchEvent->touchPointStates() & Qt::TouchPointPressed) {
                m_gestureTriggered = true;
            }
            if (touchEvent->touchPointStates() & Qt::TouchPointReleased && m_gestureTriggered) {
                m_gestureTriggered = false;
                return FinishGesture;
            }
        }
        break;
    }

    default:
        return Ignore;
    }
    return Ignore;
}

KTwoFingerTap::KTwoFingerTap(QObject* parent) :
    QGesture(parent),
    m_pos(QPointF(-1, -1)),
    m_screenPos(QPointF(-1, -1)),
    m_scenePos(QPointF(-1, -1))
{
}

KTwoFingerTap::~KTwoFingerTap()
{
}

QPointF KTwoFingerTap::pos() const
{
    return m_pos;
}

void KTwoFingerTap::setPos(QPointF _pos)
{
    m_pos = _pos;
}

QPointF KTwoFingerTap::screenPos() const
{
    return m_screenPos;
}

void KTwoFingerTap::setScreenPos(QPointF _screenPos)
{
    m_screenPos = _screenPos;
}

QPointF KTwoFingerTap::scenePos() const
{
    return m_scenePos;
}

void KTwoFingerTap::setScenePos(QPointF _scenePos)
{
    m_scenePos = _scenePos;
}
