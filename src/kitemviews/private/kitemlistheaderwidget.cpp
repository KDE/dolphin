/*
 * SPDX-FileCopyrightText: 2011 Peter Penz <peter.penz19@gmail.com>
 * SPDX-FileCopyrightText: 2022, 2024 Felix Ernst <felixernst@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "kitemlistheaderwidget.h"
#include "kitemviews/kitemmodelbase.h"

#include <QApplication>
#include <QGraphicsSceneHoverEvent>
#include <QPainter>
#include <QStyleOptionHeader>

namespace
{
/**
 * @returns a list which has a reversed order of elements compared to @a list.
 */
QList<QByteArray> reversed(const QList<QByteArray> list)
{
    QList<QByteArray> reversedList;
    for (auto i = list.rbegin(); i != list.rend(); i++) {
        reversedList.emplaceBack(*i);
    }
    return reversedList;
};

/**
 * @returns the index of the column for the name/text of items. This depends on the layoutDirection() and column count of @a itemListHeaderWidget.
 */
int nameColumnIndex(const KItemListHeaderWidget *itemListHeaderWidget)
{
    if (itemListHeaderWidget->layoutDirection() == Qt::LeftToRight) {
        return 0;
    }
    return itemListHeaderWidget->columns().count() - 1;
};
}

KItemListHeaderWidget::KItemListHeaderWidget(QGraphicsWidget *parent)
    : QGraphicsWidget(parent)
    , m_automaticColumnResizing(true)
    , m_model(nullptr)
    , m_offset(0)
    , m_leftPadding(0)
    , m_rightPadding(0)
    , m_columns()
    , m_columnWidths()
    , m_preferredColumnWidths()
    , m_hoveredIndex(-1)
    , m_pressedRoleIndex(-1)
    , m_pressedMousePos()
    , m_movingRole()
{
    m_movingRole.x = 0;
    m_movingRole.xDec = 0;
    m_movingRole.index = -1;

    setAcceptHoverEvents(true);
    // TODO update when font changes at runtime
    setFont(QApplication::font("QHeaderView"));
}

KItemListHeaderWidget::~KItemListHeaderWidget()
{
}

void KItemListHeaderWidget::setModel(KItemModelBase *model)
{
    if (m_model == model) {
        return;
    }

    if (m_model) {
        disconnect(m_model, &KItemModelBase::sortRoleChanged, this, &KItemListHeaderWidget::slotSortRoleChanged);
        disconnect(m_model, &KItemModelBase::sortOrderChanged, this, &KItemListHeaderWidget::slotSortOrderChanged);
    }

    m_model = model;

    if (m_model) {
        connect(m_model, &KItemModelBase::sortRoleChanged, this, &KItemListHeaderWidget::slotSortRoleChanged);
        connect(m_model, &KItemModelBase::sortOrderChanged, this, &KItemListHeaderWidget::slotSortOrderChanged);
    }
}

KItemModelBase *KItemListHeaderWidget::model() const
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

void KItemListHeaderWidget::setColumns(const QList<QByteArray> &roles)
{
    for (const QByteArray &role : roles) {
        if (!m_columnWidths.contains(role)) {
            m_preferredColumnWidths.remove(role);
        }
    }

    m_columns = layoutDirection() == Qt::LeftToRight ? roles : reversed(roles);
    update();
}

QList<QByteArray> KItemListHeaderWidget::columns() const
{
    return layoutDirection() == Qt::LeftToRight ? m_columns : reversed(m_columns);
}

void KItemListHeaderWidget::setColumnWidth(const QByteArray &role, qreal width)
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

qreal KItemListHeaderWidget::columnWidth(const QByteArray &role) const
{
    return m_columnWidths.value(role);
}

void KItemListHeaderWidget::setPreferredColumnWidth(const QByteArray &role, qreal width)
{
    m_preferredColumnWidths.insert(role, width);
}

qreal KItemListHeaderWidget::preferredColumnWidth(const QByteArray &role) const
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

void KItemListHeaderWidget::setSidePadding(qreal leftPaddingWidth, qreal rightPaddingWidth)
{
    bool changed = false;
    if (m_leftPadding != leftPaddingWidth) {
        m_leftPadding = leftPaddingWidth;
        changed = true;
    }

    if (m_rightPadding != rightPaddingWidth) {
        m_rightPadding = rightPaddingWidth;
        changed = true;
    }

    if (!changed) {
        return;
    }

    Q_EMIT sidePaddingChanged(leftPaddingWidth, rightPaddingWidth);
    update();
}

