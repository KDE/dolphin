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

#include "kitemlistheader_p.h"

#include "kitemmodelbase.h"

#include <QApplication>
#include <QGraphicsSceneHoverEvent>
#include <QPainter>
#include <QStyleOptionHeader>

#include <KDebug>

KItemListHeader::KItemListHeader(QGraphicsWidget* parent) :
    QGraphicsWidget(parent),
    m_model(0),
    m_visibleRoles(),
    m_visibleRolesWidths(),
    m_hoveredRoleIndex(-1),
    m_pressedRoleIndex(-1),
    m_roleOperation(NoRoleOperation),
    m_pressedMousePos()
{
    setAcceptHoverEvents(true);

    QStyleOptionHeader option;
    const QSize headerSize = style()->sizeFromContents(QStyle::CT_HeaderSection, &option, QSize());
    resize(0, headerSize.height());
}

KItemListHeader::~KItemListHeader()
{
}

void KItemListHeader::setModel(KItemModelBase* model)
{
    if (m_model == model) {
        return;
    }

    if (m_model) {
        disconnect(m_model, SIGNAL(sortRoleChanged(QByteArray,QByteArray)),
                   this, SLOT(slotSortRoleChanged(QByteArray,QByteArray)));
        disconnect(m_model, SIGNAL(sortOrderChanged(Qt::SortOrder,Qt::SortOrder)),
                   this, SLOT(slotSortOrderChanged(Qt::SortOrder,Qt::SortOrder)));
    }

    m_model = model;

    if (m_model) {
        connect(m_model, SIGNAL(sortRoleChanged(QByteArray,QByteArray)),
                this, SLOT(slotSortRoleChanged(QByteArray,QByteArray)));
        connect(m_model, SIGNAL(sortOrderChanged(Qt::SortOrder,Qt::SortOrder)),
                this, SLOT(slotSortOrderChanged(Qt::SortOrder,Qt::SortOrder)));
    }
}

KItemModelBase* KItemListHeader::model() const
{
    return m_model;
}

void KItemListHeader::setVisibleRoles(const QList<QByteArray>& roles)
{
    m_visibleRoles = roles;
    update();
}

QList<QByteArray> KItemListHeader::visibleRoles() const
{
    return m_visibleRoles;
}

void KItemListHeader::setVisibleRolesWidths(const QHash<QByteArray, qreal> rolesWidths)
{
    m_visibleRolesWidths = rolesWidths;

    // Assure that no width is smaller than the minimum allowed width
    const qreal minWidth = minimumRoleWidth();
    QMutableHashIterator<QByteArray, qreal> it(m_visibleRolesWidths);
    while (it.hasNext()) {
        it.next();
        if (it.value() < minWidth) {
            m_visibleRolesWidths.insert(it.key(), minWidth);
        }
    }

    update();
}

QHash<QByteArray, qreal> KItemListHeader::visibleRolesWidths() const
{
    return m_visibleRolesWidths;
}

void KItemListHeader::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    if (!m_model) {
        return;
    }

    // Draw roles
    painter->setFont(font());
    painter->setPen(palette().text().color());

    qreal x = 0;
    int orderIndex = 0;
    foreach (const QByteArray& role, m_visibleRoles) {
        const qreal roleWidth = m_visibleRolesWidths.value(role);
        const QRectF rect(x, 0, roleWidth, size().height());
        paintRole(painter, role, rect, orderIndex);
        x += roleWidth;
        ++orderIndex;
    }

    // Draw background without roles
    QStyleOption opt;
    opt.init(widget);
    opt.rect = QRect(x, 0, size().width() - x, size().height());
    opt.state |= QStyle::State_Horizontal;
    style()->drawControl(QStyle::CE_HeaderEmptyArea, &opt, painter);
}

void KItemListHeader::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    event->accept();
    updatePressedRoleIndex(event->pos());
    m_pressedMousePos = event->pos();
    m_roleOperation = isAboveRoleGrip(m_pressedMousePos, m_pressedRoleIndex) ?
                      ResizeRoleOperation : NoRoleOperation;
}

void KItemListHeader::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    QGraphicsWidget::mouseReleaseEvent(event);

    if (m_pressedRoleIndex == -1) {
        return;
    }

    if (m_roleOperation == NoRoleOperation) {
        // Only a click has been done and no moving or resizing has been started
        const QByteArray sortRole = m_model->sortRole();
        const int sortRoleIndex = m_visibleRoles.indexOf(sortRole);
        if (m_pressedRoleIndex == sortRoleIndex) {
            // Toggle the sort order
            const Qt::SortOrder toggled = (m_model->sortOrder() == Qt::AscendingOrder) ?
                                          Qt::DescendingOrder : Qt::AscendingOrder;
            m_model->setSortOrder(toggled);
        } else {
            // Change the sort role
            const QByteArray sortRole = m_visibleRoles.at(m_pressedRoleIndex);
            m_model->setSortRole(sortRole);
        }
    }

    m_pressedRoleIndex = -1;
    m_roleOperation = NoRoleOperation;
    update();
}

