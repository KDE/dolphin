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

#include "kitemlistheaderwidget.h"

#include <KAction>
#include <KMenu>
#include <kitemviews/kitemmodelbase.h>

#include <QApplication>
#include <QGraphicsSceneHoverEvent>
#include <QPainter>
#include <QStyleOptionHeader>

#include <KDebug>

KItemListHeaderWidget::KItemListHeaderWidget(QGraphicsWidget* parent) :
    QGraphicsWidget(parent),
    m_automaticColumnResizing(true),
    m_model(0),
    m_offset(0),
    m_columns(),
    m_columnWidths(),
    m_preferredColumnWidths(),
    m_hoveredRoleIndex(-1),
    m_pressedRoleIndex(-1),
    m_roleOperation(NoRoleOperation),
    m_pressedMousePos(),
    m_movingRole()
{
    m_movingRole.x = 0;
    m_movingRole.xDec = 0;
    m_movingRole.index = -1;

    setAcceptHoverEvents(true);
}

KItemListHeaderWidget::~KItemListHeaderWidget()
{
}

void KItemListHeaderWidget::setModel(KItemModelBase* model)
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

KItemModelBase* KItemListHeaderWidget::model() const
{
    return m_model;
}

void KItemListHeaderWidget::setAutomaticColumnResizing(bool automatic)
{
    m_automaticColumnResizing = automatic;
}

bool KItemListHeaderWidget::automaticColumnResizing() const
{
    return m_automaticColumnResizing;
}

void KItemListHeaderWidget::setColumns(const QList<QByteArray>& roles)
{
    foreach (const QByteArray& role, roles) {
        if (!m_columnWidths.contains(role)) {
            m_columnWidths.remove(role);
            m_preferredColumnWidths.remove(role);
        }
    }

    m_columns = roles;
    update();
}

QList<QByteArray> KItemListHeaderWidget::columns() const
{
    return m_columns;
}

void KItemListHeaderWidget::setColumnWidth(const QByteArray& role, qreal width)
{
    const qreal minWidth = minimumColumnWidth();
    if (width < minWidth) {
        width = minWidth;
    }

    if (m_columnWidths.value(role) != width) {
        m_columnWidths.insert(role, width);
        update();
    }
}

qreal KItemListHeaderWidget::columnWidth(const QByteArray& role) const
{
    return m_columnWidths.value(role);
}

void KItemListHeaderWidget::setPreferredColumnWidth(const QByteArray& role, qreal width)
{
    m_preferredColumnWidths.insert(role, width);
}

qreal KItemListHeaderWidget::preferredColumnWidth(const QByteArray& role) const
{
    return m_preferredColumnWidths.value(role);
}

void KItemListHeaderWidget::setOffset(qreal offset)
{
    if (m_offset != offset) {
        m_offset = offset;
        update();
    }
}

qreal KItemListHeaderWidget::offset() const
{
    return m_offset;
}

qreal KItemListHeaderWidget::minimumColumnWidth() const
{
    QFontMetricsF fontMetrics(font());
    return fontMetrics.height() * 4;
}

void KItemListHeaderWidget::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget)
{
    Q_UNUSED(option);
    Q_UNUSED(widget);

    if (!m_model) {
        return;
    }

    // Draw roles
    painter->setFont(font());
    painter->setPen(palette().text().color());

    qreal x = -m_offset;
    int orderIndex = 0;
    foreach (const QByteArray& role, m_columns) {
        const qreal roleWidth = m_columnWidths.value(role);
        const QRectF rect(x, 0, roleWidth, size().height());
        paintRole(painter, role, rect, orderIndex, widget);
        x += roleWidth;
        ++orderIndex;
    }

    // Draw background without roles
    QStyleOption opt;
    opt.init(widget);
    opt.rect = QRect(x, 0, size().width() - x, size().height());
    opt.state |= QStyle::State_Horizontal;
    style()->drawControl(QStyle::CE_HeaderEmptyArea, &opt, painter);

    if (!m_movingRole.pixmap.isNull()) {
        Q_ASSERT(m_roleOperation == MoveRoleOperation);
        painter->drawPixmap(m_movingRole.x, 0, m_movingRole.pixmap);
    }
}

void KItemListHeaderWidget::mousePressEvent(QGraphicsSceneMouseEvent* event)
{
    if (event->button() & Qt::LeftButton) {
        updatePressedRoleIndex(event->pos());
        m_pressedMousePos = event->pos();
        m_roleOperation = isAboveRoleGrip(m_pressedMousePos, m_pressedRoleIndex) ?
                          ResizeRoleOperation : NoRoleOperation;
        event->accept();
    } else {
        event->ignore();
    }
}