qreal KItemListHeaderWidget::leftPadding() const
{
    return m_leftPadding;
}

qreal KItemListHeaderWidget::rightPadding() const
{
    return m_rightPadding;
}

qreal KItemListHeaderWidget::minimumColumnWidth() const
{
    QFontMetricsF fontMetrics(font());
    return fontMetrics.height() * 4;
}

void KItemListHeaderWidget::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    Q_UNUSED(option)
    Q_UNUSED(widget)

    if (!m_model) {
        return;
    }

    // Draw roles
    painter->setFont(font());
    painter->setPen(palette().text().color());

    qreal x = -m_offset + m_leftPadding + unusedSpace();
    int orderIndex = 0;
    for (const QByteArray &role : std::as_const(m_columns)) {
        const qreal roleWidth = m_columnWidths.value(role);
        const QRectF rect(x, 0, roleWidth, size().height());
        paintRole(painter, role, rect, orderIndex, widget);
        x += roleWidth;
        ++orderIndex;
    }

    if (!m_movingRole.pixmap.isNull()) {
        painter->drawPixmap(m_movingRole.x, 0, m_movingRole.pixmap);
    }
}

void KItemListHeaderWidget::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (event->button() & Qt::LeftButton) {
        m_pressedMousePos = event->pos();
        m_pressedGrip = isAboveResizeGrip(m_pressedMousePos);
        if (!m_pressedGrip) {
            updatePressedRoleIndex(event->pos());
        }
        event->accept();
    } else {
        event->ignore();
    }
}

void KItemListHeaderWidget::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsWidget::mouseReleaseEvent(event);

    if (m_pressedGrip) {
        // Emitting a column width change removes automatic column resizing, so we do not emit if only the padding is being changed.
        // Eception: In mouseMoveEvent() we also resize the last column if the right padding is at zero but the user still quickly resizes beyond the screen
        // boarder. Such a resize "of the right padding" is let through when automatic column resizing was disabled by that resize.
        if (m_pressedGrip->roleToTheLeft != "leftPadding" && (m_pressedGrip->roleToTheRight != "rightPadding" || !m_automaticColumnResizing)) {
            const qreal currentWidth = m_columnWidths.value(m_pressedGrip->roleToTheLeft);
            Q_EMIT columnWidthChangeFinished(m_pressedGrip->roleToTheLeft, currentWidth);
        }
    } else if (m_pressedRoleIndex != -1 && m_movingRole.index == -1) {
        // Only a click has been done and no moving or resizing has been started
        const QByteArray sortRole = m_model->sortRole();
        const int sortRoleIndex = m_columns.indexOf(sortRole);
        if (m_pressedRoleIndex == sortRoleIndex) {
            // Toggle the sort order
            const Qt::SortOrder previous = m_model->sortOrder();
            const Qt::SortOrder current = (m_model->sortOrder() == Qt::AscendingOrder) ? Qt::DescendingOrder : Qt::AscendingOrder;
            m_model->setSortOrder(current);
            Q_EMIT sortOrderChanged(current, previous);
        } else {
            // Change the sort role and reset to the ascending order
            const QByteArray previous = m_model->sortRole();
            const QByteArray current = m_columns[m_pressedRoleIndex];
            const bool resetSortOrder = m_model->sortOrder() == Qt::DescendingOrder;
            m_model->setSortRole(current, !resetSortOrder);
            Q_EMIT sortRoleChanged(current, previous);

            if (resetSortOrder) {
                m_model->setSortOrder(Qt::AscendingOrder);
                Q_EMIT sortOrderChanged(Qt::AscendingOrder, Qt::DescendingOrder);
            }
        }
    }

    m_movingRole.pixmap = QPixmap();
    m_movingRole.x = 0;
    m_movingRole.xDec = 0;
    m_movingRole.index = -1;

    m_pressedGrip = std::nullopt;
    m_pressedRoleIndex = -1;
    update();

    QApplication::restoreOverrideCursor();
}