void KItemListHeader::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    QGraphicsWidget::mouseMoveEvent(event);

    if (m_roleOperation == ResizeRoleOperation) {
        const QByteArray pressedRole = m_visibleRoles.at(m_pressedRoleIndex);

        qreal previousWidth = m_visibleRolesWidths.value(pressedRole);
        qreal currentWidth = previousWidth;
        currentWidth += event->pos().x() - event->lastPos().x();
        currentWidth = qMax(minimumRoleWidth(), currentWidth);

        m_visibleRolesWidths.insert(pressedRole, currentWidth);
        update();

        emit visibleRoleWidthChanged(pressedRole, currentWidth, previousWidth);
    } else if ((event->pos() - m_pressedMousePos).manhattanLength() >= QApplication::startDragDistance()) {
        kDebug() << "Moving of role not supported yet";
        m_roleOperation = MoveRoleOperation;
    }
}

void KItemListHeader::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
    QGraphicsWidget::hoverEnterEvent(event);
    updateHoveredRoleIndex(event->pos());
}

void KItemListHeader::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
    QGraphicsWidget::hoverLeaveEvent(event);
    if (m_hoveredRoleIndex != -1) {
        m_hoveredRoleIndex = -1;
        update();
    }
}

void KItemListHeader::hoverMoveEvent(QGraphicsSceneHoverEvent* event)
{
    QGraphicsWidget::hoverMoveEvent(event);

    const QPointF& pos = event->pos();
    updateHoveredRoleIndex(pos);
    if (m_hoveredRoleIndex >= 0 && isAboveRoleGrip(pos, m_hoveredRoleIndex)) {
        setCursor(Qt::SplitHCursor);
    } else {
        unsetCursor();
    }
}

void KItemListHeader::slotSortRoleChanged(const QByteArray& current, const QByteArray& previous)
{
    Q_UNUSED(current);
    Q_UNUSED(previous);
    update();
}

void KItemListHeader::slotSortOrderChanged(Qt::SortOrder current, Qt::SortOrder previous)
{
    Q_UNUSED(current);
    Q_UNUSED(previous);
    update();
}

void KItemListHeader::paintRole(QPainter* painter,
                                const QByteArray& role,
                                const QRectF& rect,
                                int orderIndex)
{
    // The following code is based on the code from QHeaderView::paintSection().
    // Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
    QStyleOptionHeader option;
    option.section = orderIndex;
    option.state = QStyle::State_None | QStyle::State_Raised | QStyle::State_Horizontal;
    if (isEnabled()) {
        option.state |= QStyle::State_Enabled;
    }
    if (window() && window()->isActiveWindow()) {
        option.state |= QStyle::State_Active;
    }
    if (m_hoveredRoleIndex == orderIndex) {
        option.state |= QStyle::State_MouseOver;
    }
    if (m_pressedRoleIndex == orderIndex) {
        option.state |= QStyle::State_Sunken;
    }
    if (m_model->sortRole() == role) {
        option.sortIndicator = (m_model->sortOrder() == Qt::AscendingOrder) ?
                                QStyleOptionHeader::SortDown : QStyleOptionHeader::SortUp;
    }
    option.rect = rect.toRect();

    if (m_visibleRoles.count() == 1) {
        option.position = QStyleOptionHeader::OnlyOneSection;
    } else if (orderIndex == 0) {
        option.position = QStyleOptionHeader::Beginning;
    } else if (orderIndex == m_visibleRoles.count() - 1) {
        option.position = QStyleOptionHeader::End;
    } else {
        option.position = QStyleOptionHeader::Middle;
    }

    option.orientation = Qt::Horizontal;
    option.selectedPosition = QStyleOptionHeader::NotAdjacent;
    option.text = m_model->roleDescription(role);

    style()->drawControl(QStyle::CE_Header, &option, painter);
}

void KItemListHeader::updatePressedRoleIndex(const QPointF& pos)
{
    const int pressedIndex = roleIndexAt(pos);
    if (m_pressedRoleIndex != pressedIndex) {
        m_pressedRoleIndex = pressedIndex;
        update();
    }
}

void KItemListHeader::updateHoveredRoleIndex(const QPointF& pos)
{
    const int hoverIndex = roleIndexAt(pos);
    if (m_hoveredRoleIndex != hoverIndex) {
        m_hoveredRoleIndex = hoverIndex;
        update();
    }
}

int KItemListHeader::roleIndexAt(const QPointF& pos) const
{
    int index = -1;

    qreal x = 0;
    foreach (const QByteArray& role, m_visibleRoles) {
        ++index;
        x += m_visibleRolesWidths.value(role);
        if (pos.x() <= x) {
            break;
        }
    }

    return index;
}

bool KItemListHeader::isAboveRoleGrip(const QPointF& pos, int roleIndex) const
{
    qreal x = 0;
    for (int i = 0; i <= roleIndex; ++i) {
        const QByteArray role = m_visibleRoles.at(i);
        x += m_visibleRolesWidths.value(role);
    }

    const int grip = style()->pixelMetric(QStyle::PM_HeaderGripMargin);
    return pos.x() >= (x - grip) && pos.x() <= x;
}

qreal KItemListHeader::minimumRoleWidth() const
{
    QFontMetricsF fontMetrics(font());
    return fontMetrics.averageCharWidth() * 8;
}

#include "kitemlistheader_p.moc"
