/*
 * SPDX-FileCopyrightText: 2020 Steffen Hartleib <steffenhartleib@t-online.de>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KTWOFINGERSWIPE_H
#define KTWOFINGERSWIPE_H

#include "dolphin_export.h"
// Qt
#include <QGesture>
#include <QGestureRecognizer>

class DOLPHIN_EXPORT KTwoFingerSwipe : public QGesture
{
    Q_OBJECT
    Q_PROPERTY(QPointF pos READ pos WRITE setPos)
    Q_PROPERTY(QPointF screenPos READ screenPos WRITE setScreenPos)
    Q_PROPERTY(QPointF scenePos READ scenePos WRITE setScenePos)
    Q_PROPERTY(qreal swipeAngle READ swipeAngle WRITE setSwipeAngle)
public:
    explicit KTwoFingerSwipe(QObject* parent = nullptr);
    ~KTwoFingerSwipe() override;
    QPointF pos() const;
    void setPos(QPointF pos);
    QPointF screenPos() const;
    void setScreenPos(QPointF screenPos);
    QPointF scenePos() const;
    void setScenePos(QPointF scenePos);
    qreal swipeAngle() const;
    void setSwipeAngle(qreal swipeAngle);
private:
    QPointF m_pos;
    QPointF m_screenPos;
    QPointF m_scenePos;
    qreal m_swipeAngle;
};

class DOLPHIN_EXPORT KTwoFingerSwipeRecognizer : public QGestureRecognizer
{
public:
    explicit KTwoFingerSwipeRecognizer();
    ~KTwoFingerSwipeRecognizer() override;
    QGesture* create(QObject*) override;
    Result recognize(QGesture*, QObject*, QEvent*) override;
private:
    Q_DISABLE_COPY( KTwoFingerSwipeRecognizer )
    qint64 m_touchBeginnTimestamp;
    bool m_gestureAlreadyTriggered;
};

#endif /* KTWOFINGERSWIPE_H */

