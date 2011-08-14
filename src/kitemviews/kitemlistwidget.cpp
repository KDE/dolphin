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

#include "kitemlistwidget.h"

#include "kitemlistview.h"
#include "kitemmodelbase.h"

#include <KDebug>

#include <QPainter>
#include <QPropertyAnimation>
#include <QStyle>
#include <QStyleOption>

KItemListWidget::KItemListWidget(QGraphicsItem* parent) :
    QGraphicsWidget(parent, 0),
    m_index(-1),
    m_selected(false),
    m_current(false),
    m_hovered(false),
    m_data(),
    m_visibleRoles(),
    m_visibleRolesSizes(),
    m_styleOption(),
    m_hoverOpacity(0),
    m_hoverCache(0),
    m_hoverAnimation(0)
{
}

KItemListWidget::~KItemListWidget()
{
    clearCache();
}

void KItemListWidget::setIndex(int index)
{
    if (m_index != index) {
        if (m_hoverAnimation) {
            m_hoverAnimation->stop();
            m_hoverOpacity = 0;
        }
        clearCache();

        m_index = index;
    }
}

int KItemListWidget::index() const
{
    return m_index;
}

void KItemListWidget::setData(const QHash<QByteArray, QVariant>& data,
                              const QSet<QByteArray>& roles)
{
    clearCache();
    if (roles.isEmpty()) {
        m_data = data;
        dataChanged(m_data);
    } else {
        foreach (const QByteArray& role, roles) {
            m_data[role] = data[role];
        }
        dataChanged(m_data, roles);
    }
}

QHash<QByteArray, QVariant> KItemListWidget::data() const
{
    return m_data;
}

void KItemListWidget::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);

    painter->setRenderHint(QPainter::Antialiasing);

    const QRect iconBounds = iconBoundingRect().toRect();
    if (m_selected) {
        QStyleOptionViewItemV4 viewItemOption;
        viewItemOption.initFrom(widget);
        viewItemOption.rect = iconBounds;
        viewItemOption.state = QStyle::State_Enabled | QStyle::State_Selected | QStyle::State_Item;
        viewItemOption.viewItemPosition = QStyleOptionViewItemV4::OnlyOne;
        widget->style()->drawPrimitive(QStyle::PE_PanelItemViewItem, &viewItemOption, painter, widget);

        drawTextBackground(painter);
    }

    if (isCurrent()) {
        drawFocusIndicator(painter);
    }

    if (m_hoverOpacity <= 0.0) {
        return;
    }

    if (!m_hoverCache) {
        // Initialize the m_hoverCache pixmap to improve the drawing performance
        // when fading the hover background
        m_hoverCache = new QPixmap(iconBounds.size());
        m_hoverCache->fill(Qt::transparent);

        QPainter pixmapPainter(m_hoverCache);

        QStyleOptionViewItemV4 viewItemOption;
        viewItemOption.initFrom(widget);
        viewItemOption.rect = QRect(0, 0, iconBounds.width(), iconBounds.height());
        viewItemOption.state = QStyle::State_Enabled | QStyle::State_MouseOver | QStyle::State_Item;
        viewItemOption.viewItemPosition = QStyleOptionViewItemV4::OnlyOne;

        widget->style()->drawPrimitive(QStyle::PE_PanelItemViewItem, &viewItemOption, &pixmapPainter, widget);
    }

    const qreal opacity = painter->opacity();
    painter->setOpacity(m_hoverOpacity * opacity);
    painter->drawPixmap(iconBounds.topLeft(), *m_hoverCache);
    drawTextBackground(painter);
    painter->setOpacity(opacity);
}

void KItemListWidget::setVisibleRoles(const QHash<QByteArray, int>& roles)
{
    const QHash<QByteArray, int> previousRoles = m_visibleRoles;
    m_visibleRoles = roles;
    visibleRolesChanged(roles, previousRoles);
}

QHash<QByteArray, int> KItemListWidget::visibleRoles() const
{
    return m_visibleRoles;
}

void KItemListWidget::setVisibleRolesSizes(const QHash<QByteArray, QSizeF> rolesSizes)
{
    const QHash<QByteArray, QSizeF> previousRolesSizes = m_visibleRolesSizes;
    m_visibleRolesSizes = rolesSizes;
    visibleRolesSizesChanged(rolesSizes, previousRolesSizes);
}

QHash<QByteArray, QSizeF> KItemListWidget::visibleRolesSizes() const
{
    return m_visibleRolesSizes;
}

void KItemListWidget::setStyleOption(const KItemListStyleOption& option)
{
    const KItemListStyleOption previous = m_styleOption;
    clearCache();
    m_styleOption = option;

    styleOptionChanged(option, previous);
}

const KItemListStyleOption& KItemListWidget::styleOption() const
{
    return m_styleOption;
}

void KItemListWidget::setSelected(bool selected)
{
    if (m_selected != selected) {
        m_selected = selected;
        selectedChanged(selected);
        update();
    }
}

bool KItemListWidget::isSelected() const
{
    return m_selected;
}

