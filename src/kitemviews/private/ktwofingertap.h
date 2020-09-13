/*
 * SPDX-FileCopyrightText: 2020 Steffen Hartleib <steffenhartleib@t-online.de>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KTWOFINGERTAP_H
#define KTWOFINGERTAP_H

#include <dolphin_export.h>
// Qt
#include <QGesture>
#include <QGestureRecognizer>

class DOLPHIN_EXPORT KTwoFingerTap : public QGesture
{
    Q_OBJECT
    Q_PROPERTY(QPointF pos READ pos WRITE setPos)
    Q_PROPERTY(QPointF screenPos READ screenPos WRITE setScreenPos)
    Q_PROPERTY(QPointF scenePos READ scenePos WRITE setScenePos)
public:
    explicit KTwoFingerTap(QObject* parent = nullptr);
    ~KTwoFingerTap();
    QPointF pos() const;
    void setPos(QPointF pos);
    QPointF screenPos() const;
    void setScreenPos(QPointF screenPos);
    QPointF scenePos() const;
    void setScenePos(QPointF scenePos);
private:
    QPointF m_pos;
    QPointF m_screenPos;
    QPointF m_scenePos;
};

class DOLPHIN_EXPORT KTwoFingerTapRecognizer : public QGestureRecognizer
{
public:
    explicit KTwoFingerTapRecognizer();
    ~KTwoFingerTapRecognizer();
    QGesture* create(QObject*) override; 
    Result recognize(QGesture*, QObject*, QEvent*) override;
private:
    Q_DISABLE_COPY(KTwoFingerTapRecognizer)
    bool m_gestureTriggered;
};

#endif /* KTWOFINGERTAP_H */