void KItemListHeaderWidget::mouseReleaseEvent(QGraphicsSceneMouseEvent* event)
{
    QGraphicsWidget::mouseReleaseEvent(event);

    if (m_pressedRoleIndex == -1) {
        return;
    }

    switch (m_roleOperation) {
    case NoRoleOperation: {
        // Only a click has been done and no moving or resizing has been started
        const QByteArray sortRole = m_model->sortRole();
        const int sortRoleIndex = m_columns.indexOf(sortRole);
        if (m_pressedRoleIndex == sortRoleIndex) {
            // Toggle the sort order
            const Qt::SortOrder previous = m_model->sortOrder();
            const Qt::SortOrder current = (m_model->sortOrder() == Qt::AscendingOrder) ?
                                          Qt::DescendingOrder : Qt::AscendingOrder;
            m_model->setSortOrder(current);
            emit sortOrderChanged(current, previous);
        } else {
            // Change the sort role and reset to the ascending order
            const QByteArray previous = m_model->sortRole();
            const QByteArray current = m_columns[m_pressedRoleIndex];
            m_model->setSortRole(current);
            emit sortRoleChanged(current, previous);

            if (m_model->sortOrder() == Qt::DescendingOrder) {
                m_model->setSortOrder(Qt::AscendingOrder);
                emit sortOrderChanged(Qt::AscendingOrder, Qt::DescendingOrder);
            }
        }
        break;
    }

    case MoveRoleOperation:
        m_movingRole.pixmap = QPixmap();
        m_movingRole.x = 0;
        m_movingRole.xDec = 0;
        m_movingRole.index = -1;
        break;

    default:
        break;
    }

    m_pressedRoleIndex = -1;
    m_roleOperation = NoRoleOperation;
    update();

    QApplication::restoreOverrideCursor();
}

void KItemListHeaderWidget::mouseMoveEvent(QGraphicsSceneMouseEvent* event)
{
    QGraphicsWidget::mouseMoveEvent(event);

    switch (m_roleOperation) {
    case NoRoleOperation:
        if ((event->pos() - m_pressedMousePos).manhattanLength() >= QApplication::startDragDistance()) {
            // A role gets dragged by the user. Create a pixmap of the role that will get
            // synchronized on each furter mouse-move-event with the mouse-position.
            m_roleOperation = MoveRoleOperation;
            const int roleIndex = roleIndexAt(m_pressedMousePos);
            m_movingRole.index = roleIndex;
            if (roleIndex == 0) {
                // TODO: It should be configurable whether moving the first role is allowed.
                // In the context of Dolphin this is not required, however this should be
                // changed if KItemViews are used in a more generic way.
                QApplication::setOverrideCursor(QCursor(Qt::ForbiddenCursor));
            } else {
                m_movingRole.pixmap = createRolePixmap(roleIndex);

                qreal roleX = -m_offset;
                for (int i = 0; i < roleIndex; ++i) {
                    const QByteArray role = m_columns[i];
                    roleX += m_columnWidths.value(role);
                }

                m_movingRole.xDec = event->pos().x() - roleX;
                m_movingRole.x = roleX;
                update();
            }
        }
        break;

    case ResizeRoleOperation: {
        const QByteArray pressedRole = m_columns[m_pressedRoleIndex];

        qreal previousWidth = m_columnWidths.value(pressedRole);
        qreal currentWidth = previousWidth;
        currentWidth += event->pos().x() - event->lastPos().x();
        currentWidth = qMax(minimumColumnWidth(), currentWidth);

        m_columnWidths.insert(pressedRole, currentWidth);
        update();

        emit columnWidthChanged(pressedRole, currentWidth, previousWidth);
        break;
    }

    case MoveRoleOperation: {
        // TODO: It should be configurable whether moving the first role is allowed.
        // In the context of Dolphin this is not required, however this should be
        // changed if KItemViews are used in a more generic way.
        if (m_movingRole.index > 0) {
            m_movingRole.x = event->pos().x() - m_movingRole.xDec;
            update();

            const int targetIndex = targetOfMovingRole();
            if (targetIndex > 0 && targetIndex != m_movingRole.index) {
                const QByteArray role = m_columns[m_movingRole.index];
                const int previousIndex = m_movingRole.index;
                m_movingRole.index = targetIndex;
                emit columnMoved(role, targetIndex, previousIndex);

                m_movingRole.xDec = event->pos().x() - roleXPosition(role);
            }
        }
        break;
    }

    default:
        break;
    }
}

void KItemListHeaderWidget::hoverEnterEvent(QGraphicsSceneHoverEvent* event)
{
    QGraphicsWidget::hoverEnterEvent(event);
    updateHoveredRoleIndex(event->pos());
}

void KItemListHeaderWidget::hoverLeaveEvent(QGraphicsSceneHoverEvent* event)
{
    QGraphicsWidget::hoverLeaveEvent(event);
    if (m_hoveredRoleIndex != -1) {
        m_hoveredRoleIndex = -1;
        update();
    }
}

void KItemListHeaderWidget::hoverMoveEvent(QGraphicsSceneHoverEvent* event)
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