void KItemListHeaderWidget::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsWidget::mouseMoveEvent(event);

    if (m_pressedGrip) {
        if (m_pressedGrip->roleToTheLeft == "leftPadding") {
            qreal currentWidth = m_leftPadding;
            currentWidth += event->pos().x() - event->lastPos().x();
            m_leftPadding = qMax(0.0, currentWidth);

            update();
            Q_EMIT sidePaddingChanged(m_leftPadding, m_rightPadding);
            return;
        }

        if (m_pressedGrip->roleToTheRight == "rightPadding") {
            qreal currentWidth = m_rightPadding;
            currentWidth -= event->pos().x() - event->lastPos().x();
            m_rightPadding = qMax(0.0, currentWidth);

            update();
            Q_EMIT sidePaddingChanged(m_leftPadding, m_rightPadding);
            if (m_rightPadding > 0.0) {
                return;
            }
            // Continue so resizing of the last column beyond the view width is possible.
            if (currentWidth > -10) {
                return; // Automatic column resizing is valuable, so we don't want to give it up just for a few pixels of extra width for the rightmost column.
            }
            m_automaticColumnResizing = false;
        }

        qreal previousWidth = m_columnWidths.value(m_pressedGrip->roleToTheLeft);
        qreal currentWidth = previousWidth;
        currentWidth += event->pos().x() - event->lastPos().x();
        currentWidth = qMax(minimumColumnWidth(), currentWidth);

        m_columnWidths.insert(m_pressedGrip->roleToTheLeft, currentWidth);
        update();

        Q_EMIT columnWidthChanged(m_pressedGrip->roleToTheLeft, currentWidth, previousWidth);
        return;
    }

    if (m_movingRole.index != -1) {
        // TODO: It should be configurable whether moving the first role is allowed.
        // In the context of Dolphin this is not required, however this should be
        // changed if KItemViews are used in a more generic way.
        if (m_movingRole.index != nameColumnIndex(this)) {
            m_movingRole.x = event->pos().x() - m_movingRole.xDec;
            update();

            const int targetIndex = targetOfMovingRole();
            if (targetIndex > 0 && targetIndex != m_movingRole.index) {
                const QByteArray role = m_columns[m_movingRole.index];
                const int previousIndex = m_movingRole.index;
                m_movingRole.index = targetIndex;
                if (layoutDirection() == Qt::LeftToRight) {
                    Q_EMIT columnMoved(role, targetIndex, previousIndex);
                } else {
                    Q_EMIT columnMoved(role, m_columns.count() - 1 - targetIndex, m_columns.count() - 1 - previousIndex);
                }

                m_movingRole.xDec = event->pos().x() - roleXPosition(role);
            }
        }
        return;
    }

    if ((event->pos() - m_pressedMousePos).manhattanLength() >= QApplication::startDragDistance()) {
        // A role gets dragged by the user. Create a pixmap of the role that will get
        // synchronized on each further mouse-move-event with the mouse-position.
        const int roleIndex = roleIndexAt(m_pressedMousePos);
        m_movingRole.index = roleIndex;
        if (roleIndex == nameColumnIndex(this)) {
            // TODO: It should be configurable whether moving the first role is allowed.
            // In the context of Dolphin this is not required, however this should be
            // changed if KItemViews are used in a more generic way.
            QApplication::setOverrideCursor(QCursor(Qt::ForbiddenCursor));
            return;
        }

        m_movingRole.pixmap = createRolePixmap(roleIndex);

        qreal roleX = -m_offset + m_leftPadding + unusedSpace();
        for (int i = 0; i < roleIndex; ++i) {
            const QByteArray role = m_columns[i];
            roleX += m_columnWidths.value(role);
        }

        m_movingRole.xDec = event->pos().x() - roleX;
        m_movingRole.x = roleX;
        update();
    }
}

void KItemListHeaderWidget::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    QGraphicsItem::mouseDoubleClickEvent(event);

    const std::optional<Grip> doubleClickedGrip = isAboveResizeGrip(event->pos());
    if (!doubleClickedGrip || doubleClickedGrip->roleToTheLeft.isEmpty()) {
        return;
    }

    qreal previousWidth = columnWidth(doubleClickedGrip->roleToTheLeft);
    setColumnWidth(doubleClickedGrip->roleToTheLeft, preferredColumnWidth(doubleClickedGrip->roleToTheLeft));
    qreal currentWidth = columnWidth(doubleClickedGrip->roleToTheLeft);

    Q_EMIT columnWidthChanged(doubleClickedGrip->roleToTheLeft, currentWidth, previousWidth);
    Q_EMIT columnWidthChangeFinished(doubleClickedGrip->roleToTheLeft, currentWidth);
}

