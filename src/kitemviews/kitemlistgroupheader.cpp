/***************************************************************************
 *   Copyright (C) 2011 by Peter Penz <peter.penz19@gmail.com>             *
 *                                                                         *
 *   Based on the Itemviews NG project from Trolltech Labs:                *
 *   http://qt.gitorious.org/qt-labs/itemviews-ng                          *
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

#include "kitemlistgroupheader.h"

#include "kitemlistview.h"

#include <QGraphicsSceneResizeEvent>
#include <QPainter>
#include <QStyleOptionGraphicsItem>
#include <KDebug>

KItemListGroupHeader::KItemListGroupHeader(QGraphicsWidget* parent) :
    QGraphicsWidget(parent, 0),
    m_dirtyCache(true),
    m_role(),
    m_data(),
    m_styleOption(),
    m_scrollOrientation(Qt::Vertical),
    m_leftBorderCache(0),
    m_rightBorderCache(0),
    m_outlineColor()
{
}

KItemListGroupHeader::~KItemListGroupHeader()
{
    delete m_leftBorderCache;
    delete m_rightBorderCache;
}

void KItemListGroupHeader::setRole(const QByteArray& role)
{
    if (m_role != role) {
        const QByteArray previous = m_role;
        m_role = role;
        update();
        roleChanged(role, previous);
    }
}

QByteArray KItemListGroupHeader::role() const
{
    return m_role;
}

void KItemListGroupHeader::setData(const QVariant& data)
{
    if (m_data != data) {
        const QVariant previous = m_data;
        m_data = data;
        update();
        dataChanged(data, previous);
    }
}

QVariant KItemListGroupHeader::data() const
{
    return m_data;
}

void KItemListGroupHeader::setStyleOption(const KItemListStyleOption& option)
{
    const KItemListStyleOption previous = m_styleOption;
    m_styleOption = option;
    m_dirtyCache = true;
    styleOptionChanged(option, previous);
}

const KItemListStyleOption& KItemListGroupHeader::styleOption() const
{
    return m_styleOption;
}

void KItemListGroupHeader::setScrollOrientation(Qt::Orientation orientation)
{
    if (m_scrollOrientation != orientation) {
        const Qt::Orientation previous = m_scrollOrientation;
        m_scrollOrientation = orientation;
        m_dirtyCache = true;
        scrollOrientationChanged(orientation, previous);
    }
}

Qt::Orientation KItemListGroupHeader::scrollOrientation() const
{
    return m_scrollOrientation;
}

void KItemListGroupHeader::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    if (m_dirtyCache) {
        updateCache();
    }

    Q_UNUSED(option);
    Q_UNUSED(widget);

    const int leftBorderX = m_leftBorderCache->width() + 1;
    const int rightBorderX = size().width() - m_rightBorderCache->width() - 2;

    painter->setPen(m_outlineColor);
    painter->drawLine(leftBorderX, 1, rightBorderX, 1);

    painter->drawPixmap(1, 1, *m_leftBorderCache);
    painter->drawPixmap(rightBorderX, 1, *m_rightBorderCache);
}

void KItemListGroupHeader::roleChanged(const QByteArray& current, const QByteArray& previous)
{
    Q_UNUSED(current);
    Q_UNUSED(previous);
}

void KItemListGroupHeader::dataChanged(const QVariant& current, const QVariant& previous)
{
    Q_UNUSED(current);
    Q_UNUSED(previous);
}

void KItemListGroupHeader::styleOptionChanged(const KItemListStyleOption& current, const KItemListStyleOption& previous)
{
    Q_UNUSED(current);
    Q_UNUSED(previous);
}

void KItemListGroupHeader::scrollOrientationChanged(Qt::Orientation current, Qt::Orientation previous)
{
    Q_UNUSED(current);
    Q_UNUSED(previous);
}

void KItemListGroupHeader::resizeEvent(QGraphicsSceneResizeEvent* event)
{
    QGraphicsWidget::resizeEvent(event);
    if (event->oldSize().height() != event->newSize().height()) {
        m_dirtyCache = true;
    }
}

void KItemListGroupHeader::updateCache()
{
    Q_ASSERT(m_dirtyCache);

    delete m_leftBorderCache;
    delete m_rightBorderCache;

    const int length = qMax(int(size().height() - 1), 1);
    m_leftBorderCache = new QPixmap(length, length);
    m_leftBorderCache->fill(Qt::transparent);

    m_rightBorderCache = new QPixmap(length, length);
    m_rightBorderCache->fill(Qt::transparent);

    // Calculate the outline color. No alphablending is used for
    // performance reasons.
    const QColor c1 = m_styleOption.palette.text().color();
    const QColor c2 = m_styleOption.palette.background().color();
    const int p1 = 35;
    const int p2 = 100 - p1;
    m_outlineColor = QColor((c1.red()   * p1 + c2.red()   * p2) / 100,
                            (c1.green() * p1 + c2.green() * p2) / 100,
                            (c1.blue()  * p1 + c2.blue()  * p2) / 100);

    // The drawing code is based on the code of DolphinCategoryDrawer from Dolphin 1.7
    // Copyright (C) 2007 Rafael Fernández López <ereslibre@kde.org>
    {
        // Cache the left border as pixmap
        QPainter painter(m_leftBorderCache);
        painter.setPen(m_outlineColor);

        // 1. Draw top horizontal line
        painter.drawLine(3, 0, length, 0);

        // 2. Draw vertical line with gradient
        const QPoint start(0, 3);
        QLinearGradient gradient(start, QPoint(0, length));
        gradient.setColorAt(0, m_outlineColor);
        gradient.setColorAt(1, Qt::transparent);
        painter.fillRect(QRect(start, QSize(1, length - start.y())), gradient);

        // 3. Draw arc
        painter.setRenderHint(QPainter::Antialiasing);
        QRectF arc(QPointF(0, 0), QSizeF(4, 4));
        arc.translate(0.5, 0.5);
        painter.drawArc(arc, 1440, 1440);
    }

    {
        // Cache the right border as pixmap
        QPainter painter(m_rightBorderCache);
        painter.setPen(m_outlineColor);

        const int right = length - 1;
        if (m_scrollOrientation == Qt::Vertical) {
            // 1. Draw top horizontal line
            painter.drawLine(0, 0, length - 3, 0);

            // 2. Draw vertical line with gradient
            const QPoint start(right, 3);
            QLinearGradient gradient(start, QPoint(right, length));
            gradient.setColorAt(0, m_outlineColor);
            gradient.setColorAt(1, Qt::transparent);
            painter.fillRect(QRect(start, QSize(1, length - start.y())), gradient);

            // 3. Draw arc
            painter.setRenderHint(QPainter::Antialiasing);
            QRectF arc(QPointF(length - 5, 0), QSizeF(4, 4));
            arc.translate(0.5, 0.5);
            painter.drawArc(arc, 0, 1440);
        } else {
            // Draw a horizontal gradiented line
            QLinearGradient gradient(QPoint(0, 0), QPoint(length, 0));
            gradient.setColorAt(0, m_outlineColor);
            gradient.setColorAt(1, Qt::transparent);
            painter.fillRect(QRect(QPoint(0, 0), QSize(length, 1)), gradient);
        }
    }

    m_dirtyCache = false;
}

#include "kitemlistgroupheader.moc"
