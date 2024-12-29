/*
 * SPDX-FileCopyrightText: 2011 Peter Penz <peter.penz19@gmail.com>
 * SPDX-FileCopyrightText: 2022, 2024 Felix Ernst <felixernst@kde.org>
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef KITEMLISTHEADERWIDGET_H
#define KITEMLISTHEADERWIDGET_H

#include "dolphin_export.h"

#include <QGraphicsWidget>
#include <QHash>
#include <QList>

class KItemModelBase;

/**
 * @brief Widget the implements the header for KItemListView showing the currently used roles.
 *
 * The widget is an internal API, the user of KItemListView may only access the
 * class KItemListHeader.
 */
class DOLPHIN_EXPORT KItemListHeaderWidget : public QGraphicsWidget
{
    Q_OBJECT

public:
    explicit KItemListHeaderWidget(QGraphicsWidget *parent = nullptr);
    ~KItemListHeaderWidget() override;

    void setModel(KItemModelBase *model);
    KItemModelBase *model() const;

    void setAutomaticColumnResizing(bool automatic);
    bool automaticColumnResizing() const;

    void setColumns(const QList<QByteArray> &roles);
    QList<QByteArray> columns() const;

    void setColumnWidth(const QByteArray &role, qreal width);
    qreal columnWidth(const QByteArray &role) const;

    /**
     * Sets the column-width that is required to show the role unclipped.
     */
    void setPreferredColumnWidth(const QByteArray &role, qreal width);
    qreal preferredColumnWidth(const QByteArray &role) const;

    void setOffset(qreal offset);
    qreal offset() const;

    void setSidePadding(qreal leftPaddingWidth, qreal rightPaddingWidth);
    qreal leftPadding() const;
    qreal rightPadding() const;

    qreal minimumColumnWidth() const;

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

Q_SIGNALS:
    /**
     * Is emitted if the width of a visible role has been adjusted by the user with the mouse
     * (no signal is emitted if KItemListHeader::setVisibleRoleWidth() is invoked).
     */
    void columnWidthChanged(const QByteArray &role, qreal currentWidth, qreal previousWidth);

    void sidePaddingChanged(qreal leftPaddingWidth, qreal rightPaddingWidth);

    /**
     * Is emitted if the user has released the mouse button after adjusting the
     * width of a visible role.
     */
    void columnWidthChangeFinished(const QByteArray &role, qreal currentWidth);

    /**
     * Is emitted if the position of the column has been changed.
     */
    void columnMoved(const QByteArray &role, int currentIndex, int previousIndex);

    /**
     * Is emitted if the user has changed the sort order by clicking on a
     * header item. The sort order of the model has already been adjusted to
     * the current sort order. Note that no signal will be emitted if the
     * sort order of the model has been changed without user interaction.
     */
    void sortOrderChanged(Qt::SortOrder current, Qt::SortOrder previous);

    /**
     * Is emitted if the user has changed the sort role by clicking on a
     * header item. The sort role of the model has already been adjusted to
     * the current sort role. Note that no signal will be emitted if the
     * sort role of the model has been changed without user interaction.
     */
    void sortRoleChanged(const QByteArray &current, const QByteArray &previous);

    void columnUnHovered(int columnIndex);
    void columnHovered(int columnIndex);

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *event) override;
    void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override;

private Q_SLOTS:
    void slotSortRoleChanged(const QByteArray &current, const QByteArray &previous);
    void slotSortOrderChanged(Qt::SortOrder current, Qt::SortOrder previous);

private:
    struct Grip {
        QByteArray roleToTheLeft;
        QByteArray roleToTheRight;
    };

    void paintRole(QPainter *painter, const QByteArray &role, const QRectF &rect, int orderIndex, QWidget *widget = nullptr) const;

    void updatePressedRoleIndex(const QPointF &pos);
    void updateHoveredIndex(const QPointF &pos);
    int roleIndexAt(const QPointF &pos) const;

    /**
     * @returns std::nullopt if none of the resize grips is below @p position.
     *          Otherwise returns a Grip defined by the two roles on each side of @p position.
     * @note If the Grip is between a padding and a role, a class-specific "leftPadding" or "rightPadding" role is used.
     */
    std::optional<const Grip> isAboveResizeGrip(const QPointF &position) const;

    /**
     * Creates a pixmap of the role with the index \a roleIndex that is shown
     * during moving a role.
     */
    QPixmap createRolePixmap(int roleIndex) const;

    /**
     * @return Target index of the currently moving visible role based on the current
     *         state of m_movingRole.
     */
    int targetOfMovingRole() const;

    /**
     * @return x-position of the left border of the role \a role.
     */
    qreal roleXPosition(const QByteArray &role) const;

    /**
     * @returns 0 for left-to-right layoutDirection().
     *          Otherwise returns the space that is left over when space is distributed between padding and role columns.
     * Used to make the column headers stay above their information columns for right-to-left layout directions.
     */
    qreal unusedSpace() const;

private:
    bool m_automaticColumnResizing;
    KItemModelBase *m_model;
    qreal m_offset;
    qreal m_leftPadding;
    qreal m_rightPadding;
    QList<QByteArray> m_columns;
    QHash<QByteArray, qreal> m_columnWidths;
    QHash<QByteArray, qreal> m_preferredColumnWidths;

    int m_hoveredIndex;
    std::optional<Grip> m_pressedGrip;
    int m_pressedRoleIndex;
    QPointF m_pressedMousePos;

    struct MovingRole {
        QPixmap pixmap;
        int x;
        int xDec;
        int index;
    };
    MovingRole m_movingRole;
};

#endif