void KItemListHeaderWidget::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
    QGraphicsWidget::hoverEnterEvent(event);
    updateHoveredIndex(event->pos());
}

void KItemListHeaderWidget::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
    QGraphicsWidget::hoverLeaveEvent(event);
    if (m_hoveredIndex != -1) {
        Q_EMIT columnUnHovered(m_hoveredIndex);
        m_hoveredIndex = -1;
        update();
    }
}

void KItemListHeaderWidget::hoverMoveEvent(QGraphicsSceneHoverEvent *event)
{
    QGraphicsWidget::hoverMoveEvent(event);

    const QPointF &pos = event->pos();
    updateHoveredIndex(pos);
    if (isAboveResizeGrip(pos)) {
        setCursor(Qt::SplitHCursor);
    } else {
        unsetCursor();
    }
}

void KItemListHeaderWidget::slotSortRoleChanged(const QByteArray &current, const QByteArray &previous)
{
    Q_UNUSED(current)
    Q_UNUSED(previous)
    update();
}

void KItemListHeaderWidget::slotSortOrderChanged(Qt::SortOrder current, Qt::SortOrder previous)
{
    Q_UNUSED(current)
    Q_UNUSED(previous)
    update();
}

void KItemListHeaderWidget::paintRole(QPainter *painter, const QByteArray &role, const QRectF &rect, int orderIndex, QWidget *widget) const
{
    // The following code is based on the code from QHeaderView::paintSection().
    // SPDX-FileCopyrightText: 2011 Nokia Corporation and/or its subsidiary(-ies).
    QStyleOptionHeader option;
    option.section = orderIndex;
    option.state = QStyle::State_None | QStyle::State_Raised | QStyle::State_Horizontal;
    if (isEnabled()) {
        option.state |= QStyle::State_Enabled;
    }
    if (window() && window()->isActiveWindow()) {
        option.state |= QStyle::State_Active;
    }
    if (m_hoveredIndex == orderIndex) {
        option.state |= QStyle::State_MouseOver;
    }
    if (m_pressedRoleIndex == orderIndex) {
        option.state |= QStyle::State_Sunken;
    }
    if (m_model->sortRole() == role) {
        option.sortIndicator = (m_model->sortOrder() == Qt::AscendingOrder) ? QStyleOptionHeader::SortDown : QStyleOptionHeader::SortUp;
    }
    option.rect = rect.toRect();
    option.orientation = Qt::Horizontal;
    option.selectedPosition = QStyleOptionHeader::NotAdjacent;
    option.text = m_model->roleDescription(role);

    // First we paint any potential empty (padding) space on left and/or right of this role's column.
    const auto paintPadding = [&](int section, const QRectF &rect, const QStyleOptionHeader::SectionPosition &pos) {
        QStyleOptionHeader padding;
        padding.state = QStyle::State_None | QStyle::State_Raised | QStyle::State_Horizontal;
        padding.section = section;
        padding.sortIndicator = QStyleOptionHeader::None;
        padding.rect = rect.toRect();
        padding.position = pos;
        padding.text = QString();
        style()->drawControl(QStyle::CE_Header, &padding, painter, widget);
    };

    if (m_columns.count() == 1) {
        option.position = QStyleOptionHeader::Middle;
        paintPadding(0, QRectF(0.0, 0.0, rect.left(), rect.height()), QStyleOptionHeader::Beginning);
        paintPadding(1, QRectF(rect.right(), 0.0, size().width() - rect.right(), rect.height()), QStyleOptionHeader::End);
    } else if (orderIndex == 0) {
        // Paint the header for the first column; check if there is some empty space to the left which needs to be filled.
        if (rect.left() > 0) {
            option.position = QStyleOptionHeader::Middle;
            paintPadding(0, QRectF(0.0, 0.0, rect.left(), rect.height()), QStyleOptionHeader::Beginning);
        } else {
            option.position = QStyleOptionHeader::Beginning;
        }
    } else if (orderIndex == m_columns.count() - 1) {
        // Paint the header for the last column; check if there is some empty space to the right which needs to be filled.
        if (rect.right() < size().width()) {
            option.position = QStyleOptionHeader::Middle;
            paintPadding(m_columns.count(), QRectF(rect.right(), 0.0, size().width() - rect.right(), rect.height()), QStyleOptionHeader::End);
        } else {
            option.position = QStyleOptionHeader::End;
        }
    } else {
        option.position = QStyleOptionHeader::Middle;
    }

    style()->drawControl(QStyle::CE_Header, &option, painter, widget);
}

