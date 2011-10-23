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

#ifndef KITEMLISTWIDGET_H
#define KITEMLISTWIDGET_H

#include <libdolphin_export.h>

#include <kitemviews/kitemliststyleoption.h>

#include <QGraphicsWidget>

class QPropertyAnimation;

/**
 * @brief Widget that shows a visible item from the model.
 *
 * For showing an item from a custom model it is required to at least overwrite KItemListWidget::paint().
 * All properties are set by KItemListView, for each property there is a corresponding
 * virtual protected method that allows to react on property changes.
 */
class LIBDOLPHINPRIVATE_EXPORT KItemListWidget : public QGraphicsWidget
{
    Q_OBJECT

public:
    KItemListWidget(QGraphicsItem* parent);
    virtual ~KItemListWidget();

    void setIndex(int index);
    int index() const;

    void setData(const QHash<QByteArray, QVariant>& data, const QSet<QByteArray>& roles = QSet<QByteArray>());
    QHash<QByteArray, QVariant> data() const;

    /**
     * Draws the hover-rectangle if the item is hovered. Overwrite this method
     * to show the data of the custom model provided by KItemListWidget::data().
     * @reimp
     */
    virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = 0);

    void setVisibleRoles(const QList<QByteArray>& roles);
    QList<QByteArray> visibleRoles() const;

    void setVisibleRolesSizes(const QHash<QByteArray, QSizeF> rolesSizes);
    QHash<QByteArray, QSizeF> visibleRolesSizes() const;

    void setStyleOption(const KItemListStyleOption& option);
    const KItemListStyleOption& styleOption() const;

    void setSelected(bool selected);
    bool isSelected() const;

    void setCurrent(bool current);
    bool isCurrent() const;

    void setHovered(bool hovered);
    bool isHovered() const;

    void setAlternatingBackgroundColors(bool enable);
    bool alternatingBackgroundColors() const;

    /**
     * @return True if \a point is inside KItemListWidget::hoverRect(),
     *         KItemListWidget::textRect(), KItemListWidget::selectionToggleRect()
     *         or KItemListWidget::expansionToggleRect().
     * @reimp
     */
    virtual bool contains(const QPointF& point) const;

    /**
     * @return Rectangle for the area that shows the icon.
     */
    virtual QRectF iconRect() const = 0;

    /**
     * @return Rectangle for the area that contains the text-properties.
     */
    virtual QRectF textRect() const = 0;

    /**
     * @return Rectangle for the selection-toggle that is used to select or deselect an item.
     *         Per default an empty rectangle is returned which means that no selection-toggle
     *         is available.
     */
    virtual QRectF selectionToggleRect() const;

    /**
     * @return Rectangle for the expansion-toggle that is used to open a sub-tree of the model.
     *         Per default an empty rectangle is returned which means that no opening of sub-trees
     *         is supported.
     */
    virtual QRectF expansionToggleRect() const;

protected:
    virtual void dataChanged(const QHash<QByteArray, QVariant>& current, const QSet<QByteArray>& roles = QSet<QByteArray>());
    virtual void visibleRolesChanged(const QList<QByteArray>& current, const QList<QByteArray>& previous);
    virtual void visibleRolesSizesChanged(const QHash<QByteArray, QSizeF>& current, const QHash<QByteArray, QSizeF>& previous);
    virtual void styleOptionChanged(const KItemListStyleOption& current, const KItemListStyleOption& previous);
    virtual void currentChanged(bool current);
    virtual void selectedChanged(bool selected);
    virtual void hoveredChanged(bool hovered);
    virtual void alternatingBackgroundColorsChanged(bool enabled);
    virtual void resizeEvent(QGraphicsSceneResizeEvent* event);    

    /**
     * @return The current opacity of the hover-animation. When implementing a custom painting-code for a hover-state
     *         this opacity value should be respected.
     */
    qreal hoverOpacity() const;

private:
    void setHoverOpacity(qreal opacity);
    void clearHoverCache();
    void drawTextBackground(QPainter* painter);

private:
    Q_PROPERTY(qreal hoverOpacity READ hoverOpacity WRITE setHoverOpacity)

    int m_index;
    bool m_selected;
    bool m_current;
    bool m_hovered;
    bool m_alternatingBackgroundColors;
    QHash<QByteArray, QVariant> m_data;
    QList<QByteArray> m_visibleRoles;
    QHash<QByteArray, QSizeF> m_visibleRolesSizes;
    KItemListStyleOption m_styleOption;

    qreal m_hoverOpacity;
    mutable QPixmap* m_hoverCache;
    QPropertyAnimation* m_hoverAnimation;
};
#endif


