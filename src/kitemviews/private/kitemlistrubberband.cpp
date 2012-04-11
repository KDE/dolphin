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

#include "kitemlistrubberband.moc"
