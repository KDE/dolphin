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

#include "kstandarditemlistgroupheader.h"

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
    m_itemIndex(-1),
    m_separatorColor(),
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

void KItemListGroupHeader::setItemIndex(int index)
{
    if (m_itemIndex != index) {
        const int previous = m_itemIndex;
        m_itemIndex = index;
        m_dirtyCache = true;
        itemIndexChanged(m_itemIndex, previous);
    }
}

int KItemListGroupHeader::itemIndex() const
{
    return m_itemIndex;
}

Qt::Orientation KItemListGroupHeader::scrollOrientation() const
{
    return m_scrollOrientation;
}

void KItemListGroupHeader::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(painter);
    Q_UNUSED(option);
    Q_UNUSED(widget);

    if (m_dirtyCache) {
        updateCache();
    }

    paintSeparator(painter, m_separatorColor);
    paintRole(painter, m_roleBounds, m_roleColor);
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

void KItemListGroupHeader::itemIndexChanged(int current, int previous)
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

    // Calculate the role- and line-color. No alphablending is used for
    // performance reasons.
    const QColor c1 = textColor();
    const QColor c2 = baseColor();
    m_separatorColor = mixedColor(c1, c2, 10);
    m_roleColor = mixedColor(c1, c2, 60);

    const int padding = qMax(1, m_styleOption.padding);
    const int horizontalMargin = qMax(2, m_styleOption.horizontalMargin);

    const QFontMetrics fontMetrics(m_styleOption.font);
    const qreal roleHeight = fontMetrics.height();

    const int y = (m_scrollOrientation == Qt::Vertical) ? padding : horizontalMargin;

    m_roleBounds = QRectF(horizontalMargin + padding,
                          y,
                          size().width() - 2 * padding - horizontalMargin,
                          roleHeight);

    m_dirtyCache = false;
}

QColor KItemListGroupHeader::mixedColor(const QColor& c1, const QColor& c2, int c1Percent)
{
    Q_ASSERT(c1Percent >= 0 && c1Percent <= 100);

    const int c2Percent = 100 - c1Percent;
    return QColor((c1.red()   * c1Percent + c2.red()   * c2Percent) / 100,
                  (c1.green() * c1Percent + c2.green() * c2Percent) / 100,
                  (c1.blue()  * c1Percent + c2.blue()  * c2Percent) / 100);
}

QPalette::ColorRole KItemListGroupHeader::normalTextColorRole() const
{
    return QPalette::Text;
}

QPalette::ColorRole KItemListGroupHeader::normalBaseColorRole() const
{
    return QPalette::Window;
}

QColor KItemListGroupHeader::textColor() const
{
    const QPalette::ColorGroup group = isActiveWindow() ? QPalette::Active : QPalette::Inactive;
    return styleOption().palette.color(group, normalTextColorRole());
}

QColor KItemListGroupHeader::baseColor() const
{
    const QPalette::ColorGroup group = isActiveWindow() ? QPalette::Active : QPalette::Inactive;
    return styleOption().palette.color(group, normalBaseColorRole());
}

#include "kitemlistgroupheader.moc"
