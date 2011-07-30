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

KItemListWidget::KItemListWidget(QGraphicsItem* parent) :
    QGraphicsWidget(parent, 0),
    m_index(-1),
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
    if (m_hoverOpacity <= 0.0) {
        return;
    }

    const QRect hoverBounds = hoverBoundingRect().toRect();
    if (!m_hoverCache) {
        m_hoverCache = new QPixmap(hoverBounds.size());
        m_hoverCache->fill(Qt::transparent);

        QPainter pixmapPainter(m_hoverCache);

        QStyleOptionViewItemV4 viewItemOption;
        viewItemOption.initFrom(widget);
        viewItemOption.rect = QRect(0, 0, hoverBounds.width(), hoverBounds.height());
        viewItemOption.state = QStyle::State_Enabled | QStyle::State_MouseOver;
        viewItemOption.viewItemPosition = QStyleOptionViewItemV4::OnlyOne;

        widget->style()->drawPrimitive(QStyle::PE_PanelItemViewItem, &viewItemOption, &pixmapPainter, widget);
    }

    const qreal opacity = painter->opacity();
    painter->setOpacity(m_hoverOpacity * opacity);
    painter->drawPixmap(hoverBounds.topLeft(), *m_hoverCache);
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
    if (m_index >= 0) {
        clearCache();

        const bool wasHovered = (previous.state & QStyle::State_MouseOver);
        m_styleOption = option;
        const bool isHovered = (m_styleOption.state & QStyle::State_MouseOver);

        if (wasHovered != isHovered) {
            // The hovering state has been changed. Assure that a fade-animation
            // is done to the new state.
            if (!m_hoverAnimation) {
                m_hoverAnimation = new QPropertyAnimation(this, "hoverOpacity", this);
                m_hoverAnimation->setDuration(200);
            }
            m_hoverAnimation->stop();

            if (!wasHovered && isHovered) {
                m_hoverAnimation->setEndValue(1.0);
            } else {
                Q_ASSERT(wasHovered && !isHovered);
                m_hoverAnimation->setEndValue(0.0);
            }

            m_hoverAnimation->start();
        }
    } else {
        m_styleOption = option;
    }

    styleOptionChanged(option, previous);
}

const KItemListStyleOption& KItemListWidget::styleOption() const
{
    return m_styleOption;
}

bool KItemListWidget::contains(const QPointF& point) const
{
    return hoverBoundingRect().contains(point) ||
           expansionToggleRect().contains(point) ||
           selectionToggleRect().contains(point);
}

QRectF KItemListWidget::hoverBoundingRect() const
{
    return QRectF(QPointF(0, 0), size());
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

#include "kitemlistwidget.moc"
