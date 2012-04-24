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

#ifndef KITEMLISTGROUPHEADER_H
#define KITEMLISTGROUPHEADER_H

#include <libdolphin_export.h>

#include <kitemviews/kitemliststyleoption.h>

#include <QByteArray>
#include <QGraphicsWidget>
#include <QVariant>

class KItemListView;

/**
 * @brief Base class for group headers.
 *
 * Draws a default header background. Derived classes must reimplement
 * the method paint() and draw the role within the given roleBounds() with
 * the color roleColor().
 */
class LIBDOLPHINPRIVATE_EXPORT KItemListGroupHeader : public QGraphicsWidget
{
    Q_OBJECT

public:
    KItemListGroupHeader(QGraphicsWidget* parent = 0);
    virtual ~KItemListGroupHeader();

    void setRole(const QByteArray& role);
    QByteArray role() const;

    void setData(const QVariant& data);
    QVariant data() const;

    void setStyleOption(const KItemListStyleOption& option);
    const KItemListStyleOption& styleOption() const;

    /**
     * Sets the scroll orientation that is used by the KItemListView.
     * This allows the group header to use a modified look dependent
     * on the orientation.
     */
    void setScrollOrientation(Qt::Orientation orientation);
    Qt::Orientation scrollOrientation() const;

    void setItemIndex(int index);
    int itemIndex() const;

    virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = 0);

protected:
    virtual void paintRole(QPainter* painter, const QRectF& roleBounds, const QColor& color) = 0;
    virtual void paintSeparator(QPainter* painter, const QColor& color) = 0;

    /**
     * Is called after the role has been changed and allows the derived class
     * to react on this change.
     */
    virtual void roleChanged(const QByteArray& current, const QByteArray& previous);

    /**
     * Is called after the role has been changed and allows the derived class
     * to react on this change.
     */
    virtual void dataChanged(const QVariant& current, const QVariant& previous);

    /**
     * Is called after the style option has been changed and allows the derived class
     * to react on this change.
     */
    virtual void styleOptionChanged(const KItemListStyleOption& current, const KItemListStyleOption& previous);

    /**
     * Is called after the scroll orientation has been changed and allows the derived class
     * to react on this change.
     */
    virtual void scrollOrientationChanged(Qt::Orientation current, Qt::Orientation previous);

    /**
     * Is called after the item index has been changed and allows the derived class to react on
     * this change.
     */
    virtual void itemIndexChanged(int current, int previous);

    /** @reimp */
    virtual void resizeEvent(QGraphicsSceneResizeEvent* event);

private:
    void updateCache();

    static QColor mixedColor(const QColor& c1, const QColor& c2, int c1Percent = 50);

private:
    bool m_dirtyCache;
    QByteArray m_role;
    QVariant m_data;
    KItemListStyleOption m_styleOption;
    Qt::Orientation m_scrollOrientation;
    int m_itemIndex;

    QColor m_separatorColor;
    QColor m_roleColor;
    QRectF m_roleBounds;
};
#endif