void KItemListHeaderWidget::updatePressedRoleIndex(const QPointF &pos)
{
    const int pressedIndex = roleIndexAt(pos);
    if (m_pressedRoleIndex != pressedIndex) {
        m_pressedRoleIndex = pressedIndex;
        update();
    }
}

void KItemListHeaderWidget::updateHoveredIndex(const QPointF &pos)
{
    const int hoverIndex = isAboveResizeGrip(pos) ? -1 : roleIndexAt(pos);

    if (m_hoveredIndex != hoverIndex) {
        if (m_hoveredIndex != -1) {
            Q_EMIT columnUnHovered(m_hoveredIndex);
        }
        m_hoveredIndex = hoverIndex;
        if (m_hoveredIndex != -1) {
            Q_EMIT columnHovered(m_hoveredIndex);
        }
        update();
    }
}

int KItemListHeaderWidget::roleIndexAt(const QPointF &pos) const
{
    qreal x = -m_offset + m_leftPadding + unusedSpace();
    if (pos.x() < x) {
        return -1;
    }

    int index = -1;
    for (const QByteArray &role : std::as_const(m_columns)) {
        ++index;
        x += m_columnWidths.value(role);
        if (pos.x() <= x) {
            return index;
        }
    }

    return -1;
}

std::optional<const KItemListHeaderWidget::Grip> KItemListHeaderWidget::isAboveResizeGrip(const QPointF &position) const
{
    qreal x = -m_offset + m_leftPadding + unusedSpace();
    const int gripWidthTolerance = style()->pixelMetric(QStyle::PM_HeaderGripMargin);

    if (x - gripWidthTolerance < position.x() && position.x() < x + gripWidthTolerance) {
        return std::optional{Grip{"leftPadding", m_columns[0]}};
    }

    for (int i = 0; i < m_columns.count(); ++i) {
        const QByteArray role = m_columns[i];
        x += m_columnWidths.value(role);
        if (x - gripWidthTolerance < position.x() && position.x() < x + gripWidthTolerance) {
            if (i + 1 < m_columns.count()) {
                return std::optional{Grip{m_columns[i], m_columns[i + 1]}};
            }
            return std::optional{Grip{m_columns[i], "rightPadding"}};
        }
    }
    return std::nullopt;
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
    qreal targetLeft = -m_offset + m_leftPadding + unusedSpace();
    while (targetIndex < m_columns.count()) {
        const QByteArray role = m_columns[targetIndex];
        const qreal targetWidth = m_columnWidths.value(role);
        const qreal targetRight = targetLeft + targetWidth - 1;

        const bool isInTarget = (targetWidth >= movingWidth && movingLeft >= targetLeft && movingRight <= targetRight)
            || (targetWidth < movingWidth && movingLeft <= targetLeft && movingRight >= targetRight);

        if (isInTarget) {
            return targetIndex;
        }

        targetLeft += targetWidth;
        ++targetIndex;
    }

    return m_movingRole.index;
}

qreal KItemListHeaderWidget::roleXPosition(const QByteArray &role) const
{
    qreal x = -m_offset + m_leftPadding + unusedSpace();
    for (const QByteArray &visibleRole : std::as_const(m_columns)) {
        if (visibleRole == role) {
            return x;
        }

        x += m_columnWidths.value(visibleRole);
    }

    return -1;
}

qreal KItemListHeaderWidget::unusedSpace() const
{
    if (layoutDirection() == Qt::LeftToRight) {
        return 0;
    }
    int unusedSpace = size().width() - m_leftPadding - m_rightPadding;
    for (int i = 0; i < m_columns.count(); ++i) {
        const QByteArray role = m_columns[i];
        unusedSpace -= m_columnWidths.value(role);
    }
    return qMax(unusedSpace, 0);
}

#include "moc_kitemlistheaderwidget.cpp"
