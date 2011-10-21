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

#include <QPainter>

#include <KDebug>

KItemListGroupHeader::KItemListGroupHeader(QGraphicsWidget* parent) :
    QGraphicsWidget(parent, 0),
    m_role(),
    m_data()
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

QSizeF KItemListGroupHeader::sizeHint(Qt::SizeHint which, const QSizeF& constraint) const
{
    Q_UNUSED(which);
    Q_UNUSED(constraint);
    return QSizeF();
}

void KItemListGroupHeader::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);
    painter->setPen(Qt::darkGreen);
    painter->setBrush(QColor(0, 255, 0, 50));
    painter->drawRect(rect());

    //painter->drawText(rect(), QString::number(m_index));
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

#include "kitemlistgroupheader.moc"
