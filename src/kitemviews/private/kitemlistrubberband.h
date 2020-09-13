/*
 * SPDX-FileCopyrightText: 2011 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KITEMLISTRUBBERBAND_H
#define KITEMLISTRUBBERBAND_H

#include "dolphin_export.h"

#include <QObject>
#include <QPointF>

/**
 * @brief Manages the rubberband when selecting items.
 */
class DOLPHIN_EXPORT KItemListRubberBand : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QPointF endPosition MEMBER m_endPos READ endPosition WRITE setEndPosition)

public:
    explicit KItemListRubberBand(QObject* parent = nullptr);
    ~KItemListRubberBand() override;

    void setStartPosition(const QPointF& pos);
    QPointF startPosition() const;

    void setEndPosition(const QPointF& pos);
    QPointF endPosition() const;

    void setActive(bool active);
    bool isActive() const;

signals:
    void activationChanged(bool active);
    void startPositionChanged(const QPointF& current, const QPointF& previous);
    void endPositionChanged(const QPointF& current, const QPointF& previous);

private:
    bool m_active;
    QPointF m_startPos;
    QPointF m_endPos;
};

#endif


