/*
 * SPDX-FileCopyrightText: 2011 Peter Penz <peter.penz19@gmail.com>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kitemlistrubberband.h"

KItemListRubberBand::KItemListRubberBand(QObject* parent) :
    QObject(parent),
    m_active(false),
    m_startPos(),
    m_endPos()
{
}

KItemListRubberBand::~KItemListRubberBand()
{
}

void KItemListRubberBand::setStartPosition(const QPointF& pos)
{
    if (m_startPos != pos) {
        const QPointF previous = m_startPos;
        m_startPos = pos;
        emit startPositionChanged(m_startPos, previous);
    }
}

QPointF KItemListRubberBand::startPosition() const
{
    return m_startPos;
}

void KItemListRubberBand::setEndPosition(const QPointF& pos)
{
    if (m_endPos != pos) {
        const QPointF previous = m_endPos;
        m_endPos = pos;

        if (m_startPos.x() == m_endPos.x()) {
            if (previous.x() < m_startPos.x()) {
                m_endPos.rx() = m_startPos.x() - 1.0;
            } else {
                m_endPos.rx() = m_startPos.x() + 1.0;
            }
        }
        if (m_startPos.y() == m_endPos.y()) {
            if (previous.y() < m_startPos.y()) {
                m_endPos.ry() = m_startPos.y() - 1.0;
            } else {
                m_endPos.ry() = m_startPos.y() + 1.0;
            }
        }

        emit endPositionChanged(m_endPos, previous);
    }
}

QPointF KItemListRubberBand::endPosition() const
{
    return m_endPos;
}

void KItemListRubberBand::setActive(bool active)
{
    if (m_active != active) {
        m_active = active;
        emit activationChanged(active);
    }
}

bool KItemListRubberBand::isActive() const
{
    return m_active;
}

