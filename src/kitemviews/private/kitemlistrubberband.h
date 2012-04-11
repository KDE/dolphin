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

#ifndef KITEMLISTRUBBERBAND_H
#define KITEMLISTRUBBERBAND_H

#include <libdolphin_export.h>
#include <QObject>
#include <QPointF>

/**
 * @brief Manages the rubberband when selecting items.
 */
class LIBDOLPHINPRIVATE_EXPORT KItemListRubberBand : public QObject
{
    Q_OBJECT

public:
    explicit KItemListRubberBand(QObject* parent = 0);
    virtual ~KItemListRubberBand();

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