void KItemListHeaderWidget::slotSortRoleChanged(const QByteArray& current, const QByteArray& previous)
{
    Q_UNUSED(current);
    Q_UNUSED(previous);
    update();
}

void KItemListHeaderWidget::slotSortOrderChanged(Qt::SortOrder current, Qt::SortOrder previous)
{
    Q_UNUSED(current);
    Q_UNUSED(previous);
    update();
}

void KItemListHeaderWidget::paintRole(QPainter* painter,
                                      const QByteArray& role,
                                      const QRectF& rect,
                                      int orderIndex,
                                      QWidget* widget) const
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

    if (m_columns.count() == 1) {
        option.position = QStyleOptionHeader::OnlyOneSection;
    } else if (orderIndex == 0) {
        option.position = QStyleOptionHeader::Beginning;
    } else if (orderIndex == m_columns.count() - 1) {
        option.position = QStyleOptionHeader::End;
    } else {
        option.position = QStyleOptionHeader::Middle;
    }

    option.orientation = Qt::Horizontal;
    option.selectedPosition = QStyleOptionHeader::NotAdjacent;
    option.text = m_model->roleDescription(role);

    style()->drawControl(QStyle::CE_Header, &option, painter, widget);
}

void KItemListHeaderWidget::updatePressedRoleIndex(const QPointF& pos)
{
    const int pressedIndex = roleIndexAt(pos);
    if (m_pressedRoleIndex != pressedIndex) {
        m_pressedRoleIndex = pressedIndex;
        update();
    }
}

void KItemListHeaderWidget::updateHoveredRoleIndex(const QPointF& pos)
{
    const int hoverIndex = roleIndexAt(pos);
    if (m_hoveredRoleIndex != hoverIndex) {
        m_hoveredRoleIndex = hoverIndex;
        update();
    }
}

int KItemListHeaderWidget::roleIndexAt(const QPointF& pos) const
{
    int index = -1;

    qreal x = -m_offset;
    foreach (const QByteArray& role, m_columns) {
        ++index;
        x += m_columnWidths.value(role);
        if (pos.x() <= x) {
            break;
        }
    }

    return index;
}

bool KItemListHeaderWidget::isAboveRoleGrip(const QPointF& pos, int roleIndex) const
{
    qreal x = -m_offset;
    for (int i = 0; i <= roleIndex; ++i) {
        const QByteArray role = m_columns[i];
        x += m_columnWidths.value(role);
    }

    const int grip = style()->pixelMetric(QStyle::PM_HeaderGripMargin);
    return pos.x() >= (x - grip) && pos.x() <= x;
}

QPixmap KItemListHeaderWidget::createRolePixmap(int roleIndex) const
{
    const QByteArray role = m_columns[roleIndex];
    const qreal roleWidth = m_columnWidths.value(role);
    const QRect rect(0, 0, roleWidth, size().height());

    QImage image(rect.size(), QImage::Format_ARGB32_Premultiplied);

    QPainter painter(&image);
    paintRole(&painter, role, rect, roleIndex);

    // Apply a highlighting-color
    const QPalette::ColorGroup group = isActiveWindow() ? QPalette::Active : QPalette::Inactive;
    QColor highlightColor = palette().color(group, QPalette::Highlight);
    highlightColor.setAlpha(64);
    painter.fillRect(rect, highlightColor);

    // Make the image transparent
    painter.setCompositionMode(QPainter::CompositionMode_DestinationIn);
    painter.fillRect(0, 0, image.width(), image.height(), QColor(0, 0, 0, 192));

    return QPixmap::fromImage(image);
}

int KItemListHeaderWidget::targetOfMovingRole() const
{
    const int movingWidth = m_movingRole.pixmap.width();
    const int movingLeft = m_movingRole.x;
    const int movingRight = movingLeft + movingWidth - 1;

    int targetIndex = 0;
    qreal targetLeft = -m_offset;
    while (targetIndex < m_columns.count()) {
        const QByteArray role = m_columns[targetIndex];
        const qreal targetWidth = m_columnWidths.value(role);
        const qreal targetRight = targetLeft + targetWidth - 1;

        const bool isInTarget = (targetWidth >= movingWidth &&
                                 movingLeft  >= targetLeft  &&
                                 movingRight <= targetRight) ||
                                (targetWidth <  movingWidth &&
                                 movingLeft  <= targetLeft  &&
                                 movingRight >= targetRight);

        if (isInTarget) {
            return targetIndex;
        }

        targetLeft += targetWidth;
        ++targetIndex;
    }

    return m_movingRole.index;
}

qreal KItemListHeaderWidget::roleXPosition(const QByteArray& role) const
{
    qreal x = -m_offset;
    foreach (const QByteArray& visibleRole, m_columns) {
        if (visibleRole == role) {
            return x;
        }

        x += m_columnWidths.value(visibleRole);
    }

    return -1;
}

#include "kitemlistheaderwidget.moc"
