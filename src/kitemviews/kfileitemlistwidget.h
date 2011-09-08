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

#ifndef KFILEITEMLISTWIDGET_H
#define KFILEITEMLISTWIDGET_H

#include <libdolphin_export.h>

#include <kitemviews/kitemlistwidget.h>

#include <QPixmap>
#include <QPointF>
#include <QStaticText>

class LIBDOLPHINPRIVATE_EXPORT KFileItemListWidget : public KItemListWidget
{
    Q_OBJECT

public:
    enum Layout
    {
        IconsLayout,
        CompactLayout,
        DetailsLayout
    };

    KFileItemListWidget(QGraphicsItem* parent);
    virtual ~KFileItemListWidget();

    void setLayout(Layout layout);
    Layout layout() const;

    virtual void paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget = 0);

    virtual QRectF iconBoundingRect() const;
    virtual QRectF textBoundingRect() const;
    virtual QRectF expansionToggleRect() const;

protected:
    void setTextColor(const QColor& color);
    QColor textColor() const;

    void setOverlay(const QPixmap& overlay);
    QPixmap overlay() const;
    
    virtual void dataChanged(const QHash<QByteArray, QVariant>& current, const QSet<QByteArray>& roles = QSet<QByteArray>());
    virtual void visibleRolesChanged(const QHash<QByteArray, int>& current, const QHash<QByteArray, int>& previous);
    virtual void visibleRolesSizesChanged(const QHash<QByteArray, QSizeF>& current, const QHash<QByteArray, QSizeF>& previous);
    virtual void styleOptionChanged(const KItemListStyleOption& current, const KItemListStyleOption& previous);
    virtual void hoveredChanged(bool hovered);
    virtual void resizeEvent(QGraphicsSceneResizeEvent* event);

private:
    enum TextId {
        Name,
        Size,
        Date,
        Permissions,
        Owner,
        Group,
        Type,
        Destination,
        Path,
        TextIdCount // Mandatory last entry
    };

    void updateCache();
    void updateExpansionArea();
    void updatePixmapCache();

    void updateTextsCache();
    void updateIconsLayoutTextCache();
    void updateCompactLayoutTextCache();
    void updateDetailsLayoutTextCache();
    
    void updateAdditionalInfoTextColor();

    QString roleText(TextId textId, const QVariant& roleValue) const;

    void drawPixmap(QPainter* painter, const QPixmap& pixmap);

    static QPixmap pixmapForIcon(const QString& name, int size);
    static TextId roleTextId(const QByteArray& role);

private:
    bool m_isDir;
    bool m_dirtyLayout;
    bool m_dirtyContent;
    QSet<QByteArray> m_dirtyContentRoles;

    Layout m_layout;
    QPointF m_pixmapPos;
    QPixmap m_pixmap;
    QSize m_scaledPixmapSize;

    QRectF m_hoverPixmapRect;
    QPixmap m_hoverPixmap;

    QPointF m_textPos[TextIdCount];
    QStaticText m_text[TextIdCount];
    QRectF m_textBoundingRect;

    QList<QByteArray> m_sortedVisibleRoles;

    QRectF m_expansionArea;

    QColor m_customTextColor;
    QColor m_additionalInfoTextColor;

    QPixmap m_overlay;
};

#endif