void KItemListWidget::setCurrent(bool current)
{
    if (m_current != current) {
        m_current = current;
        currentChanged(current);
        update();
    }
}

bool KItemListWidget::isCurrent() const
{
    return m_current;
}

void KItemListWidget::setHovered(bool hovered)
{
    if (hovered == m_hovered) {
        return;
    }

    m_hovered = hovered;

    if (!m_hoverAnimation) {
        m_hoverAnimation = new QPropertyAnimation(this, "hoverOpacity", this);
        m_hoverAnimation->setDuration(200);
    }
    m_hoverAnimation->stop();

    if (hovered) {
        m_hoverAnimation->setEndValue(1.0);
    } else {
        m_hoverAnimation->setEndValue(0.0);
    }

    m_hoverAnimation->start();

    hoveredChanged(hovered);

    update();
}

bool KItemListWidget::isHovered() const
{
    return m_hovered;
}

bool KItemListWidget::contains(const QPointF& point) const
{
    if (!QGraphicsWidget::contains(point)) {
        return false;
    }

    return iconBoundingRect().contains(point) ||
           textBoundingRect().contains(point) ||
           expansionToggleRect().contains(point) ||
           selectionToggleRect().contains(point);
}

QRectF KItemListWidget::selectionToggleRect() const
{
    return QRectF();
}

QRectF KItemListWidget::expansionToggleRect() const
{
    return QRectF();
}

void KItemListWidget::dataChanged(const QHash<QByteArray, QVariant>& current,
                                  const QSet<QByteArray>& roles)
{
    Q_UNUSED(current);
    Q_UNUSED(roles);
    update();
}

void KItemListWidget::visibleRolesChanged(const QHash<QByteArray, int>& current,
                                          const QHash<QByteArray, int>& previous)
{
    Q_UNUSED(current);
    Q_UNUSED(previous);
    update();
}

void KItemListWidget::visibleRolesSizesChanged(const QHash<QByteArray, QSizeF>& current,
                                               const QHash<QByteArray, QSizeF>& previous)
{
    Q_UNUSED(current);
    Q_UNUSED(previous);
    update();
}

void KItemListWidget::styleOptionChanged(const KItemListStyleOption& current,
                                         const KItemListStyleOption& previous)
{
    Q_UNUSED(current);
    Q_UNUSED(previous);
    update();
}

void KItemListWidget::currentChanged(bool current)
{
    Q_UNUSED(current);
}

void KItemListWidget::selectedChanged(bool selected)
{
    Q_UNUSED(selected);
}

void KItemListWidget::hoveredChanged(bool hovered)
{
    Q_UNUSED(hovered);
}

void KItemListWidget::resizeEvent(QGraphicsSceneResizeEvent* event)
{
    QGraphicsWidget::resizeEvent(event);
    clearCache();
}

qreal KItemListWidget::hoverOpacity() const
{
    return m_hoverOpacity;
}

void KItemListWidget::setHoverOpacity(qreal opacity)
{
    m_hoverOpacity = opacity;
    update();
}

void KItemListWidget::clearCache()
{
    delete m_hoverCache;
    m_hoverCache = 0;
}

void KItemListWidget::drawFocusIndicator(QPainter* painter)
{
    // Ideally style()->drawPrimitive(QStyle::PE_FrameFocusRect...)
    // should be used, but Oxygen only draws indicators within classes
    // derived from QAbstractItemView or Q3ListView. As a workaround
    // the indicator is drawn manually. Code copied from oxygenstyle.cpp
    // Copyright ( C ) 2009-2010 Hugo Pereira Da Costa <hugo@oxygen-icons.org>
    // TODO: Clarify with Oxygen maintainers how to proceed with this.

    const KItemListStyleOption& option = styleOption();
    const QPalette palette = option.palette;
    const QRect rect = textBoundingRect().toRect().adjusted(0, 0, 0, -1);

    QLinearGradient gradient(rect.bottomLeft(), rect.bottomRight());
    gradient.setColorAt(0.0, Qt::transparent);
    gradient.setColorAt(1.0, Qt::transparent);
    gradient.setColorAt(0.2, palette.color(QPalette::Text));
    gradient.setColorAt(0.8, palette.color(QPalette::Text));

    painter->setRenderHint(QPainter::Antialiasing, false);
    painter->setPen(QPen(gradient, 1));
    painter->drawLine(rect.bottomLeft(), rect.bottomRight());
    painter->setRenderHint(QPainter::Antialiasing, true);
}

void KItemListWidget::drawTextBackground(QPainter* painter)
{
    const qreal opacity = painter->opacity();

    QRectF textBounds = textBoundingRect();
    const qreal marginDiff = m_styleOption.margin / 2;
    textBounds.adjust(marginDiff, marginDiff, -marginDiff, -marginDiff);
    painter->setOpacity(opacity * 0.1);
    painter->setPen(Qt::NoPen);
    painter->setBrush(m_styleOption.palette.text());
    painter->drawRoundedRect(textBounds, 4, 4);

    painter->setOpacity(opacity);
}

#include "kitemlistwidget.moc"
