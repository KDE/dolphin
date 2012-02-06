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
    m_roleColor(),
    m_roleBounds()
{
}

KItemListGroupHeader::~KItemListGroupHeader()
{
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
        if (orientation == Qt::Vertical) {
            m_dirtyCache = true;
        }
        scrollOrientationChanged(orientation, previous);
    }
}

Qt::Orientation KItemListGroupHeader::scrollOrientation() const
{
    return m_scrollOrientation;
}

void KItemListGroupHeader::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    if (m_dirtyCache) {
        updateCache();
    }

    if (m_scrollOrientation != Qt::Horizontal) {
        painter->setPen(m_roleColor);
        const qreal y = m_roleBounds.y() - m_styleOption.margin;
        painter->drawLine(0, y, size().width() - 1, y);
    }
}

QRectF KItemListGroupHeader::roleBounds() const
{
    return m_roleBounds;
}

QColor KItemListGroupHeader::roleColor() const
{
    return m_roleColor;
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

    // Calculate the outline color. No alphablending is used for
    // performance reasons.
    const QColor c1 = m_styleOption.palette.text().color();
    const QColor c2 = m_styleOption.palette.background().color();
    const int p1 = 35;
    const int p2 = 100 - p1;
    m_roleColor = QColor((c1.red()   * p1 + c2.red()   * p2) / 100,
                         (c1.green() * p1 + c2.green() * p2) / 100,
                         (c1.blue()  * p1 + c2.blue()  * p2) / 100);

    const int margin = m_styleOption.margin;
    const QFontMetrics fontMetrics(m_styleOption.font);
    const qreal roleHeight = fontMetrics.height();

    m_roleBounds = QRectF(margin,
                          size().height() - roleHeight - margin,
                          size().width() - 2 * margin,
                          roleHeight);

    m_dirtyCache = false;
}

#include "kitemlistgroupheader.moc"
